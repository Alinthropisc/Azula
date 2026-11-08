package js

import (
	"errors"
	"fmt"
	"strings"
	"testing"

	"github.com/Alinthropisc/Azula/core"
	"github.com/Alinthropisc/Azula/log"
	"github.com/evilsocket/islazy/plugin"
	"github.com/robertkrimen/otto"
)

var publicBuiltins = []string{
	"mkdirAll", "readDir", "readFile", "writeFile", "appendFile",
	"log", "log_debug", "log_info", "log_warn", "log_error", "log_fatal", "log_trace",
	"Crypto", "btoa", "atob", "gzipCompress", "gzipDecompress",
	"textEncode", "textDecode", "httpRequest", "http", "random", "azula",
}

// ---------------------------------------------------------------------------
// Registry
// ---------------------------------------------------------------------------

func TestDefinitions_ContainsPublicAPI(t *testing.T) {
	defs := Definitions()
	for _, name := range publicBuiltins {
		if _, ok := defs[name]; !ok {
			t.Errorf("builtin %q missing from Definitions(): this is public API of caplets", name)
		}
	}
}

func TestDefinitions_ReturnsFreshMap(t *testing.T) {
	a := Definitions()
	delete(a, "btoa")
	if _, ok := Definitions()["btoa"]; !ok {
		t.Error("mutating the returned map must not affect later calls")
	}
}

func TestInit_PopulatesPluginDefines(t *testing.T) {
	for name := range Definitions() {
		if _, ok := plugin.Defines[name]; !ok {
			t.Errorf("plugin.Defines is missing %q (init must copy the registry)", name)
		}
	}
}

func TestCryptoObject_Functions(t *testing.T) {
	vm := newVM(t)
	for _, fn := range []string{"sha1", "sha256", "md5", "sha1Hex", "sha256Hex", "md5Hex"} {
		if got := runString(t, vm, "typeof Crypto."+fn); got != "function" {
			t.Errorf("typeof Crypto.%s = %q, want function", fn, got)
		}
	}
}

func TestAzulaGlobal(t *testing.T) {
	vm := newVM(t)
	info := core.Info()
	want := map[string]string{
		"name":    info.Name,
		"version": info.Version,
		"author":  info.Author,
		"website": info.Website,
	}
	for key, w := range want {
		if got := runString(t, vm, "azula."+key); got != w {
			t.Errorf("azula.%s = %q, want %q", key, got, w)
		}
	}
	// Keys must be exactly as documented, not Go-style capitalised.
	if got := runString(t, vm, "typeof azula.Name"); got != "undefined" {
		t.Errorf("typeof azula.Name = %q, want undefined", got)
	}
}

// ---------------------------------------------------------------------------
// ReportError / NullValue
// ---------------------------------------------------------------------------

func TestNullValueIsUndefined(t *testing.T) {
	if !NullValue.IsUndefined() {
		t.Error("NullValue must stay otto's zero Value (undefined): API contract")
	}
}

func TestReportError(t *testing.T) {
	sink := captureLogs(t)

	v := ReportError("failed %s #%d", "op", 7)
	if !v.IsUndefined() {
		t.Errorf("ReportError result = %v, want undefined", v)
	}
	errs := sink.errors()
	if len(errs) != 1 || errs[0] != "failed op #7" {
		t.Errorf("logged errors = %q", errs)
	}
}

// ---------------------------------------------------------------------------
// unary decorator
// ---------------------------------------------------------------------------

func TestUnary_Decorator(t *testing.T) {
	sink := captureLogs(t)

	calls := 0
	upper := func(s string) (interface{}, error) {
		calls++
		if s == "bad" {
			return nil, errors.New("boom")
		}
		return strings.ToUpper(s), nil
	}

	vm := otto.New()
	setVars(t, vm, map[string]interface{}{
		"strict":  unaryString("t.strict", upper),
		"lenient": unaryCoerced("t.lenient", upper),
	})

	if got := runString(t, vm, `strict("abc")`); got != "ABC" {
		t.Errorf("strict(abc) = %q", got)
	}

	// The wrapped function must not run on invalid input.
	before := calls
	requireFailed(t, sink, run(t, vm, `strict(1)`), "strict(1)")
	if calls != before {
		t.Error("wrapped function was called despite a type error")
	}

	// Errors from the wrapped function are reported with the builtin name.
	sink2 := captureLogs(t)
	v := run(t, vm, `strict("bad")`)
	requireFailed(t, sink2, v, `strict("bad")`)
	requireReported(t, sink2, "t.strict: boom")

	// Lenient variant coerces numbers like the original builtins did.
	if got := runString(t, vm, `lenient(42)`); got != "42" {
		t.Errorf("lenient(42) = %q, want 42", got)
	}
}

// ---------------------------------------------------------------------------
// Argument validation across the real registry
// ---------------------------------------------------------------------------

func TestArgValidation_StrictBuiltins(t *testing.T) {
	strict := []string{
		"btoa", "atob", "gzipCompress", "textEncode",
		"Crypto.sha1", "Crypto.sha256", "Crypto.md5",
		"Crypto.sha1Hex", "Crypto.sha256Hex", "Crypto.md5Hex",
	}
	calls := []string{"%s()", `%s("a","b")`, "%s(42)", "%s(null)"}

	for _, name := range strict {
		for _, tpl := range calls {
			src := fmt.Sprintf(tpl, name)
			t.Run(src, func(t *testing.T) {
				sink := captureLogs(t)
				vm := newVM(t)
				requireFailed(t, sink, run(t, vm, src), src)
			})
		}
	}
}

func TestArgValidation_WrongArgumentCount(t *testing.T) {
	calls := []string{
		`gzipDecompress()`,
		`gzipDecompress("a","b")`,
		`textDecode()`,
		`mkdirAll()`,
		`readDir("a","b")`,
		`readFile()`,
		`writeFile("x")`,
		`appendFile("x","y","z")`,
		`httpRequest()`,
		`httpRequest("GET")`,
	}
	for _, src := range calls {
		t.Run(src, func(t *testing.T) {
			sink := captureLogs(t)
			vm := newVM(t)
			requireFailed(t, sink, run(t, vm, src), src)
		})
	}
}

// ---------------------------------------------------------------------------
// Logging builtins
// ---------------------------------------------------------------------------

func TestLog_Levels(t *testing.T) {
	tests := []struct {
		fn    string
		level log.Level
	}{
		{"log", log.LevelInfo},
		{"log_info", log.LevelInfo},
		{"log_debug", log.LevelDebug},
		{"log_warn", log.LevelWarning},
		{"log_error", log.LevelError},
	}
	for _, tc := range tests {
		t.Run(tc.fn, func(t *testing.T) {
			sink := captureLogs(t)
			vm := newVM(t)

			v := run(t, vm, tc.fn+`("hello")`)
			if !v.IsUndefined() {
				t.Errorf("result = %v, want undefined", v)
			}
			got := sink.all()
			if len(got) != 1 || got[0].Level != tc.level || got[0].Msg != "hello" {
				t.Errorf("entries = %+v, want one %v entry %q", got, tc.level, "hello")
			}
		})
	}
}

func TestLog_MultipleArgsAndCoercion(t *testing.T) {
	sink := captureLogs(t)
	vm := newVM(t)

	run(t, vm, `log_info("a", 42, true)`)

	var msgs []string
	for _, e := range sink.all() {
		msgs = append(msgs, e.Msg)
	}
	want := []string{"a", "42", "true"}
	if strings.Join(msgs, "|") != strings.Join(want, "|") {
		t.Errorf("messages = %q, want %q (one entry per argument)", msgs, want)
	}
}

func TestLog_NoArgsLogsNothing(t *testing.T) {
	sink := captureLogs(t)
	vm := newVM(t)
	run(t, vm, `log_info()`)
	if n := len(sink.all()); n != 0 {
		t.Errorf("%d entries logged for a call without arguments", n)
	}
}

func TestLogTrace_GoesToSubscribersOnly(t *testing.T) {
	sink := captureLogs(t)
	traces := captureTraces(t)
	vm := newVM(t)

	run(t, vm, `log_trace("trace-me")`)

	if !traces.has("trace-me") {
		t.Error("tracing subscriber did not receive the log_trace message")
	}
	for _, e := range sink.all() {
		if e.Msg == "trace-me" {
			t.Errorf("trace message leaked into the logger: %+v", e)
		}
	}
}

func TestLogFatal_ExitsAfterLogging(t *testing.T) {
	sink := captureLogs(t)

	var calls, code, seen int
	old := exit
	exit = func(c int) {
		calls++
		code = c
		seen = len(sink.all())
	}
	t.Cleanup(func() { exit = old })

	vm := newVM(t)
	run(t, vm, `log_fatal("boom", "bang")`)

	if calls != 1 || code != 1 {
		t.Fatalf("exit calls=%d code=%d, want exactly one call with code 1", calls, code)
	}
	if seen != 2 {
		t.Errorf("exit was called after %d log entries, want 2 (all messages logged first)", seen)
	}
	for _, e := range sink.all() {
		if e.Level != log.LevelFatal {
			t.Errorf("entry %+v is not Fatal level", e)
		}
	}
}

func TestLogFatal_NoArgsDoesNotExit(t *testing.T) {
	captureLogs(t)
	called := false
	old := exit
	exit = func(int) { called = true }
	t.Cleanup(func() { exit = old })

	run(t, newVM(t), `log_fatal()`)
	if called {
		t.Error("log_fatal() without arguments must not terminate the process")
	}
}
