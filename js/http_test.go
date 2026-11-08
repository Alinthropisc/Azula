package js

import (
	"encoding/json"
	"io"
	"net/http"
	"net/http/httptest"
	"net/url"
	"strings"
	"sync"
	"testing"
	"time"
)

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

type echoed struct {
	Method      string
	ContentType string
	Body        string
	Header      http.Header
}

func useHTTPClient(t *testing.T, c *http.Client) {
	t.Helper()
	SetHTTPClient(c)
	t.Cleanup(func() { SetHTTPClient(nil) })
}

// newEchoServer replies with JSON describing the request it received.
func newEchoServer(t *testing.T) *httptest.Server {
	t.Helper()
	srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		body, _ := io.ReadAll(r.Body)
		w.Header().Set("Content-Type", "application/json")
		_ = json.NewEncoder(w).Encode(echoed{
			Method:      r.Method,
			ContentType: r.Header.Get("Content-Type"),
			Body:        string(body),
			Header:      r.Header,
		})
	}))
	t.Cleanup(srv.Close)
	useHTTPClient(t, srv.Client())
	return srv
}

// newFixedServer replies with a fixed status, content type and body.
func newFixedServer(t *testing.T, status int, contentType, body string) *httptest.Server {
	t.Helper()
	srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if contentType != "" {
			w.Header().Set("Content-Type", contentType)
		}
		w.WriteHeader(status)
		if body != "" {
			_, _ = io.WriteString(w, body)
		}
	}))
	t.Cleanup(srv.Close)
	useHTTPClient(t, srv.Client())
	return srv
}

func decodeEcho(t *testing.T, body string) echoed {
	t.Helper()
	var e echoed
	if err := json.Unmarshal([]byte(body), &e); err != nil {
		t.Fatalf("cannot decode echo %q: %v", body, err)
	}
	return e
}

// ---------------------------------------------------------------------------
// Client management
// ---------------------------------------------------------------------------

func TestHTTPClient_DefaultAndOverride(t *testing.T) {
	SetHTTPClient(nil)
	t.Cleanup(func() { SetHTTPClient(nil) })

	def := HTTPClient()
	if def == nil {
		t.Fatal("HTTPClient() returned nil")
	}
	if def.Timeout != defaultHTTPTimeout {
		t.Errorf("default timeout = %v, want %v", def.Timeout, defaultHTTPTimeout)
	}

	custom := &http.Client{Timeout: time.Second}
	SetHTTPClient(custom)
	if HTTPClient() != custom {
		t.Error("SetHTTPClient did not install the client")
	}

	SetHTTPClient(nil)
	if got := HTTPClient(); got == nil || got == custom || got.Timeout != defaultHTTPTimeout {
		t.Error("SetHTTPClient(nil) must restore a fresh default client")
	}
}

// Meaningful with -race.
func TestHTTPClient_ConcurrentAccess(t *testing.T) {
	t.Cleanup(func() { SetHTTPClient(nil) })

	var wg sync.WaitGroup
	for i := 0; i < 8; i++ {
		wg.Add(2)
		go func() {
			defer wg.Done()
			for j := 0; j < 100; j++ {
				SetHTTPClient(&http.Client{Timeout: time.Second})
			}
		}()
		go func() {
			defer wg.Done()
			for j := 0; j < 100; j++ {
				if HTTPClient() == nil {
					t.Error("HTTPClient() returned nil")
					return
				}
			}
		}()
	}
	wg.Wait()
}

// ---------------------------------------------------------------------------
// httpPackage.Request and helpers
// ---------------------------------------------------------------------------

func TestHTTPEncode(t *testing.T) {
	if got := (httpPackage{}).Encode("a b&c=d/é"); got != url.QueryEscape("a b&c=d/é") {
		t.Errorf("Encode = %q", got)
	}
	if got := (httpPackage{}).Encode("a b&c"); got != "a+b%26c" {
		t.Errorf("Encode = %q", got)
	}
}

func TestRequest_GetWithHeaders(t *testing.T) {
	srv := newEchoServer(t)

	res := httpPackage{}.Get(srv.URL, map[string]string{"X-Test": "1"})
	if res.Error != nil {
		t.Fatalf("Error = %v", res.Error)
	}
	if res.Response == nil || res.Response.StatusCode != http.StatusOK {
		t.Fatalf("Response = %+v", res.Response)
	}
	if res.Body != string(res.Raw) || res.Body == "" {
		t.Errorf("Body/Raw mismatch: %q vs %q", res.Body, res.Raw)
	}

	e := decodeEcho(t, res.Body)
	if e.Method != http.MethodGet {
		t.Errorf("method = %s", e.Method)
	}
	if e.Header.Get("X-Test") != "1" {
		t.Errorf("custom header lost: %v", e.Header)
	}
	if e.ContentType != "" {
		t.Errorf("GET without a body must not set Content-Type, got %q", e.ContentType)
	}
}

func TestRequest_PostForm(t *testing.T) {
	srv := newEchoServer(t)

	res := httpPackage{}.PostForm(srv.URL,
		map[string]string{"X-T": "v"},
		map[string]string{"a": "1 2", "b": "x&y"})
	if res.Error != nil {
		t.Fatalf("Error = %v", res.Error)
	}

	e := decodeEcho(t, res.Body)
	if e.Method != http.MethodPost {
		t.Errorf("method = %s", e.Method)
	}
	if e.ContentType != "application/x-www-form-urlencoded" {
		t.Errorf("Content-Type = %q", e.ContentType)
	}
	form, err := url.ParseQuery(e.Body)
	if err != nil {
		t.Fatal(err)
	}
	if form.Get("a") != "1 2" || form.Get("b") != "x&y" {
		t.Errorf("form = %v", form)
	}
	if e.Header.Get("X-T") != "v" {
		t.Errorf("header lost: %v", e.Header)
	}
}

func TestRequest_PostJSON(t *testing.T) {
	srv := newEchoServer(t)

	res := httpPackage{}.PostJSON(srv.URL, nil, `{"k":"v"}`)
	if res.Error != nil {
		t.Fatalf("Error = %v", res.Error)
	}
	e := decodeEcho(t, res.Body)
	if e.ContentType != "application/json" {
		t.Errorf("Content-Type = %q", e.ContentType)
	}
	if e.Body != `{"k":"v"}` {
		t.Errorf("body = %q", e.Body)
	}
}

func TestRequest_StatusHandling(t *testing.T) {
	tests := []struct {
		status  int
		wantErr bool
	}{
		{200, false}, {201, false}, {204, false}, {299, false},
		{400, true}, {404, true}, {500, true}, {503, true},
	}
	for _, tc := range tests {
		t.Run(http.StatusText(tc.status), func(t *testing.T) {
			srv := newFixedServer(t, tc.status, "text/plain", "")
			res := httpPackage{}.Get(srv.URL, nil)
			if tc.wantErr {
				if res.Error == nil {
					t.Fatalf("status %d: Error is nil", tc.status)
				}
				if !strings.Contains(res.Error.Error(), http.StatusText(tc.status)) {
					t.Errorf("Error = %v, want it to mention the status", res.Error)
				}
			} else if res.Error != nil {
				t.Errorf("status %d must not be an error, got %v", tc.status, res.Error)
			}
			if res.Response == nil || res.Response.StatusCode != tc.status {
				t.Errorf("Response = %+v", res.Response)
			}
		})
	}
}

func TestRequest_JSONField(t *testing.T) {
	t.Run("object", func(t *testing.T) {
		srv := newFixedServer(t, 200, "application/json; charset=utf-8", `{"a":1,"b":[true]}`)
		res := httpPackage{}.Get(srv.URL, nil)
		m, ok := res.JSON.(map[string]interface{})
		if !ok || m["a"] != float64(1) {
			t.Errorf("JSON = %#v", res.JSON)
		}
	})
	t.Run("array", func(t *testing.T) {
		srv := newFixedServer(t, 200, "application/json", `[1,2,3]`)
		res := httpPackage{}.Get(srv.URL, nil)
		if arr, ok := res.JSON.([]interface{}); !ok || len(arr) != 3 {
			t.Errorf("JSON = %#v", res.JSON)
		}
	})
	t.Run("invalid JSON is not an error", func(t *testing.T) {
		srv := newFixedServer(t, 200, "application/json", `{broken`)
		res := httpPackage{}.Get(srv.URL, nil)
		if res.JSON != nil || res.Error != nil {
			t.Errorf("JSON = %#v, Error = %v", res.JSON, res.Error)
		}
		if res.Body != `{broken` {
			t.Errorf("Body = %q", res.Body)
		}
	})
	t.Run("non-json content type", func(t *testing.T) {
		srv := newFixedServer(t, 200, "text/plain", `{"a":1}`)
		if res := (httpPackage{}).Get(srv.URL, nil); res.JSON != nil {
			t.Errorf("JSON = %#v, want nil for text/plain", res.JSON)
		}
	})
	t.Run("empty body", func(t *testing.T) {
		srv := newFixedServer(t, 200, "application/json", "")
		if res := (httpPackage{}).Get(srv.URL, nil); res.JSON != nil || res.Error != nil {
			t.Errorf("JSON = %#v, Error = %v", res.JSON, res.Error)
		}
	})
}

func TestRequest_BodySizeLimit(t *testing.T) {
	overrideInt64(t, &maxResponseSize, 10)

	t.Run("exactly at the limit", func(t *testing.T) {
		srv := newFixedServer(t, 200, "text/plain", "0123456789")
		res := httpPackage{}.Get(srv.URL, nil)
		if res.Error != nil || res.Body != "0123456789" {
			t.Errorf("Body = %q, Error = %v", res.Body, res.Error)
		}
	})
	t.Run("one byte over", func(t *testing.T) {
		srv := newFixedServer(t, 200, "text/plain", "0123456789A")
		res := httpPackage{}.Get(srv.URL, nil)
		if res.Error == nil || !strings.Contains(res.Error.Error(), "exceeds") {
			t.Errorf("Error = %v, want a size-limit error", res.Error)
		}
	})
}

func TestRequest_Timeout(t *testing.T) {
	release := make(chan struct{})
	srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		select {
		case <-r.Context().Done():
		case <-release:
		case <-time.After(5 * time.Second):
		}
	}))
	t.Cleanup(srv.Close)
	t.Cleanup(func() { close(release) }) // runs before srv.Close
	useHTTPClient(t, &http.Client{Timeout: 100 * time.Millisecond})

	start := time.Now()
	res := httpPackage{}.Get(srv.URL, nil)
	if res.Error == nil {
		t.Fatal("expected a timeout error")
	}
	if res.Response != nil {
		t.Error("Response must be nil on transport errors")
	}
	if d := time.Since(start); d > 3*time.Second {
		t.Errorf("request took %v, the client timeout was not honoured", d)
	}
}

func TestRequest_TransportAndURLErrors(t *testing.T) {
	t.Run("connection refused", func(t *testing.T) {
		srv := httptest.NewServer(http.NotFoundHandler())
		addr := srv.URL
		srv.Close()
		useHTTPClient(t, &http.Client{Timeout: 2 * time.Second})

		if res := (httpPackage{}).Get(addr, nil); res.Error == nil {
			t.Error("expected a connection error")
		}
	})
	t.Run("invalid url", func(t *testing.T) {
		if res := (httpPackage{}).Get("://bad url", nil); res.Error == nil {
			t.Error("expected a URL error")
		}
	})
	t.Run("invalid method", func(t *testing.T) {
		if res := (httpPackage{}).Request("BAD METHOD", "http://127.0.0.1", nil, nil, ""); res.Error == nil {
			t.Error("expected a method error")
		}
	})
}

// ---------------------------------------------------------------------------
// Through JS: http.* and httpRequest
// ---------------------------------------------------------------------------

func TestJS_HTTPPackage(t *testing.T) {
	srv := newEchoServer(t)
	vm := newVM(t)
	setVars(t, vm, map[string]interface{}{"u": srv.URL})

	body := runString(t, vm, `http.Get(u, {"X-A": "1"}).Body`)
	if e := decodeEcho(t, body); e.Header.Get("X-A") != "1" {
		t.Errorf("header from JS lost: %v", e.Header)
	}
	if got := runString(t, vm, `http.Encode("a b")`); got != "a+b" {
		t.Errorf("http.Encode = %q", got)
	}
}

func TestJS_HttpRequest_Basic(t *testing.T) {
	srv := newEchoServer(t)
	vm := newVM(t)
	setVars(t, vm, map[string]interface{}{"u": srv.URL})

	if n := runInt(t, vm, `httpRequest("GET", u).status`); n != 200 {
		t.Errorf("status = %d, want 200", n)
	}
	e := decodeEcho(t, runString(t, vm, `httpRequest("GET", u).body`))
	if e.Method != http.MethodGet {
		t.Errorf("method = %s", e.Method)
	}
}

func TestJS_HttpRequest_BodyAndHeaders(t *testing.T) {
	srv := newEchoServer(t)
	vm := newVM(t)
	setVars(t, vm, map[string]interface{}{"u": srv.URL})

	body := runString(t, vm, `httpRequest("POST", u, "payload", {"X-Token": "abc"}).body`)
	e := decodeEcho(t, body)
	if e.Method != http.MethodPost || e.Body != "payload" {
		t.Errorf("echo = %+v", e)
	}
	if e.Header.Get("X-Token") != "abc" {
		t.Errorf("header lost: %v", e.Header)
	}
}

func TestJS_HttpRequest_NullAndUndefinedHeaders(t *testing.T) {
	sink := captureLogs(t)
	srv := newEchoServer(t)
	vm := newVM(t)
	setVars(t, vm, map[string]interface{}{"u": srv.URL})

	for _, src := range []string{
		`httpRequest("GET", u, "", null).status`,
		`httpRequest("GET", u, "", undefined).status`,
	} {
		if n := runInt(t, vm, src); n != 200 {
			t.Errorf("%s = %d, want 200 (null/undefined mean no headers)", src, n)
		}
	}
	if n := len(sink.errors()); n != 0 {
		t.Errorf("unexpected errors: %q", sink.errors())
	}
}

func TestJS_HttpRequest_HeadersMustBeObject(t *testing.T) {
	sink := captureLogs(t)
	srv := newEchoServer(t)
	vm := newVM(t)
	setVars(t, vm, map[string]interface{}{"u": srv.URL})

	requireFailed(t, sink, run(t, vm, `httpRequest("GET", u, "", "not-an-object")`), "string headers")
	requireReported(t, sink, "headers argument must be an object")
}

// Documents the current contract: non-2xx statuses are not errors at the
// httpRequest level, the script inspects `status` itself.
func TestJS_HttpRequest_NonSuccessStatusIsReturned(t *testing.T) {
	sink := captureLogs(t)
	srv := newFixedServer(t, 404, "text/plain", "nope")
	vm := newVM(t)
	setVars(t, vm, map[string]interface{}{"u": srv.URL})

	if n := runInt(t, vm, `httpRequest("GET", u).status`); n != 404 {
		t.Errorf("status = %d, want 404", n)
	}
	if got := runString(t, vm, `httpRequest("GET", u).body`); got != "nope" {
		t.Errorf("body = %q", got)
	}
	if n := len(sink.errors()); n != 0 {
		t.Errorf("unexpected errors: %q", sink.errors())
	}
}

func TestJS_HttpRequest_Errors(t *testing.T) {
	t.Run("invalid method", func(t *testing.T) {
		sink := captureLogs(t)
		vm := newVM(t)
		requireFailed(t, sink, run(t, vm, `httpRequest("BAD METHOD", "http://127.0.0.1")`), "bad method")
		requireReported(t, sink, "could not create request")
	})
	t.Run("connection refused", func(t *testing.T) {
		srv := httptest.NewServer(http.NotFoundHandler())
		addr := srv.URL
		srv.Close()
		useHTTPClient(t, &http.Client{Timeout: 2 * time.Second})

		sink := captureLogs(t)
		vm := newVM(t)
		setVars(t, vm, map[string]interface{}{"u": addr})
		requireFailed(t, sink, run(t, vm, `httpRequest("GET", u)`), "refused")
		requireReported(t, sink, "could not request")
	})
	t.Run("response too large", func(t *testing.T) {
		overrideInt64(t, &maxResponseSize, 5)
		srv := newFixedServer(t, 200, "text/plain", "0123456789")

		sink := captureLogs(t)
		vm := newVM(t)
		setVars(t, vm, map[string]interface{}{"u": srv.URL})
		requireFailed(t, sink, run(t, vm, `httpRequest("GET", u)`), "too large")
		requireReported(t, sink, "could not read response")
	})
}
