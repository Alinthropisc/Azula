// File: js/http.go
package js

import (
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"net/url"
	"strings"
	"sync"
	"time"

	"github.com/Alinthropisc/Azula/log"
	"github.com/robertkrimen/otto"
)

// defaultHTTPTimeout protects scripts from hanging forever: the original
// client had no timeout at all.
const defaultHTTPTimeout = 30 * time.Second

// maxResponseSize bounds the response body kept in memory.
// It is a variable so tests can lower it.
var maxResponseSize int64 = 32 << 20

var (
	httpClientMu sync.RWMutex
	httpClient   = &http.Client{Timeout: defaultHTTPTimeout}
)

// SetHTTPClient replaces the HTTP client used by all JS builtins
// (Strategy / DIP): tests inject httptest, production may tune the timeout
// or transport. nil restores the default client. Safe for concurrent use.
func SetHTTPClient(client *http.Client) {
	httpClientMu.Lock()
	defer httpClientMu.Unlock()
	if client == nil {
		client = &http.Client{Timeout: defaultHTTPTimeout}
	}
	httpClient = client
}

// HTTPClient returns the current client (never nil).
func HTTPClient() *http.Client {
	httpClientMu.RLock()
	defer httpClientMu.RUnlock()
	return httpClient
}

// readBody reads at most maxResponseSize bytes and fails on larger bodies.
func readBody(r io.Reader) ([]byte, error) {
	raw, err := io.ReadAll(io.LimitReader(r, maxResponseSize+1))
	if err != nil {
		return nil, err
	}
	if int64(len(raw)) > maxResponseSize {
		return nil, fmt.Errorf("response body exceeds %d bytes", maxResponseSize)
	}
	return raw, nil
}

type httpResponse struct {
	Error    error
	Response *http.Response
	Raw      []byte
	Body     string
	JSON     interface{}
}

type httpPackage struct{}

func (httpPackage) Encode(s string) string {
	return url.QueryEscape(s)
}

func (httpPackage) Request(method string, uri string,
	headers map[string]string,
	form map[string]string,
	jsonBody string) httpResponse {

	defer log.TraceFunc("js.http.Request")()
	log.Tracing("js: http %s %s", method, uri)

	var reader io.Reader
	switch {
	case form != nil:
		data := url.Values{}
		for k, v := range form {
			data.Set(k, v)
		}
		reader = strings.NewReader(data.Encode())
	case jsonBody != "":
		reader = strings.NewReader(jsonBody)
	}

	req, err := http.NewRequest(method, uri, reader)
	if err != nil {
		return httpResponse{Error: err}
	}

	switch {
	case form != nil:
		req.Header.Set("Content-Type", "application/x-www-form-urlencoded")
	case jsonBody != "":
		req.Header.Set("Content-Type", "application/json")
	}
	for name, value := range headers {
		req.Header.Add(name, value)
	}

	resp, err := HTTPClient().Do(req)
	if err != nil {
		return httpResponse{Error: err}
	}
	defer resp.Body.Close()

	raw, err := readBody(resp.Body)
	if err != nil {
		return httpResponse{Error: err}
	}

	res := httpResponse{
		Response: resp,
		Raw:      raw,
		Body:     string(raw),
	}

	// The JSON field was declared in the original but never populated.
	if len(raw) > 0 && strings.Contains(resp.Header.Get("Content-Type"), "json") {
		var parsed interface{}
		if err := json.Unmarshal(raw, &parsed); err == nil {
			res.JSON = parsed
		}
	}

	// Any non-2xx status is an error (the original flagged 201/204 too).
	if resp.StatusCode < 200 || resp.StatusCode > 299 {
		res.Error = fmt.Errorf("%s", resp.Status)
	}
	return res
}

func (c httpPackage) Get(uri string, headers map[string]string) httpResponse {
	return c.Request(http.MethodGet, uri, headers, nil, "")
}

func (c httpPackage) PostForm(uri string, headers map[string]string, form map[string]string) httpResponse {
	return c.Request(http.MethodPost, uri, headers, form, "")
}

func (c httpPackage) PostJSON(uri string, headers map[string]string, jsonBody string) httpResponse {
	return c.Request(http.MethodPost, uri, headers, nil, jsonBody)
}

func httpRequest(call otto.FunctionCall) otto.Value {
	if !checkMinArgs(call, "httpRequest", 2) {
		return NullValue
	}
	defer log.TraceFunc("js.httpRequest")()

	method := call.Argument(0).String()
	uri := call.Argument(1).String()

	var body io.Reader
	if len(call.ArgumentList) >= 3 {
		body = strings.NewReader(call.Argument(2).String())
	}

	// A single creation point: the original overwrote (req, err) when
	// argc >= 3 and silently lost the first error.
	req, err := http.NewRequest(method, uri, body)
	if err != nil {
		return ReportError("httpRequest: could not create request to %s: %s", uri, err)
	}

	if len(call.ArgumentList) > 3 {
		if hv := call.Argument(3); hv.IsDefined() && !hv.IsNull() {
			headers := hv.Object()
			if headers == nil {
				return ReportError("httpRequest: headers argument must be an object.")
			}
			for _, key := range headers.Keys() {
				v, err := headers.Get(key)
				if err != nil {
					return ReportError("httpRequest: could not read header %s: %s", key, err)
				}
				req.Header.Add(key, v.String())
			}
		}
	}

	resp, err := HTTPClient().Do(req)
	if err != nil {
		return ReportError("httpRequest: could not request %s: %s", uri, err)
	}
	defer resp.Body.Close()

	raw, err := readBody(resp.Body)
	if err != nil {
		return ReportError("httpRequest: could not read response: %s", err)
	}

	// The object lives in the calling VM, not in a fresh otto.New().
	object, err := call.Otto.Object("({})")
	if err != nil {
		return ReportError("httpRequest: could not create response object: %s", err)
	}
	if err := object.Set("body", string(raw)); err != nil {
		return ReportError("httpRequest: could not populate response object: %s", err)
	}
	if err := object.Set("status", resp.StatusCode); err != nil {
		return ReportError("httpRequest: could not populate response object: %s", err)
	}
	return object.Value()
}
