// File: js/data.go
package js

import (
	"bytes"
	"compress/gzip"
	"encoding/base64"
	"fmt"
	"io"
	"strings"

	"github.com/robertkrimen/otto"
)

// maxDecompressedSize bounds gzipDecompress output (gzip-bomb protection).
// It is a variable so tests can lower it.
var maxDecompressedSize int64 = 64 << 20

var btoa = unaryString("btoa", func(s string) (interface{}, error) {
	return base64.StdEncoding.EncodeToString([]byte(s)), nil
})

var atob = unaryString("atob", func(s string) (interface{}, error) {
	raw, err := base64.StdEncoding.DecodeString(s)
	if err != nil {
		return nil, fmt.Errorf("could not decode string: %w", err)
	}
	return string(raw), nil
})

var gzipCompress = unaryString("gzipCompress", func(s string) (interface{}, error) {
	var buf bytes.Buffer
	writer := gzip.NewWriter(&buf)
	if _, err := writer.Write([]byte(s)); err != nil {
		return nil, fmt.Errorf("could not compress data: %w", err)
	}
	// Close flushes the final block; ignoring its error truncates the stream.
	if err := writer.Close(); err != nil {
		return nil, fmt.Errorf("could not finalize gzip stream: %w", err)
	}
	return buf.String(), nil
})

// gzipDecompress historically accepted any value (ToString), keep it lenient.
var gzipDecompress = unaryCoerced("gzipDecompress", func(s string) (interface{}, error) {
	reader, err := gzip.NewReader(strings.NewReader(s))
	if err != nil {
		return nil, fmt.Errorf("could not create gzip reader: %w", err)
	}
	defer reader.Close()

	raw, err := io.ReadAll(io.LimitReader(reader, maxDecompressedSize+1))
	if err != nil {
		return nil, fmt.Errorf("could not decompress data: %w", err)
	}
	if int64(len(raw)) > maxDecompressedSize {
		return nil, fmt.Errorf("decompressed data exceeds %d bytes", maxDecompressedSize)
	}
	return string(raw), nil
})

// textEncode converts in the calling VM (call.Otto), not in a throw-away one.
func textEncode(call otto.FunctionCall) otto.Value {
	if !checkArgs(call, "textEncode", 1) {
		return NullValue
	}
	arg, ok := strictString(call, "textEncode", 0)
	if !ok {
		return NullValue
	}
	return toValue(call, "textEncode", []byte(arg))
}

func textDecode(call otto.FunctionCall) otto.Value {
	if !checkArgs(call, "textDecode", 1) {
		return NullValue
	}
	exported, err := call.Argument(0).Export()
	if err != nil {
		return ReportError("textDecode: could not export argument value: %s", err)
	}
	raw, ok := exported.([]uint8)
	if !ok {
		return ReportError("textDecode: single argument must be of type []uint8.")
	}
	return toValue(call, "textDecode", string(raw))
}
