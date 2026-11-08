package core

import (
	"fmt"
	"path/filepath"
	"runtime"
	"strings"
	"sync"
	"sync/atomic"
	"testing"
	"time"

	islazy "github.com/evilsocket/islazy/log"
)

// resetGlobals resets the package's global state before and after a test.
// Tests that use it must NOT call t.Parallel().
func resetGlobals(t *testing.T) {
	t.Helper()
	reset := func() {
		SetLogger(nil)
		tracing = &traceHub{}
	}
	reset()
	t.Cleanup(reset)
}

type record struct {
	Level Level
	Msg   string
}

// recorder — a thread-safe Logger that stores all entries.

type recorder struct {
	mu   sync.Mutex
	recs []record
}

func (r *recorder) Log(level Level, format string, args ...interface{}) {
	r.mu.Lock()
	defer r.mu.Unlock()
	r.recs = append(r.recs, record{Level: level, Msg: fmt.Sprintf(format, args...)})
}

func (r *recorder) records() []record {
	r.mu.Lock()
	defer r.mu.Unlock()
	return append([]record(nil), r.recs...)
}

// collector — a thread-safe TracingSubscriber that records all events.

type collector struct {
	mu     sync.Mutex
	events []TraceEvent
}

func (c *collector) OnTrace(e TraceEvent) {
	c.mu.Lock()
	defer c.mu.Unlock()
	c.events = append(c.events, e)
}

func (c *collector) snapshot() []TraceEvent {
	c.mu.Lock()
	defer c.mu.Unlock()
	return append([]TraceEvent(nil), c.events...)
}

func (c *collector) messages() []string {
	evs := c.snapshot()
	out := make([]string, len(evs))
	for i, e := range evs {
		out[i] = e.Message
	}
	return out
}

// nextLine returns "file.go:N+1", where N is the line number of the nextLine call.
// The call to the function being tested must be located strictly on the following line.

func nextLine() string {
	_, file, line, _ := runtime.Caller(1)
	return fmt.Sprintf("%s:%d", filepath.Base(file), line+1)
}

// mustFinish detects deadlocks without waiting for the overall go test timeout.

func mustFinish(t *testing.T, d time.Duration, fn func()) {
	t.Helper()
	done := make(chan struct{})
	go func() {
		defer close(done)
		fn()
	}()
	select {
	case <-done:
	case <-time.After(d):
		t.Fatal("timeout: возможный deadlock")
	}
}

func equalStrings(a, b []string) bool {
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

func TestLevel_String(t *testing.T) {
	cases := []struct {
		level Level
		want  string
	}{
		{LevelTracing, "TRC"},
		{LevelDebug, "DBG"},
		{LevelInfo, "INF"},
		{LevelImportant, "IMP"},
		{LevelWarning, "WRN"},
		{LevelError, "ERR"},
		{LevelFatal, "FTL"},
		{Level(-1), "???"},
		{Level(100), "???"},
	}
	for _, tc := range cases {
		if got := tc.level.String(); got != tc.want {
			t.Errorf("Level(%d).String() = %q, want %q", tc.level, got, tc.want)
		}
	}
}

func TestLevel_Ordering(t *testing.T) {
	ordered := []Level{LevelTracing, LevelDebug, LevelInfo, LevelImportant, LevelWarning, LevelError, LevelFatal}
	for i := 1; i < len(ordered); i++ {
		if ordered[i-1] >= ordered[i] {
			t.Errorf("%s должен быть ниже %s", ordered[i-1], ordered[i])
		}
	}
}

func TestToIslazy(t *testing.T) {
	cases := []struct {
		in   Level
		want islazy.Verbosity
	}{
		{LevelTracing, islazy.DEBUG}, // islazy does not have Tracing
		{LevelDebug, islazy.DEBUG},
		{LevelInfo, islazy.INFO},
		{LevelImportant, islazy.IMPORTANT},
		{LevelWarning, islazy.WARNING},
		{LevelError, islazy.ERROR},
		{LevelFatal, islazy.FATAL},
		{Level(100), islazy.INFO}, // unknown → INFO
		{Level(-1), islazy.INFO},
	}
	for _, tc := range cases {
		if got := ToIslazy(tc.in); got != tc.want {
			t.Errorf("ToIslazy(%s) = %v, want %v", tc.in, got, tc.want)
		}
	}
}

func TestFromIslazy(t *testing.T) {
	cases := []struct {
		in   islazy.Verbosity
		want Level
	}{
		{islazy.DEBUG, LevelDebug},
		{islazy.INFO, LevelInfo},
		{islazy.IMPORTANT, LevelImportant},
		{islazy.WARNING, LevelWarning},
		{islazy.ERROR, LevelError},
		{islazy.FATAL, LevelFatal},
		{islazy.Verbosity(100), LevelInfo}, // unknown → Info
	}
	for _, tc := range cases {
		if got := FromIslazy(tc.in); got != tc.want {
			t.Errorf("FromIslazy(%v) = %s, want %s", tc.in, got, tc.want)
		}
	}
}

func TestIslazyConversion_RoundTrip(t *testing.T) {
	for _, l := range []Level{LevelDebug, LevelInfo, LevelImportant, LevelWarning, LevelError, LevelFatal} {
		if got := FromIslazy(ToIslazy(l)); got != l {
			t.Errorf("round-trip(%s) = %s", l, got)
		}
	}
	// During conversion, "tracing" collapses into "debug.".
	if got := FromIslazy(ToIslazy(LevelTracing)); got != LevelDebug {
		t.Errorf("round-trip(Tracing) = %s, want DBG", got)
	}
}

func TestActive_DefaultIsNopLogger(t *testing.T) {
	resetGlobals(t)

	if _, ok := Active().(NopLogger); !ok {
		t.Fatalf("Active() = %T, want NopLogger", Active())
	}
	// There is no need to panic.
	Debug("x")
	Info("x")
	Important("x")
	Warning("x")
	Error("x")
	Fatal("x") // Fatal does not terminate the process itself.

}

func TestSetLogger(t *testing.T) {
	resetGlobals(t)

	rec := &recorder{}
	SetLogger(rec)
	if Active() != Logger(rec) {
		t.Fatal("Active() вернул не тот логгер, что был установлен")
	}

	SetLogger(nil)
	if _, ok := Active().(NopLogger); !ok {
		t.Fatalf("после SetLogger(nil) Active() = %T, want NopLogger", Active())
	}
}

func TestSetLogFunction(t *testing.T) {
	resetGlobals(t)

	var gotLevel Level
	var gotMsg string
	SetLogFunction(func(level Level, format string, args ...interface{}) {
		gotLevel = level
		gotMsg = fmt.Sprintf(format, args...)
	})

	Warning("disk %d%% full", 93)
	if gotLevel != LevelWarning || gotMsg != "disk 93% full" {
		t.Errorf("получили (%s, %q)", gotLevel, gotMsg)
	}

	SetLogFunction(nil)
	if _, ok := Active().(NopLogger); !ok {
		t.Errorf("после SetLogFunction(nil) Active() = %T, want NopLogger", Active())
	}
}

func TestLoggerFunc_ImplementsLogger(t *testing.T) {
	called := false
	var l Logger = LoggerFunc(func(Level, string, ...interface{}) { called = true })
	l.Log(LevelInfo, "x")
	if !called {
		t.Error("LoggerFunc не вызвала функцию")
	}
}

func TestHelpers_RouteToCorrectLevel(t *testing.T) {
	cases := []struct {
		name  string
		fn    func(string, ...interface{})
		level Level
	}{
		{"Debug", Debug, LevelDebug},
		{"Info", Info, LevelInfo},
		{"Important", Important, LevelImportant},
		{"Warning", Warning, LevelWarning},
		{"Error", Error, LevelError},
		{"Fatal", Fatal, LevelFatal},
	}

	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			resetGlobals(t)
			rec := &recorder{}
			SetLogger(rec)

			tc.fn("value=%d name=%s", 7, "bob")

			got := rec.records()
			if len(got) != 1 {
				t.Fatalf("записей: %d, want 1", len(got))
			}
			if got[0].Level != tc.level {
				t.Errorf("level = %s, want %s", got[0].Level, tc.level)
			}
			if got[0].Msg != "value=7 name=bob" {
				t.Errorf("msg = %q", got[0].Msg)
			}
		})
	}
}

func TestLog_TracingLevelNeverReachesLogger(t *testing.T) {
	resetGlobals(t)
	rec := &recorder{}
	SetLogger(rec)

	// No subscribers.

	Log(LevelTracing, "nobody listens")
	Tracing("nobody listens")

	// with subscribers.
	c := &collector{}
	SubscribeTracing(c)
	Log(LevelTracing, "listened %d", 1)
	Tracing("listened %d", 2)

	if n := len(rec.records()); n != 0 {
		t.Errorf("Logger получил %d trace-записей, want 0", n)
	}
	if got, want := c.messages(), []string{"listened 1", "listened 2"}; !equalStrings(got, want) {
		t.Errorf("подписчик получил %v, want %v", got, want)
	}
}

func TestLog_NegativeLevelTreatedAsTracing(t *testing.T) {
	resetGlobals(t)
	rec := &recorder{}
	SetLogger(rec)
	c := &collector{}
	SubscribeTracing(c)

	Log(Level(-5), "below tracing")

	if len(rec.records()) != 0 {
		t.Error("Logger не должен получать уровни ниже Tracing")
	}
	if got := c.messages(); len(got) != 1 || got[0] != "below tracing" {
		t.Errorf("подписчик получил %v", got)
	}
}

func TestSetLogger_ConcurrentWithLog(t *testing.T) {
	resetGlobals(t)

	var wg sync.WaitGroup
	for i := 0; i < 4; i++ {
		wg.Add(2)
		go func() {
			defer wg.Done()
			for j := 0; j < 500; j++ {
				SetLogger(&recorder{})
			}
		}()
		go func() {
			defer wg.Done()
			for j := 0; j < 500; j++ {
				Info("msg %d", j)
			}
		}()
	}
	wg.Wait() // The point of the test is the absence of races when the -race flag is used
}

// ═════════════════════════════════ Tracing ════════════════════════════════════

func TestTracing_DisabledWithoutSubscribers(t *testing.T) {
	resetGlobals(t)

	if TracingEnabled() {
		t.Fatal("TracingEnabled() = true without subscribe")
	}
	Tracing("nothing") // one should not panic

}

func TestTracing_FastPathDoesNotFormat(t *testing.T) {
	resetGlobals(t)

	var calls int32
	p := probe{calls: &calls}

	Tracing("%v", p)
	Log(LevelTracing, "%v", p)
	if n := atomic.LoadInt32(&calls); n != 0 {
		t.Fatalf("without subscribers, formatting was triggered %d times; expected 0", n)
	}
}

func TestTracing_FormatsOncePerEvent(t *testing.T) {
	resetGlobals(t)

	var calls int32
	SubscribeTracing(&collector{})
	SubscribeTracing(&collector{})
	SubscribeTracing(&collector{})

	Tracing("%v", probe{calls: &calls})

	if n := atomic.LoadInt32(&calls); n != 1 {
		t.Fatalf("String() called %d times, want 1 (one Sprintf per event)", n)
	}
}

// The probe keeps track of how many times it has been formatted.
type probe struct{ calls *int32 }

func (p probe) String() string {
	atomic.AddInt32(p.calls, 1)
	return "probe"
}

func TestTracing_DeliversEventWithMetadata(t *testing.T) {
	resetGlobals(t)
	c := &collector{}
	SubscribeTracing(c)

	before := time.Now()
	want := nextLine()
	Tracing("opening %s port %d", "eth0", 80)
	after := time.Now()

	evs := c.snapshot()
	if len(evs) != 1 {
		t.Fatalf("событий: %d, want 1", len(evs))
	}
	e := evs[0]

	if e.Message != "opening eth0 port 80" {
		t.Errorf("Message = %q", e.Message)
	}
	if e.Caller != want {
		t.Errorf("Caller = %q, want %q", e.Caller, want)
	}
	if !strings.Contains(e.Func, "TestTracing_DeliversEventWithMetadata") {
		t.Errorf("Func = %q, ожидалось имя теста", e.Func)
	}
	if e.Time.Before(before) || e.Time.After(after) {
		t.Errorf("Time = %v вне диапазона [%v, %v]", e.Time, before, after)
	}
}

func TestLog_TracingCallerPointsToUserCode(t *testing.T) {
	resetGlobals(t)
	c := &collector{}
	SubscribeTracing(c)

	want := nextLine()
	Log(LevelTracing, "via Log")

	evs := c.snapshot()
	if len(evs) != 1 || evs[0].Caller != want {
		t.Fatalf("события %+v, want Caller=%q", evs, want)
	}
}

func TestTracing_PreservesSubscriptionOrder(t *testing.T) {
	resetGlobals(t)

	var mu sync.Mutex
	var order []string
	mk := func(name string) TracingSubscriber {
		return SubscriberFunc(func(TraceEvent) {
			mu.Lock()
			order = append(order, name)
			mu.Unlock()
		})
	}
	for _, n := range []string{"A", "B", "C", "D", "E"} {
		SubscribeTracing(mk(n))
	}

	Tracing("x")

	if want := []string{"A", "B", "C", "D", "E"}; !equalStrings(order, want) {
		t.Errorf("порядок доставки %v, want %v", order, want)
	}
}

func TestTracing_Unsubscribe(t *testing.T) {
	resetGlobals(t)
	c := &collector{}
	unsub := SubscribeTracing(c)

	if !TracingEnabled() {
		t.Fatal("TracingEnabled() = false при активном подписчике")
	}

	Tracing("one")
	unsub()
	Tracing("two")

	if got := c.messages(); !equalStrings(got, []string{"one"}) {
		t.Errorf("получено %v, want [one]", got)
	}
	if TracingEnabled() {
		t.Error("TracingEnabled() = true после отписки единственного подписчика")
	}
}

func TestTracing_UnsubscribeIsIdempotent(t *testing.T) {
	resetGlobals(t)
	a, b := &collector{}, &collector{}
	unsubA := SubscribeTracing(a)
	SubscribeTracing(b)

	unsubA()
	unsubA()
	unsubA()

	Tracing("x")
	if len(a.snapshot()) != 0 {
		t.Error("The unsubscribed user received an event.")
	}
	if len(b.snapshot()) != 1 {
		t.Error("Repeated unsubscriptions broke things for the remaining subscribers.")
	}
}

func TestTracing_UnsubscribeMiddleKeepsOrder(t *testing.T) {
	resetGlobals(t)

	var order []string
	mk := func(name string) TracingSubscriber {
		return SubscriberFunc(func(TraceEvent) { order = append(order, name) })
	}
	SubscribeTracing(mk("A"))
	unsubB := SubscribeTracing(mk("B"))
	SubscribeTracing(mk("C"))

	unsubB()
	Tracing("x")

	if want := []string{"A", "C"}; !equalStrings(order, want) {
		t.Errorf("order = %v, want %v", order, want)
	}
}

func TestSubscribeTracing_NilSubscriber(t *testing.T) {
	resetGlobals(t)

	unsub := SubscribeTracing(nil)
	if unsub == nil {
		t.Fatal("SubscribeTracing(nil) returned nil instead of a no-op function")
	}
	unsub() // не должно паниковать
	if TracingEnabled() {
		t.Error("A nil-subscriber must not enable tracing.")
	}
}

func TestTracing_SameSubscriberTwice(t *testing.T) {
	resetGlobals(t)
	c := &collector{}
	unsub1 := SubscribeTracing(c)
	SubscribeTracing(c)

	Tracing("a")
	if n := len(c.snapshot()); n != 2 {
		t.Fatalf("after two event subscriptions: %d, want 2", n)
	}

	unsub1()
	Tracing("b")
	if n := len(c.snapshot()); n != 3 {
		t.Fatalf("after one unsubscription, events: %d, want 3", n)
	}
}

func TestTracing_PanickingSubscriberIsIsolated(t *testing.T) {
	resetGlobals(t)

	before, after := &collector{}, &collector{}
	SubscribeTracing(before)
	SubscribeTracing(SubscriberFunc(func(TraceEvent) { panic("boom") }))
	SubscribeTracing(after)

	func() {
		defer func() {
			if r := recover(); r != nil {
				t.Fatalf("The subscriber's panic reached the person being called: %v", r)
			}
		}()
		Tracing("x")
	}()

	if len(before.snapshot()) != 1 || len(after.snapshot()) != 1 {
		t.Error("The panic of one subscriber got in the way of the others")
	}
}

// Subscribing from within a handler must not lead to a deadlock,
// and a new subscriber does not receive the event that is currently being delivered.

func TestTracing_SubscribeDuringDelivery(t *testing.T) {
	resetGlobals(t)

	inner := &collector{}
	var once sync.Once
	SubscribeTracing(SubscriberFunc(func(TraceEvent) {
		once.Do(func() { SubscribeTracing(inner) })
	}))

	mustFinish(t, 2*time.Second, func() {
		Tracing("first")
		if n := len(inner.snapshot()); n != 0 {
			t.Errorf("a new subscriber received the current event (%d), want 0", n)
		}
		Tracing("second")
	})

	if got := inner.messages(); !equalStrings(got, []string{"second"}) {
		t.Errorf("inner received %v, want [second]", got)
	}
}

// Unsubscribing from within a handler: delivery is based on a snapshot, so the "victim"
// receives the current event, but will not receive any subsequent ones.
func TestTracing_UnsubscribeDuringDelivery(t *testing.T) {
	resetGlobals(t)

	victim := &collector{}
	var unsubVictim func()

	SubscribeTracing(SubscriberFunc(func(TraceEvent) {
		if unsubVictim != nil {
			unsubVictim()
		}
	}))
	unsubVictim = SubscribeTracing(victim)

	mustFinish(t, 2*time.Second, func() {
		Tracing("first")
		Tracing("second")
	})

	if got := victim.messages(); !equalStrings(got, []string{"first"}) {
		t.Errorf("victim получил %v, want [first]", got)
	}
}

func TestTracing_ConcurrentSubscribePublishUnsubscribe(t *testing.T) {
	resetGlobals(t)

	var received int64
	SubscribeTracing(SubscriberFunc(func(TraceEvent) { atomic.AddInt64(&received, 1) }))

	const goroutines, iterations = 8, 200
	var wg sync.WaitGroup
	for g := 0; g < goroutines; g++ {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for i := 0; i < iterations; i++ {
				unsub := SubscribeTracing(SubscriberFunc(func(TraceEvent) {}))
				Tracing("msg %d", i)
				unsub()
			}
		}()
	}
	wg.Wait()

	if got := atomic.LoadInt64(&received); got != goroutines*iterations {
		t.Errorf("persistent subscriber received %d events, expected %d", got, goroutines*iterations)
	}
	if p := tracing.subs.Load(); p == nil || len(*p) != 1 {
		t.Errorf("After all the unsubscribes, 1 subscriber should remain; there is %v", p)
	}
}

func TestTraceEvent_String(t *testing.T) {
	e := TraceEvent{
		Time:    time.Date(2024, 1, 2, 3, 4, 5, 678_000_000, time.UTC),
		Message: "hello",
		Caller:  "sniffer.go:42",
	}
	want := "03:04:05.678000 [TRC] sniffer.go:42 hello"
	if got := e.String(); got != want {
		t.Errorf("String() = %q, want %q", got, want)
	}
}

func TestTraceFunc_DisabledIsNoop(t *testing.T) {
	resetGlobals(t)

	done := TraceFunc("Noop")
	if done == nil {
		t.Fatal("TraceFunc вернула nil")
	}
	done() //one should not panic
}

// If tracing was disabled at the input, the output remains silent as well,
// even if a subscriber appears while the function is executing.
func TestTraceFunc_DisabledAtEntryStaysSilent(t *testing.T) {
	resetGlobals(t)

	done := TraceFunc("Late")
	c := &collector{}
	SubscribeTracing(c)
	done()

	if n := len(c.snapshot()); n != 0 {
		t.Errorf("получено %d событий, want 0", n)
	}
}

func TestTraceFunc_EmitsEnterAndExitWithDuration(t *testing.T) {
	resetGlobals(t)
	c := &collector{}
	SubscribeTracing(c)

	want := nextLine()
	done := TraceFunc("Sniffer.Start")
	time.Sleep(5 * time.Millisecond)
	done()

	evs := c.snapshot()
	if len(evs) != 2 {
		t.Fatalf("событий: %d, want 2", len(evs))
	}

	if evs[0].Message != "→ Sniffer.Start" {
		t.Errorf("вход: %q", evs[0].Message)
	}
	if evs[0].Caller != want {
		t.Errorf("Caller входа = %q, want %q", evs[0].Caller, want)
	}

	exit := evs[1].Message
	if !strings.HasPrefix(exit, "← Sniffer.Start (") || !strings.HasSuffix(exit, ")") {
		t.Fatalf("выход: %q", exit)
	}
	raw := strings.TrimSuffix(strings.TrimPrefix(exit, "← Sniffer.Start ("), ")")
	d, err := time.ParseDuration(raw)
	if err != nil {
		t.Fatalf("failed to parse duration %q: %v", raw, err)
	}
	if d < 5*time.Millisecond {
		t.Errorf("длительность %v < 5ms", d)
	}
}

func tracedFunction() {
	defer TraceFunc("tracedFunction")()
	Tracing("inside")
}

func TestTraceFunc_WithDefer(t *testing.T) {
	resetGlobals(t)
	c := &collector{}
	SubscribeTracing(c)

	tracedFunction()

	evs := c.snapshot()
	if len(evs) != 3 {
		t.Fatalf("events: %d, want 3", len(evs))
	}
	if evs[0].Message != "→ tracedFunction" ||
		evs[1].Message != "inside" ||
		!strings.HasPrefix(evs[2].Message, "← tracedFunction (") {
		t.Errorf("incorrect sequence: %v", c.messages())
	}
	if !strings.Contains(evs[0].Func, "tracedFunction") {
		t.Errorf("Func = %q", evs[0].Func)
	}
}

func TestTracingToLogger_ForwardsWithPrefixAndLevel(t *testing.T) {
	resetGlobals(t)
	rec := &recorder{}
	SubscribeTracing(TracingToLogger(rec, LevelDebug))

	Tracing("handshake with %s", "10.0.0.1")

	got := rec.records()
	if len(got) != 1 {
		t.Fatalf("записей: %d, want 1", len(got))
	}
	if got[0].Level != LevelDebug {
		t.Errorf("level = %s, want DBG", got[0].Level)
	}
	if !strings.HasPrefix(got[0].Msg, "[TRC] log_test.go:") ||
		!strings.HasSuffix(got[0].Msg, " handshake with 10.0.0.1") {
		t.Errorf("msg = %q", got[0].Msg)
	}
}

func TestTracingToLogger_NilLogger(t *testing.T) {
	resetGlobals(t)
	SubscribeTracing(TracingToLogger(nil, LevelInfo))
	Tracing("x") // one should not panic

}

func TestWithMinLevel_FiltersBelowMin(t *testing.T) {
	rec := &recorder{}
	l := WithMinLevel(LevelWarning, rec)

	for _, lvl := range []Level{LevelDebug, LevelInfo, LevelImportant, LevelWarning, LevelError, LevelFatal} {
		l.Log(lvl, "msg")
	}

	got := rec.records()
	want := []Level{LevelWarning, LevelError, LevelFatal}
	if len(got) != len(want) {
		t.Fatalf("записей: %d, want %d (%v)", len(got), len(want), got)
	}
	for i := range want {
		if got[i].Level != want[i] {
			t.Errorf("[%d] level = %s, want %s", i, got[i].Level, want[i])
		}
	}
}

func TestWithMinLevel_NilNext(t *testing.T) {
	l := WithMinLevel(LevelInfo, nil)
	if l == nil {
		t.Fatal("WithMinLevel(nil) вернул nil")
	}
	l.Log(LevelError, "x") // one should not panic

}

func TestWithMinLevel_IntegratesWithGlobalLog(t *testing.T) {
	resetGlobals(t)
	rec := &recorder{}
	SetLogger(WithMinLevel(LevelInfo, rec))

	Debug("hidden")
	Info("shown")
	Error("shown too")

	got := rec.records()
	if len(got) != 2 || got[0].Msg != "shown" || got[1].Msg != "shown too" {
		t.Errorf("records = %v", got)
	}
}

func TestMulti_FansOutInOrder(t *testing.T) {
	var order []string
	mk := func(name string) Logger {
		return LoggerFunc(func(Level, string, ...interface{}) { order = append(order, name) })
	}
	l := Multi(mk("A"), nil, mk("B"), nil, mk("C"))

	l.Log(LevelInfo, "x")

	if want := []string{"A", "B", "C"}; !equalStrings(order, want) {
		t.Errorf("order = %v, want %v", order, want)
	}
}

func TestMulti_DeliversSameRecordToAll(t *testing.T) {
	a, b := &recorder{}, &recorder{}
	Multi(a, b).Log(LevelError, "code=%d", 500)

	for name, r := range map[string]*recorder{"a": a, "b": b} {
		got := r.records()
		if len(got) != 1 || got[0].Level != LevelError || got[0].Msg != "code=500" {
			t.Errorf("%s: %v", name, got)
		}
	}
}

func TestMulti_Empty(t *testing.T) {
	Multi().Log(LevelInfo, "x")
	Multi(nil, nil).Log(LevelInfo, "x")
}

func TestMulti_ComposesWithDecorator(t *testing.T) {
	all, errs := &recorder{}, &recorder{}
	l := Multi(all, WithMinLevel(LevelError, errs))

	l.Log(LevelInfo, "i")
	l.Log(LevelError, "e")

	if n := len(all.records()); n != 2 {
		t.Errorf("all: %d, want 2", n)
	}
	if got := errs.records(); len(got) != 1 || got[0].Msg != "e" {
		t.Errorf("errs: %v", got)
	}
}

// Smoke test for the islazy adapter. Fatal is intentionally not called:
// islazy.Fatal terminates the process via os.Exit.
func TestIslazyLogger_DoesNotPanic(t *testing.T) {
	l := IslazyLogger()
	if l == nil {
		t.Fatal("IslazyLogger() вернул nil")
	}
	for _, lvl := range []Level{LevelTracing, LevelDebug, LevelInfo, LevelImportant, LevelWarning, LevelError} {
		l.Log(lvl, "smoke %s", lvl)
	}
}

func BenchmarkTracing_NoSubscribers(b *testing.B) {
	tracing = &traceHub{}
	b.ReportAllocs()
	for i := 0; i < b.N; i++ {
		Tracing("packet %d from %s", i, "10.0.0.1")
	}
}

func BenchmarkTracing_OneSubscriber(b *testing.B) {
	tracing = &traceHub{}
	unsub := SubscribeTracing(SubscriberFunc(func(TraceEvent) {}))
	defer unsub()

	b.ReportAllocs()
	for i := 0; i < b.N; i++ {
		Tracing("packet %d from %s", i, "10.0.0.1")
	}
}

func BenchmarkTracing_NoSubscribersParallel(b *testing.B) {
	tracing = &traceHub{}
	b.ReportAllocs()
	b.RunParallel(func(pb *testing.PB) {
		for pb.Next() {
			Tracing("packet")
		}
	})
}

func BenchmarkTraceFunc_Disabled(b *testing.B) {
	tracing = &traceHub{}
	b.ReportAllocs()
	for i := 0; i < b.N; i++ {
		TraceFunc("Sniffer.Start")()
	}
}

func BenchmarkLog_NopLogger(b *testing.B) {
	SetLogger(nil)
	b.ReportAllocs()
	for i := 0; i < b.N; i++ {
		Info("hello %d", i)
	}
}

func BenchmarkLog_ParallelWithSetLogger(b *testing.B) {
	SetLogger(NopLogger{})
	b.ReportAllocs()
	b.RunParallel(func(pb *testing.PB) {
		for pb.Next() {
			Info("hello")
		}
	})
}
