package core

import (
	"fmt"
	"path/filepath"
	"runtime"
	"sync"
	"sync/atomic"
	"time"

	islazy "github.com/evilsocket/islazy/log"
)

type Level int8

const (
	LevelTracing Level = iota
	LevelDebug
	LevelInfo
	LevelImportant
	LevelWarning
	LevelError
	LevelFatal
)

var levelNames = [...]string{"TRC", "DBG", "INF", "IMP", "WRN", "ERR", "FTL"}

func (l Level) String() string {
	if l < 0 || int(l) >= len(levelNames) {
		return "???"
	}
	return levelNames[l]
}

// ToIslazy maps the Level to islazy.Verbosity (islazy does not have Tracing, so it will become DEBUG).
func ToIslazy(l Level) islazy.Verbosity {
	switch l {
	case LevelTracing, LevelDebug:
		return islazy.DEBUG
	case LevelInfo:
		return islazy.INFO
	case LevelImportant:
		return islazy.IMPORTANT
	case LevelWarning:
		return islazy.WARNING
	case LevelError:
		return islazy.ERROR
	case LevelFatal:
		return islazy.FATAL
	default:
		return islazy.INFO
	}
}

// FromIslazy — inverse transformation.

func FromIslazy(v islazy.Verbosity) Level {
	switch v {
	case islazy.DEBUG:
		return LevelDebug
	case islazy.INFO:
		return LevelInfo
	case islazy.IMPORTANT:
		return LevelImportant
	case islazy.WARNING:
		return LevelWarning
	case islazy.ERROR:
		return LevelError
	case islazy.FATAL:
		return LevelFatal
	default:
		return LevelInfo
	}
}

type Logger interface {
	Log(level Level, format string, args ...interface{})
}

type LoggerFunc func(level Level, format string, args ...interface{})

func (f LoggerFunc) Log(level Level, format string, args ...interface{}) {
	f(level, format, args...)
}

// NopLogger — Null Object.
type NopLogger struct{}

func (NopLogger) Log(Level, string, ...interface{}) {

}

// loggerBox is needed because atomic.Pointer stores a specific type,
// whereas an interface can hold various different implementations.
type loggerBox struct{ l Logger }

var active atomic.Pointer[loggerBox]

func init() { active.Store(&loggerBox{l: NopLogger{}}) }

// SetLogger replaces the backend. Can be safely called at any time from any goroutine.
func SetLogger(l Logger) {
	if l == nil {
		l = NopLogger{}
	}
	active.Store(&loggerBox{l: l})
}

func SetLogFunction(f func(level Level, format string, args ...interface{})) {
	if f == nil {
		SetLogger(nil)
		return
	}
	SetLogger(LoggerFunc(f))
}

func Active() Logger {
	return active.Load().l
}

type TraceEvent struct {
	Time    time.Time
	Message string // an already formatted message
	Caller  string // "file.go:42"
	Func    string // full function name
}

func (e TraceEvent) String() string {
	return fmt.Sprintf("%s [TRC] %s %s", e.Time.Format("15:04:05.000000"), e.Caller, e.Message)
}

// TracingSubscriber — Observer. The call is synchronous; the implementation must be fast and thread-safe.
type TracingSubscriber interface {
	OnTrace(event TraceEvent)
}

// SubscriberFunc — Adapter functions into the subscriber.
type SubscriberFunc func(event TraceEvent)

func (f SubscriberFunc) OnTrace(event TraceEvent) { f(event) }

// TracingToLogger — Adapter/bridge: forwards a trace to a Logger at a specified level,
// with an [TRC] prefix. Must be attached explicitly; traces are not automatically sent to the logger.

func TracingToLogger(l Logger, as Level) TracingSubscriber {
	if l == nil {
		l = NopLogger{}
	}
	return SubscriberFunc(func(e TraceEvent) {
		l.Log(as, "[TRC] %s %s", e.Caller, e.Message)
	})
}

type subscription struct {
	id  uint64
	sub TracingSubscriber
}

// traceHub — Subject. Copy-on-write: lock-free reads, mutex only for writes.

type traceHub struct {
	mu     sync.Mutex
	nextID uint64
	subs   atomic.Pointer[[]subscription]
}

var tracing = &traceHub{}

func (h *traceHub) enabled() bool {
	p := h.subs.Load()
	return p != nil && len(*p) > 0
}

func (h *traceHub) subscribe(s TracingSubscriber) (unsubscribe func()) {
	h.mu.Lock()
	h.nextID++
	id := h.nextID

	var next []subscription
	if p := h.subs.Load(); p != nil {
		next = make([]subscription, len(*p), len(*p)+1)
		copy(next, *p)
	}
	next = append(next, subscription{id: id, sub: s})
	h.subs.Store(&next)
	h.mu.Unlock()

	var once sync.Once
	return func() {
		once.Do(func() {
			h.mu.Lock()
			defer h.mu.Unlock()

			p := h.subs.Load()
			if p == nil {
				return
			}
			next := make([]subscription, 0, len(*p))
			for _, sc := range *p {
				if sc.id != id {
					next = append(next, sc)
				}
			}
			h.subs.Store(&next)
		})
	}
}

// publish constructs an event once. skip is the number of frames before the user code
// (all public functions call publish directly with skip=2).

func (h *traceHub) publish(skip int, format string, args ...interface{}) {
	p := h.subs.Load()
	if p == nil || len(*p) == 0 {
		return // fast path: no listeners, we aren't formatting anything
	}

	ev := TraceEvent{
		Time:    time.Now(),
		Message: fmt.Sprintf(format, args...),
		Caller:  "?",
	}
	if pc, file, line, ok := runtime.Caller(skip); ok {
		ev.Caller = fmt.Sprintf("%s:%d", filepath.Base(file), line)
		if fn := runtime.FuncForPC(pc); fn != nil {
			ev.Func = fn.Name()
		}
	}

	for _, sc := range *p { // the subscription procedure remains the same
		deliver(sc.sub, ev)
	}
}

// deliver isolates the process from a panic in a subscriber.

func deliver(s TracingSubscriber, e TraceEvent) {
	defer func() { _ = recover() }()
	s.OnTrace(e)
}

// SubscribeTracing returns an idempotent unsubscribe function.
func SubscribeTracing(s TracingSubscriber) (unsubscribe func()) {
	if s == nil {
		return func() {}
	}
	return tracing.subscribe(s)
}

// TracingEnabled allows you to avoid constructing expensive arguments when there are no listeners.
func TracingEnabled() bool {
	return tracing.enabled()
}

// Tracing publishes an event to subscribers.
func Tracing(format string, args ...interface{}) {
	tracing.publish(2, format, args...)
}

// TraceFunc traces entry and exit, including duration:
//
//	defer log.TraceFunc("Sniffer.Start")()
func TraceFunc(name string) (done func()) {
	if !tracing.enabled() {
		return func() {}
	}
	start := time.Now()
	tracing.publish(2, "→ %s", name)
	return func() {
		tracing.publish(2, "← %s (%s)", name, time.Since(start))
	}
}

// Log: traces are sent to subscribers, while other levels are directed to the active Logger.
func Log(level Level, format string, args ...interface{}) {
	if level <= LevelTracing {
		tracing.publish(2, format, args...)
		return
	}
	Active().Log(level, format, args...)
}

func Debug(format string, args ...interface{}) {
	Log(LevelDebug, format, args...)
}

func Info(format string, args ...interface{}) {
	Log(LevelInfo, format, args...)
}

func Important(format string, args ...interface{}) {
	Log(LevelImportant, format, args...)
}

func Warning(format string, args ...interface{}) {
	Log(LevelWarning, format, args...)
}

func Error(format string, args ...interface{}) {
	Log(LevelError, format, args...)
}

func Fatal(format string, args ...interface{}) {
	Log(LevelFatal, format, args...)
}

// IslazyLogger — an adapter for the global `islazy` functions (for standalone use).
func IslazyLogger() Logger {
	return LoggerFunc(func(level Level, format string, args ...interface{}) {
		switch level {
		case LevelTracing:
			islazy.Debug("[TRC] "+format, args...)
		case LevelDebug:
			islazy.Debug(format, args...)
		case LevelInfo:
			islazy.Info(format, args...)
		case LevelImportant:
			islazy.Important(format, args...)
		case LevelWarning:
			islazy.Warning(format, args...)
		case LevelError:
			islazy.Error(format, args...)
		case LevelFatal:
			islazy.Fatal(format, args...)
		}
	})
}

// Multi — Composite: sends the record to all backends (nil values ​​are discarded).
func Multi(loggers ...Logger) Logger {
	backends := make([]Logger, 0, len(loggers))
	for _, l := range loggers {
		if l != nil {
			backends = append(backends, l)
		}
	}
	return LoggerFunc(func(level Level, format string, args ...interface{}) {
		for _, l := range backends {
			l.Log(level, format, args...)
		}
	})
}

// WithMinLevel — Decorator: discards records below min.
func WithMinLevel(min Level, next Logger) Logger {
	if next == nil {
		next = NopLogger{}
	}
	return LoggerFunc(func(level Level, format string, args ...interface{}) {
		if level >= min {
			next.Log(level, format, args...)
		}
	})
}

// Setup Backend with filter level
// log.SetLogger(log.WithMinLevel(log.LevelInfo, log.IslazyLogger()))
// Subscribe Tracing (example, panel diagnostics in TUI):
// unsub := log.SubscribeTracing(log.SubscriberFunc(func(e log.TraceEvent) {
// 	ui.TracePane.Write(e.String())
// }))
// defer unsub()

// log.Tracing("handshake with %s started", addr)

// before
// log.Logger = func(level log.Verbosity, format string, args ...interface{}) { ... }

// after
// log.SetLogFunction(func(level log.Level, format string, args ...interface{}) { ... })

// Instead of log.Logger = s.Events.Log (where it is currently being assigned):
// log.SetLogger(log.WithMinLevel(log.LevelInfo, log.LoggerFunc(
// 	func(level log.Level, format string, args ...interface{}) {
// 		s.Events.Log(log.ToIslazy(level), format, args...)
// 	},
// )))

// Trace in TUI-panel:
// unsub := log.SubscribeTracing(log.SubscriberFunc(func(e log.TraceEvent) {
// 	ui.TracePane.Write(e.String())
// }))
// defer unsub()

// Trace to a standard log when the -trace flag is set:

// log.SubscribeTracing(log.TracingToLogger(log.Active(), log.LevelDebug))

// func (s *Sniffer) Start() error {
// 	defer log.TraceFunc("Sniffer.Start")()
// 	log.Tracing("opening %s", s.iface)
// 	...
// }
