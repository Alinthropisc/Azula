package core

import (
	"fmt"
	"strings"
	"sync"
	"testing"
)

type logRecord struct {
	Level Level
	Msg   string
}

// captureLogs swaps the active logger for an in-memory recorder and restores
// the default one on cleanup. Tests using it must not call t.Parallel().
func captureLogs(t *testing.T) func() []logRecord {
	t.Helper()

	var mu sync.Mutex
	var recs []logRecord

	SetLogger(LoggerFunc(func(level Level, format string, args ...interface{}) {
		mu.Lock()
		defer mu.Unlock()
		recs = append(recs, logRecord{level, fmt.Sprintf(format, args...)})
	}))
	t.Cleanup(func() { SetLogger(nil) })

	return func() []logRecord {
		mu.Lock()
		defer mu.Unlock()
		return append([]logRecord(nil), recs...)
	}
}

// captureTrace subscribes to tracing events and unsubscribes on cleanup.
func captureTrace(t *testing.T) func() []string {
	t.Helper()

	var mu sync.Mutex
	var msgs []string

	unsub := SubscribeTracing(SubscriberFunc(func(e TraceEvent) {
		mu.Lock()
		msgs = append(msgs, e.Message)
		mu.Unlock()
	}))
	t.Cleanup(unsub)

	return func() []string {
		mu.Lock()
		defer mu.Unlock()
		return append([]string(nil), msgs...)
	}
}

// hasLog reports whether any record has the given level and contains all parts.
func hasLog(recs []logRecord, level Level, parts ...string) bool {
	for _, r := range recs {
		if r.Level != level {
			continue
		}
		ok := true
		for _, p := range parts {
			if !strings.Contains(r.Msg, p) {
				ok = false
				break
			}
		}
		if ok {
			return true
		}
	}
	return false
}

// sameStrings compares slices by content; nil and empty are considered equal.
func sameStrings(a, b []string) bool {
	if len(a) != len(b) {
		return false
	}
	for i := range a {
		if a[i] != b[i] {
			return false
		}
	}
	return true
}
