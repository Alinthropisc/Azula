package core

import (
	"context"
	"os/exec"
	"sync"
	"time"

	"github.com/evilsocket/islazy/str"
)

// Result describes a finished external command (Value Object).
type Result struct {
	// Path is the executable path resolved through $PATH.
	Path string
	// Args is the argument list the command was started with.
	Args []string
	// Output is the combined stdout+stderr with surrounding whitespace trimmed.
	Output string
	// ExitCode is the process exit code: 0 on success, -1 if the process
	// never started or was terminated by a signal.
	ExitCode int
	// Duration is the wall-clock execution time.
	Duration time.Duration
}

// OK reports whether the command finished with exit code 0.
func (r Result) OK() bool {
	return r.ExitCode == 0
}

// PathResolver finds executables (ISP: HasBinary needs nothing more).
type PathResolver interface {
	// LookPath resolves an executable name through $PATH.
	LookPath(executable string) (string, error)
}

// Runner runs external commands (ISP: separate from path resolution).
type Runner interface {
	// Run executes the command and collects its combined output.
	Run(ctx context.Context, executable string, args ...string) (Result, error)
}

// Executor abstracts process execution (Strategy + DIP): the package depends
// on this interface, not on os/exec. Implementations can be swapped in tests
// or decorated with timeouts and metrics without touching call sites.
type IExecutor interface {
	PathResolver
	Runner
}

// Compile-time interface checks.
var (
	_ IExecutor = LocalExecutor{}
	_ IExecutor = ExecutorFunc(nil)
)

// ExecutorFunc adapts a plain function to Executor (Adapter), handy for mocks.
// Its LookPath considers every binary available.
type ExecutorFunc func(ctx context.Context, executable string, args ...string) (Result, error)

// LookPath implements PathResolver.
func (f ExecutorFunc) LookPath(executable string) (string, error) {
	return executable, nil
}

// Run implements Runner.
func (f ExecutorFunc) Run(ctx context.Context, executable string, args ...string) (Result, error) {
	return f(ctx, executable, args...)
}

// LocalExecutor is the default Executor built on os/exec.
type LocalExecutor struct {
	// Timeout limits every command; <= 0 means no limit.
	Timeout time.Duration
	// Dir, if not empty, sets the working directory of commands.
	Dir string
	// Env, if not nil, fully replaces the environment of commands.
	Env []string
}

// LookPath implements PathResolver.
func (LocalExecutor) LookPath(executable string) (string, error) {
	return exec.LookPath(executable)
}

// Run implements Runner.
func (e LocalExecutor) Run(ctx context.Context, executable string, args ...string) (Result, error) {
	defer TraceFunc("core.LocalExecutor.Run")()

	// Own copy: the Result must not alias the caller's slice.
	args = append([]string(nil), args...)

	path, err := e.LookPath(executable)
	if err != nil {
		Warning("executable %s not found in $PATH", executable)
		// -1 so that Result.OK() is false for a command that never started.
		return Result{Args: args, ExitCode: -1}, err
	}
	Tracing("resolved %s -> %s", executable, path)

	if e.Timeout > 0 {
		var cancel context.CancelFunc
		ctx, cancel = context.WithTimeout(ctx, e.Timeout)
		defer cancel()
	}

	cmd := exec.CommandContext(ctx, path, args...)
	cmd.Dir = e.Dir
	if e.Env != nil {
		cmd.Env = e.Env
	}

	start := time.Now()
	raw, runErr := cmd.CombinedOutput()

	res := Result{
		Path:     path,
		Args:     args,
		Output:   str.Trim(string(raw)),
		ExitCode: exitCodeOf(cmd, runErr),
		Duration: time.Since(start),
	}

	Debug("exec=%s args=%v err=%v exit=%d took=%s out=%s",
		path, args, runErr, res.ExitCode, res.Duration, res.Output)

	if runErr != nil {
		// Cancellation or timeout explains a failure better than the
		// "signal: killed" error it caused. A successful run is never
		// turned into an error by a deadline that expired afterwards.
		if ctxErr := ctx.Err(); ctxErr != nil {
			return res, ctxErr
		}
	}
	return res, runErr
}

func exitCodeOf(cmd *exec.Cmd, runErr error) int {
	switch {
	case cmd.ProcessState != nil:
		return cmd.ProcessState.ExitCode()
	case runErr != nil:
		return -1
	default:
		return 0
	}
}

var (
	execMu   sync.RWMutex
	executor IExecutor = LocalExecutor{}
)

// UseExecutor installs e (nil restores LocalExecutor) and returns a function
// that puts the previous executor back. Safe for concurrent use.
//
//	defer core.UseExecutor(mock)()
func UseExecutor(e IExecutor) (restore func()) {
	if e == nil {
		e = LocalExecutor{}
	}

	execMu.Lock()
	prev := executor
	executor = e
	execMu.Unlock()

	return func() {
		execMu.Lock()
		executor = prev
		execMu.Unlock()
	}
}

// SetExecutor installs e; nil restores LocalExecutor. Safe for concurrent use.
func SetExecutor(e IExecutor) {
	UseExecutor(e)
}

// DefaultExecutor returns the current executor (never nil).
func DefaultExecutor() IExecutor {
	execMu.RLock()
	defer execMu.RUnlock()
	return executor
}

// HasBinary reports whether the executable is available in $PATH.
func HasBinary(executable string) bool {
	path, err := DefaultExecutor().LookPath(executable)
	found := err == nil && path != ""
	Tracing("HasBinary(%q) path=%q found=%v", executable, path, found)
	return found
}

// Exec runs a command and returns its combined stdout+stderr with surrounding
// whitespace trimmed. The output is returned even when err != nil, because it
// usually contains the command's own error text. The signature is kept for
// compatibility; new code should prefer ExecContext.
func Exec(executable string, args []string) (string, error) {
	res, err := ExecContext(context.Background(), executable, args...)
	return res.Output, err
}

// ExecContext is Exec with cancellation/timeout via ctx and a full Result.
func ExecContext(ctx context.Context, executable string, args ...string) (Result, error) {
	return DefaultExecutor().Run(ctx, executable, args...)
}

// Shell runs cmd through the platform shell. The shell path and flag come from
// the platform files (shell_unix.go, shell_windows.go, ...), so the function
// is defined exactly once (DRY).
func Shell(cmd string) (string, error) {
	return Exec(shellPath, []string{shellFlag, cmd})
}

// ShellContext is Shell with cancellation/timeout via ctx and a full Result.
func ShellContext(ctx context.Context, cmd string) (Result, error) {
	return ExecContext(ctx, shellPath, shellFlag, cmd)
}
