package core

import (
	"context"
	"errors"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"reflect"
	"strconv"
	"strings"
	"sync"
	"testing"
	"time"
)

// ═══════════════════════════ Helper process (portable) ════════════════════════
//
// Real-process tests re-execute the test binary itself, so they do not depend
// on echo/sleep/sh being installed and work the same on Linux, macOS and Windows.

const helperEnv = "CORE_TEST_HELPER_PROCESS"

// TestHelperProcess is not a real test: it is the "program" that the tests
// below run. Without helperEnv set it does nothing.
func TestHelperProcess(t *testing.T) {
	if os.Getenv(helperEnv) != "1" {
		return
	}

	args := os.Args
	for i, a := range args {
		if a == "--" {
			args = args[i+1:]
			break
		}
	}
	if len(args) == 0 {
		os.Exit(64)
	}

	switch args[0] {
	case "echo":
		fmt.Print(strings.Join(args[1:], " "))
	case "padded":
		fmt.Print("  padded  \n\n")
	case "both":
		fmt.Fprintln(os.Stdout, "from-stdout")
		fmt.Fprintln(os.Stderr, "from-stderr")
	case "exit":
		code, _ := strconv.Atoi(args[1])
		fmt.Print("failing output")
		os.Exit(code)
	case "sleep":
		d, _ := time.ParseDuration(args[1])
		time.Sleep(d)
	case "pwd":
		wd, _ := os.Getwd()
		fmt.Print(wd)
	case "env":
		fmt.Print(os.Getenv(args[1]))
	}
	os.Exit(0)
}

func helperExe(t *testing.T) string {
	t.Helper()
	exe, err := os.Executable()
	if err != nil {
		t.Skipf("cannot locate the test binary: %v", err)
	}
	return exe
}

func helperArgs(mode string, rest ...string) []string {
	return append([]string{"-test.run=^TestHelperProcess$", "--", mode}, rest...)
}

// helperExecutor always sets Env explicitly: without helperEnv the child
// would run the whole test suite output ("PASS") instead of the helper.
func helperExecutor(extraEnv ...string) LocalExecutor {
	env := append(os.Environ(), helperEnv+"=1")
	return LocalExecutor{Env: append(env, extraEnv...)}
}

// ═══════════════════════════════ Test doubles ═════════════════════════════════

// stubExecutor is a configurable Executor mock.
type stubExecutor struct {
	lookPath func(string) (string, error)
	run      func(ctx context.Context, exe string, args ...string) (Result, error)
}

func (s stubExecutor) LookPath(exe string) (string, error) {
	if s.lookPath != nil {
		return s.lookPath(exe)
	}
	return exe, nil
}

func (s stubExecutor) Run(ctx context.Context, exe string, args ...string) (Result, error) {
	if s.run != nil {
		return s.run(ctx, exe, args...)
	}
	return Result{}, nil
}

// withExecutor installs e for the duration of the test (nil = LocalExecutor).
func withExecutor(t *testing.T, e IExecutor) {
	t.Helper()
	t.Cleanup(UseExecutor(e))
}

// lateErrContext never fires Done() (so os/exec does not kill the process)
// but reports an expired deadline from Err(). It simulates a deadline that
// expires right after the command has finished.
type lateErrContext struct{ context.Context }

func (lateErrContext) Err() error { return context.DeadlineExceeded }

// ═════════════════════════════════ Result ═════════════════════════════════════

func TestResult_OK(t *testing.T) {
	cases := []struct {
		code int
		want bool
	}{{0, true}, {1, false}, {-1, false}, {255, false}}

	for _, tc := range cases {
		if got := (Result{ExitCode: tc.code}).OK(); got != tc.want {
			t.Errorf("Result{ExitCode:%d}.OK() = %v, want %v", tc.code, got, tc.want)
		}
	}
}

// ═══════════════════════════════ ExecutorFunc ═════════════════════════════════

func TestExecutorFunc_LookPathAcceptsEverything(t *testing.T) {
	f := ExecutorFunc(func(context.Context, string, ...string) (Result, error) { return Result{}, nil })

	path, err := f.LookPath("whatever")
	if err != nil || path != "whatever" {
		t.Errorf("LookPath = (%q, %v), want (whatever, nil)", path, err)
	}
}

func TestExecutorFunc_RunDelegates(t *testing.T) {
	type ctxKey struct{}
	boom := errors.New("boom")

	f := ExecutorFunc(func(ctx context.Context, exe string, args ...string) (Result, error) {
		if ctx.Value(ctxKey{}) != "marker" || exe != "tool" || !reflect.DeepEqual(args, []string{"a", "b"}) {
			t.Errorf("unexpected arguments: %v %q %v", ctx.Value(ctxKey{}), exe, args)
		}
		return Result{Output: "out", ExitCode: 4}, boom
	})

	ctx := context.WithValue(context.Background(), ctxKey{}, "marker")
	res, err := f.Run(ctx, "tool", "a", "b")

	if !errors.Is(err, boom) || res.Output != "out" || res.ExitCode != 4 {
		t.Errorf("Run = (%+v, %v)", res, err)
	}
}

// ═════════════════════════════ LocalExecutor.LookPath ═════════════════════════

func TestLocalExecutor_LookPath(t *testing.T) {
	exe := helperExe(t)

	path, err := LocalExecutor{}.LookPath(exe)
	if err != nil || path == "" {
		t.Errorf("LookPath(%q) = (%q, %v)", exe, path, err)
	}

	if _, err := (LocalExecutor{}).LookPath("azula-no-such-binary-xyz"); err == nil {
		t.Error("expected an error for a missing binary")
	}
}

// ═══════════════════════════════ LocalExecutor.Run ════════════════════════════

func TestLocalExecutor_Run_Success(t *testing.T) {
	exe := helperExe(t)
	args := helperArgs("echo", "hello", "world")

	res, err := helperExecutor().Run(context.Background(), exe, args...)
	if err != nil {
		t.Fatalf("unexpected error: %v", err)
	}
	if res.Output != "hello world" {
		t.Errorf("Output = %q", res.Output)
	}
	if !res.OK() || res.ExitCode != 0 {
		t.Errorf("ExitCode = %d, OK = %v", res.ExitCode, res.OK())
	}
	if filepath.Base(res.Path) != filepath.Base(exe) {
		t.Errorf("Path = %q, want the resolved %q", res.Path, exe)
	}
	if !reflect.DeepEqual(res.Args, args) {
		t.Errorf("Args = %v, want %v", res.Args, args)
	}
	if res.Duration <= 0 {
		t.Errorf("Duration = %v, want > 0", res.Duration)
	}
}

func TestLocalExecutor_Run_TrimsOutput(t *testing.T) {
	res, err := helperExecutor().Run(context.Background(), helperExe(t), helperArgs("padded")...)
	if err != nil || res.Output != "padded" {
		t.Errorf("Output = %q, err = %v", res.Output, err)
	}
}

func TestLocalExecutor_Run_CombinesStdoutAndStderr(t *testing.T) {
	res, err := helperExecutor().Run(context.Background(), helperExe(t), helperArgs("both")...)
	if err != nil {
		t.Fatal(err)
	}
	if !strings.Contains(res.Output, "from-stdout") || !strings.Contains(res.Output, "from-stderr") {
		t.Errorf("Output = %q, want both streams", res.Output)
	}
}

func TestLocalExecutor_Run_NonZeroExit(t *testing.T) {
	res, err := helperExecutor().Run(context.Background(), helperExe(t), helperArgs("exit", "3")...)

	var exitErr *exec.ExitError
	if !errors.As(err, &exitErr) {
		t.Fatalf("err = %v, want *exec.ExitError", err)
	}
	if res.ExitCode != 3 || res.OK() {
		t.Errorf("ExitCode = %d, OK = %v", res.ExitCode, res.OK())
	}
	// Output is kept on failure: it usually contains the command's own error text.
	if res.Output != "failing output" {
		t.Errorf("Output = %q", res.Output)
	}
}

// Regression (bug #5): a command that never started had ExitCode 0 and OK() == true.
func TestLocalExecutor_Run_NotFound(t *testing.T) {
	logs := captureLogs(t)
	args := []string{"a", "b"}

	res, err := LocalExecutor{}.Run(context.Background(), "azula-no-such-binary-xyz", args...)

	if err == nil {
		t.Fatal("expected an error")
	}
	if res.ExitCode != -1 || res.OK() {
		t.Errorf("ExitCode = %d, OK = %v; a command that never started must not be OK", res.ExitCode, res.OK())
	}
	if res.Path != "" || res.Output != "" {
		t.Errorf("Path/Output must be empty: %+v", res)
	}
	if !reflect.DeepEqual(res.Args, args) {
		t.Errorf("Args = %v, want %v", res.Args, args)
	}
	if !hasLog(logs(), LevelWarning, "executable azula-no-such-binary-xyz not found in $PATH") {
		t.Errorf("missing warning, logs: %+v", logs())
	}
}

// Regression (bug #9): Result.Args aliased the caller's slice.
func TestLocalExecutor_Run_ArgsAreCopied(t *testing.T) {
	t.Run("not found", func(t *testing.T) {
		args := []string{"original"}
		res, _ := LocalExecutor{}.Run(context.Background(), "azula-no-such-binary-xyz", args...)

		args[0] = "mutated"
		if res.Args[0] != "original" {
			t.Errorf("Result.Args aliases the caller's slice: %v", res.Args)
		}
	})
	t.Run("success", func(t *testing.T) {
		args := helperArgs("echo", "x")
		want := append([]string(nil), args...)

		res, err := helperExecutor().Run(context.Background(), helperExe(t), args...)
		if err != nil {
			t.Fatal(err)
		}

		args[len(args)-1] = "mutated"
		if !reflect.DeepEqual(res.Args, want) {
			t.Errorf("Result.Args aliases the caller's slice: %v", res.Args)
		}
	})
}

func TestLocalExecutor_Run_Timeout(t *testing.T) {
	ex := helperExecutor()
	ex.Timeout = 150 * time.Millisecond

	start := time.Now()
	res, err := ex.Run(context.Background(), helperExe(t), helperArgs("sleep", "10s")...)
	elapsed := time.Since(start)

	if !errors.Is(err, context.DeadlineExceeded) {
		t.Errorf("err = %v, want DeadlineExceeded", err)
	}
	if elapsed > 5*time.Second {
		t.Errorf("took %v: the process was not killed by the timeout", elapsed)
	}
	if res.OK() {
		t.Error("a killed command must not be OK")
	}
}

func TestLocalExecutor_Run_ContextCancellation(t *testing.T) {
	ctx, cancel := context.WithCancel(context.Background())
	go func() {
		time.Sleep(150 * time.Millisecond)
		cancel()
	}()

	start := time.Now()
	_, err := helperExecutor().Run(ctx, helperExe(t), helperArgs("sleep", "10s")...)

	if !errors.Is(err, context.Canceled) {
		t.Errorf("err = %v, want Canceled", err)
	}
	if time.Since(start) > 5*time.Second {
		t.Error("the process was not killed on cancel")
	}
}

func TestLocalExecutor_Run_TimeoutDoesNotAffectFastCommands(t *testing.T) {
	ex := helperExecutor()
	ex.Timeout = 30 * time.Second

	res, err := ex.Run(context.Background(), helperExe(t), helperArgs("echo", "fast")...)
	if err != nil || res.Output != "fast" {
		t.Errorf("Output = %q, err = %v", res.Output, err)
	}
}

func TestLocalExecutor_Run_NonPositiveTimeoutMeansNoLimit(t *testing.T) {
	for _, d := range []time.Duration{0, -time.Second} {
		ex := helperExecutor()
		ex.Timeout = d

		res, err := ex.Run(context.Background(), helperExe(t), helperArgs("echo", "ok")...)
		if err != nil || res.Output != "ok" {
			t.Errorf("Timeout=%v: Output = %q, err = %v", d, res.Output, err)
		}
	}
}

// Regression (bug #6): ctx.Err() was checked even after a successful run, so a
// deadline that expired at the moment of completion turned success into error.
func TestLocalExecutor_Run_SuccessIsNotTurnedIntoContextError(t *testing.T) {
	res, err := helperExecutor().Run(lateErrContext{context.Background()}, helperExe(t), helperArgs("echo", "done")...)

	if err != nil {
		t.Fatalf("successful command returned %v", err)
	}
	if res.Output != "done" || !res.OK() {
		t.Errorf("res = %+v", res)
	}
}

// The other half of the rule: when the command failed AND the context is done,
// the context error is the better explanation.
func TestLocalExecutor_Run_FailureWithDoneContextReportsContextError(t *testing.T) {
	_, err := helperExecutor().Run(lateErrContext{context.Background()}, helperExe(t), helperArgs("exit", "2")...)

	if !errors.Is(err, context.DeadlineExceeded) {
		t.Errorf("err = %v, want DeadlineExceeded", err)
	}
}

func TestLocalExecutor_Run_Dir(t *testing.T) {
	dir := t.TempDir()
	ex := helperExecutor()
	ex.Dir = dir

	res, err := ex.Run(context.Background(), helperExe(t), helperArgs("pwd")...)
	if err != nil {
		t.Fatal(err)
	}

	// Resolve symlinks: /var -> /private/var on macOS, 8.3 names on Windows.
	want, _ := filepath.EvalSymlinks(dir)
	got, _ := filepath.EvalSymlinks(res.Output)
	if got != want {
		t.Errorf("working dir = %q, want %q", got, want)
	}
}

func TestLocalExecutor_Run_EnvReplacesEnvironment(t *testing.T) {
	t.Setenv("CORE_TEST_PARENT_ONLY", "leaked")

	t.Run("custom variable is passed", func(t *testing.T) {
		ex := helperExecutor("CORE_TEST_CUSTOM=custom-value")
		res, err := ex.Run(context.Background(), helperExe(t), helperArgs("env", "CORE_TEST_CUSTOM")...)
		if err != nil || res.Output != "custom-value" {
			t.Errorf("Output = %q, err = %v", res.Output, err)
		}
	})

	t.Run("parent environment is not inherited", func(t *testing.T) {
		ex := LocalExecutor{Env: []string{helperEnv + "=1"}} // no os.Environ()
		res, err := ex.Run(context.Background(), helperExe(t), helperArgs("env", "CORE_TEST_PARENT_ONLY")...)
		if err != nil || res.Output != "" {
			t.Errorf("parent variable leaked: Output = %q, err = %v", res.Output, err)
		}
	})
}

func TestLocalExecutor_Run_LogsDebug(t *testing.T) {
	logs := captureLogs(t)

	_, err := helperExecutor().Run(context.Background(), helperExe(t), helperArgs("echo", "dbg")...)
	if err != nil {
		t.Fatal(err)
	}

	if !hasLog(logs(), LevelDebug, "exec=", "exit=0", "out=dbg") {
		t.Errorf("missing debug record: %+v", logs())
	}
}

func TestLocalExecutor_Run_EmitsTraceEnterResolveExit(t *testing.T) {
	trace := captureTrace(t)

	if _, err := helperExecutor().Run(context.Background(), helperExe(t), helperArgs("echo", "x")...); err != nil {
		t.Fatal(err)
	}

	msgs := trace()
	if len(msgs) != 3 {
		t.Fatalf("events = %d, want 3: %q", len(msgs), msgs)
	}
	if msgs[0] != "→ core.LocalExecutor.Run" {
		t.Errorf("enter: %q", msgs[0])
	}
	if !strings.HasPrefix(msgs[1], "resolved ") {
		t.Errorf("resolve: %q", msgs[1])
	}
	if !strings.HasPrefix(msgs[2], "← core.LocalExecutor.Run (") {
		t.Errorf("exit: %q", msgs[2])
	}
}

func TestLocalExecutor_Run_NotFoundStillClosesTraceScope(t *testing.T) {
	captureLogs(t)
	trace := captureTrace(t)

	_, _ = LocalExecutor{}.Run(context.Background(), "azula-no-such-binary-xyz")

	msgs := trace()
	if len(msgs) != 2 || msgs[0] != "→ core.LocalExecutor.Run" || !strings.HasPrefix(msgs[1], "← core.LocalExecutor.Run (") {
		t.Errorf("events: %q", msgs)
	}
}

func TestLocalExecutor_Run_ConcurrentUse(t *testing.T) {
	exe := helperExe(t)
	ex := helperExecutor()

	var wg sync.WaitGroup
	for i := 0; i < 4; i++ {
		wg.Add(1)
		go func(i int) {
			defer wg.Done()
			want := fmt.Sprintf("worker-%d", i)
			res, err := ex.Run(context.Background(), exe, helperArgs("echo", want)...)
			if err != nil || res.Output != want {
				t.Errorf("worker %d: Output = %q, err = %v", i, res.Output, err)
			}
		}(i)
	}
	wg.Wait()
}

func TestExitCodeOf(t *testing.T) {
	t.Run("no process state, no error", func(t *testing.T) {
		if got := exitCodeOf(&exec.Cmd{}, nil); got != 0 {
			t.Errorf("got %d, want 0", got)
		}
	})
	t.Run("no process state, with error", func(t *testing.T) {
		if got := exitCodeOf(&exec.Cmd{}, errors.New("start failed")); got != -1 {
			t.Errorf("got %d, want -1", got)
		}
	})
	t.Run("real exit code", func(t *testing.T) {
		cmd := exec.Command(helperExe(t), helperArgs("exit", "7")...)
		cmd.Env = append(os.Environ(), helperEnv+"=1")
		err := cmd.Run()

		if got := exitCodeOf(cmd, err); got != 7 {
			t.Errorf("got %d, want 7", got)
		}
	})
}

// ═══════════════════════════ Facade: executor switching ═══════════════════════

func TestDefaultExecutor_IsLocalExecutorByDefault(t *testing.T) {
	withExecutor(t, nil)

	if _, ok := DefaultExecutor().(LocalExecutor); !ok {
		t.Errorf("DefaultExecutor() = %T, want LocalExecutor", DefaultExecutor())
	}
}

func TestUseExecutor_RestoreReturnsPrevious(t *testing.T) {
	first := stubExecutor{lookPath: func(string) (string, error) { return "first", nil }}
	second := stubExecutor{lookPath: func(string) (string, error) { return "second", nil }}

	restoreFirst := UseExecutor(first)
	defer restoreFirst()

	restoreSecond := UseExecutor(second)
	if p, _ := DefaultExecutor().LookPath("x"); p != "second" {
		t.Errorf("active = %q, want second", p)
	}

	restoreSecond()
	if p, _ := DefaultExecutor().LookPath("x"); p != "first" {
		t.Errorf("after restore active = %q, want first", p)
	}
}

func TestUseExecutor_NilInstallsLocalExecutor(t *testing.T) {
	withExecutor(t, stubExecutor{})

	restore := UseExecutor(nil)
	defer restore()

	if _, ok := DefaultExecutor().(LocalExecutor); !ok {
		t.Errorf("DefaultExecutor() = %T, want LocalExecutor", DefaultExecutor())
	}
}

func TestSetExecutor(t *testing.T) {
	withExecutor(t, nil) // restores the original on cleanup

	SetExecutor(stubExecutor{lookPath: func(string) (string, error) { return "stub", nil }})
	if p, _ := DefaultExecutor().LookPath("x"); p != "stub" {
		t.Errorf("active = %q, want stub", p)
	}

	SetExecutor(nil)
	if _, ok := DefaultExecutor().(LocalExecutor); !ok {
		t.Errorf("SetExecutor(nil): DefaultExecutor() = %T", DefaultExecutor())
	}
}

func TestExecutor_ConcurrentSwitchingAndUse(t *testing.T) {
	withExecutor(t, stubExecutor{})

	var wg sync.WaitGroup
	for i := 0; i < 4; i++ {
		wg.Add(2)
		go func() {
			defer wg.Done()
			for j := 0; j < 200; j++ {
				UseExecutor(stubExecutor{})()
			}
		}()
		go func() {
			defer wg.Done()
			for j := 0; j < 200; j++ {
				_ = HasBinary("x")
				_ = DefaultExecutor()
			}
		}()
	}
	wg.Wait() // the point is the absence of data races under -race
}

// ════════════════════════════════ Facade: HasBinary ═══════════════════════════

func TestHasBinary(t *testing.T) {
	withExecutor(t, stubExecutor{lookPath: func(name string) (string, error) {
		switch name {
		case "found":
			return "/usr/bin/found", nil
		case "empty-path":
			return "", nil // no error, but also no path
		default:
			return "", &exec.Error{Name: name, Err: exec.ErrNotFound}
		}
	}})

	if !HasBinary("found") {
		t.Error("HasBinary(found) = false")
	}
	if HasBinary("missing") {
		t.Error("HasBinary(missing) = true")
	}
	if HasBinary("empty-path") {
		t.Error("an empty path must count as not found")
	}
}

func TestHasBinary_UsesOnlyPathResolution(t *testing.T) {
	withExecutor(t, stubExecutor{
		lookPath: func(string) (string, error) { return "/bin/x", nil },
		run: func(context.Context, string, ...string) (Result, error) {
			t.Error("HasBinary must never run a command")
			return Result{}, nil
		},
	})

	HasBinary("x")
}

func TestHasBinary_EmitsTrace(t *testing.T) {
	withExecutor(t, stubExecutor{lookPath: func(string) (string, error) { return "/usr/bin/nmap", nil }})
	trace := captureTrace(t)

	HasBinary("nmap")

	msgs := trace()
	if len(msgs) != 1 || !strings.Contains(msgs[0], `HasBinary("nmap")`) || !strings.Contains(msgs[0], "found=true") {
		t.Errorf("trace: %q", msgs)
	}
}

// ═══════════════════════════ Facade: Exec / ExecContext ═══════════════════════

func TestExec_ReturnsOutput(t *testing.T) {
	var gotExe string
	var gotArgs []string
	withExecutor(t, ExecutorFunc(func(_ context.Context, exe string, args ...string) (Result, error) {
		gotExe, gotArgs = exe, args
		return Result{Output: "hello"}, nil
	}))

	out, err := Exec("tool", []string{"-a", "b c"})

	if err != nil || out != "hello" {
		t.Errorf("Exec = (%q, %v)", out, err)
	}
	if gotExe != "tool" || !reflect.DeepEqual(gotArgs, []string{"-a", "b c"}) {
		t.Errorf("executor got %q %v", gotExe, gotArgs)
	}
}

func TestExec_NilArgs(t *testing.T) {
	var gotArgs []string
	withExecutor(t, ExecutorFunc(func(_ context.Context, _ string, args ...string) (Result, error) {
		gotArgs = args
		return Result{}, nil
	}))

	if _, err := Exec("tool", nil); err != nil {
		t.Fatal(err)
	}
	if len(gotArgs) != 0 {
		t.Errorf("args = %v, want none", gotArgs)
	}
}

func TestExec_OutputIsReturnedTogetherWithError(t *testing.T) {
	boom := errors.New("exit status 2")
	withExecutor(t, ExecutorFunc(func(context.Context, string, ...string) (Result, error) {
		return Result{Output: "permission denied", ExitCode: 2}, boom
	}))

	out, err := Exec("tool", nil)

	if !errors.Is(err, boom) || out != "permission denied" {
		t.Errorf("Exec = (%q, %v)", out, err)
	}
}

func TestExecContext_PassesContextAndReturnsFullResult(t *testing.T) {
	type ctxKey struct{}
	want := Result{Path: "/bin/t", Args: []string{"x"}, Output: "o", ExitCode: 5, Duration: time.Second}

	withExecutor(t, ExecutorFunc(func(ctx context.Context, exe string, args ...string) (Result, error) {
		if ctx.Value(ctxKey{}) != "marker" {
			t.Error("context was not propagated")
		}
		return want, nil
	}))

	ctx := context.WithValue(context.Background(), ctxKey{}, "marker")
	got, err := ExecContext(ctx, "t", "x")

	if err != nil || !reflect.DeepEqual(got, want) {
		t.Errorf("ExecContext = (%+v, %v)", got, err)
	}
}

// ═════════════════════════════ Facade: Shell / ShellContext ═══════════════════

func TestShell_UsesPlatformShell(t *testing.T) {
	var gotExe string
	var gotArgs []string
	withExecutor(t, ExecutorFunc(func(_ context.Context, exe string, args ...string) (Result, error) {
		gotExe, gotArgs = exe, args
		return Result{Output: "ok"}, nil
	}))

	out, err := Shell("echo hi && echo there")

	if err != nil || out != "ok" {
		t.Fatalf("Shell = (%q, %v)", out, err)
	}
	if gotExe != shellPath {
		t.Errorf("exe = %q, want %q", gotExe, shellPath)
	}
	if want := []string{shellFlag, "echo hi && echo there"}; !reflect.DeepEqual(gotArgs, want) {
		t.Errorf("args = %v, want %v", gotArgs, want)
	}
}

func TestShellContext_PropagatesContextAndResult(t *testing.T) {
	withExecutor(t, ExecutorFunc(func(ctx context.Context, _ string, _ ...string) (Result, error) {
		return Result{ExitCode: 9}, ctx.Err()
	}))

	ctx, cancel := context.WithCancel(context.Background())
	cancel()

	res, err := ShellContext(ctx, "true")

	if !errors.Is(err, context.Canceled) {
		t.Errorf("err = %v, want Canceled", err)
	}
	if res.ExitCode != 9 {
		t.Errorf("ExitCode = %d, want 9", res.ExitCode)
	}
}

// Integration tests with the real OS shell: they prove the platform
// constants are actually usable.
func TestShell_RealShell(t *testing.T) {
	withExecutor(t, nil)
	if !HasBinary(shellPath) {
		t.Skipf("%s is not available", shellPath)
	}

	out, err := Shell("echo hello")
	if err != nil || out != "hello" {
		t.Errorf("Shell = (%q, %v)", out, err)
	}
}

func TestShellContext_RealShellExitCode(t *testing.T) {
	withExecutor(t, nil)
	if !HasBinary(shellPath) {
		t.Skipf("%s is not available", shellPath)
	}

	res, err := ShellContext(context.Background(), "exit 3")

	var exitErr *exec.ExitError
	if !errors.As(err, &exitErr) {
		t.Fatalf("err = %v, want *exec.ExitError", err)
	}
	if res.ExitCode != 3 || res.OK() {
		t.Errorf("ExitCode = %d, OK = %v", res.ExitCode, res.OK())
	}
}
