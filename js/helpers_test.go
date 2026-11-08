package js

import (
	"fmt"
	"strings"
	"sync"
	"testing"

	"github.com/Alinthropisc/Azula/log"
	"github.com/robertkrimen/otto"
)

// ---------------------------------------------------------------------------
// Adapters: the only place that knows the exact shape of the log facade.
// If a signature differs, fix it here and nowhere else.
// ---------------------------------------------------------------------------

type logEntry struct {
	Level log.Level
	Msg   string
}

type logSink struct {
	mu      sync.Mutex
	entries []logEntry
}

func (s *logSink) add(level log.Level, msg string) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.entries = append(s.entries, logEntry{Level: level, Msg: msg})
}

func (s *logSink) all() []logEntry {
	s.mu.Lock()
	defer s.mu.Unlock()
	return append([]logEntry(nil), s.entries...)
}

func (s *logSink) errors() []string {
	var out []string
	for _, e := range s.all() {
		if e.Level == log.LevelError {
			out = append(out, e.Msg)
		}
	}
	return out
}

// captureLogs replaces the active logger for the duration of the test.
func captureLogs(t *testing.T) *logSink {
	t.Helper()
	sink := &logSink{}
	prev := log.Active()
	log.SetLogger(log.LoggerFunc(func(level log.Level, format string, args ...interface{}) {
		sink.add(level, fmt.Sprintf(format, args...))
	}))
	t.Cleanup(func() { log.SetLogger(prev) })
	return sink
}

type traceSink struct {
	mu   sync.Mutex
	msgs []string
}

func (s *traceSink) has(msg string) bool {
	s.mu.Lock()
	defer s.mu.Unlock()
	for _, m := range s.msgs {
		if m == msg {
			return true
		}
	}
	return false
}

// captureTraces subscribes to tracing events for the duration of the test.
func captureTraces(t *testing.T) *traceSink {
	t.Helper()
	s := &traceSink{}
	unsub := log.SubscribeTracing(log.SubscriberFunc(func(ev log.TraceEvent) {
		s.mu.Lock()
		s.msgs = append(s.msgs, ev.Message)
		s.mu.Unlock()
	}))
	t.Cleanup(unsub)
	return s
}

// ---------------------------------------------------------------------------
// VM helpers
// ---------------------------------------------------------------------------

// newVM returns a VM with every builtin from Definitions() registered.
func newVM(t *testing.T) *otto.Otto {
	t.Helper()
	vm := otto.New()
	for name, value := range Definitions() {
		if err := vm.Set(name, value); err != nil {
			t.Fatalf("vm.Set(%q): %v", name, err)
		}
	}
	return vm
}

func setVars(t *testing.T, vm *otto.Otto, vars map[string]interface{}) {
	t.Helper()
	for k, v := range vars {
		if err := vm.Set(k, v); err != nil {
			t.Fatalf("vm.Set(%q): %v", k, err)
		}
	}
}

func run(t *testing.T, vm *otto.Otto, src string) otto.Value {
	t.Helper()
	v, err := vm.Run(src)
	if err != nil {
		t.Fatalf("js %q: %v", src, err)
	}
	return v
}

func runString(t *testing.T, vm *otto.Otto, src string) string {
	t.Helper()
	v := run(t, vm, src)
	s, err := v.ToString()
	if err != nil {
		t.Fatalf("js %q: ToString: %v", src, err)
	}
	return s
}

func runInt(t *testing.T, vm *otto.Otto, src string) int64 {
	t.Helper()
	v := run(t, vm, src)
	n, err := v.ToInteger()
	if err != nil {
		t.Fatalf("js %q: ToInteger: %v", src, err)
	}
	return n
}

// requireFailed asserts a builtin reported exactly one error and returned
// `undefined` (NullValue).
func requireFailed(t *testing.T, sink *logSink, v otto.Value, what string) {
	t.Helper()
	if !v.IsUndefined() {
		t.Errorf("%s: result = %v, want undefined", what, v)
	}
	if n := len(sink.errors()); n != 1 {
		t.Errorf("%s: %d errors logged (%q), want exactly 1", what, n, sink.errors())
	}
}

func requireReported(t *testing.T, sink *logSink, substr string) {
	t.Helper()
	for _, m := range sink.errors() {
		if strings.Contains(m, substr) {
			return
		}
	}
	t.Errorf("no logged error contains %q; got %q", substr, sink.errors())
}

func overrideInt64(t *testing.T, p *int64, v int64) {
	t.Helper()
	old := *p
	*p = v
	t.Cleanup(func() { *p = old })
}
