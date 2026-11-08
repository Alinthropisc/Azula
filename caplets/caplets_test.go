package caplets

import (
	"bufio"
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"reflect"
	"runtime"
	"sort"
	"strings"
	"sync"
	"testing"
	"time"
)

// ---------------------------------------------------------------------------
// Adapters: the only place that knows how tests construct the objects.
// If a constructor signature changes, fix it here and nowhere else.
// ---------------------------------------------------------------------------

func newTestManager(t *testing.T, dirs ...string) *Manager {
	t.Helper()
	cfg := NewConfig(t.TempDir())
	cfg.LoadPaths = dirs
	return NewManager(cfg)
}

func listNames(m *Manager) []string {
	var names []string
	for _, c := range m.List() {
		names = append(names, c.Name)
	}
	sort.Strings(names)
	return names
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

func writeFile(t *testing.T, path, content string) {
	t.Helper()
	if err := os.MkdirAll(filepath.Dir(path), 0o755); err != nil {
		t.Fatalf("mkdir: %v", err)
	}
	if err := os.WriteFile(path, []byte(content), 0o644); err != nil {
		t.Fatalf("write: %v", err)
	}
}

func mustLoad(t *testing.T, m *Manager, name string) *Caplet {
	t.Helper()
	c, err := m.Load(name)
	if err != nil {
		t.Fatalf("Load(%q): %v", name, err)
	}
	if c == nil {
		t.Fatalf("Load(%q) returned nil caplet without error", name)
	}
	return c
}

func evalLines(t *testing.T, c *Caplet, argv []string) []string {
	t.Helper()
	var got []string
	err := c.Eval(argv, func(line string) error {
		got = append(got, line)
		return nil
	})
	if err != nil {
		t.Fatalf("Eval: %v", err)
	}
	return got
}

func sameDir(t *testing.T, a, b string) bool {
	t.Helper()
	ra, err := filepath.EvalSymlinks(a)
	if err != nil {
		t.Fatalf("EvalSymlinks(%q): %v", a, err)
	}
	rb, err := filepath.EvalSymlinks(b)
	if err != nil {
		t.Fatalf("EvalSymlinks(%q): %v", b, err)
	}
	return ra == rb
}

// withTimeout fails the test instead of hanging forever (deadlock guard).
func withTimeout(t *testing.T, d time.Duration, what string, fn func()) {
	t.Helper()
	done := make(chan struct{})
	go func() {
		defer close(done)
		fn()
	}()
	select {
	case <-done:
	case <-time.After(d):
		t.Fatalf("%s: timed out after %v (deadlock?)", what, d)
	}
}

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

func TestConstants(t *testing.T) {
	if Suffix != ".cap" {
		t.Errorf("Suffix = %q", Suffix)
	}
	if EnvVarName != "AZULA_CAPSPATH" {
		t.Errorf("EnvVarName = %q", EnvVarName)
	}
	if !strings.HasPrefix(EnvVarName, EnvPrefix) {
		t.Errorf("EnvVarName %q must start with %q", EnvVarName, EnvPrefix)
	}
}

// ---------------------------------------------------------------------------
// Loading
// ---------------------------------------------------------------------------

func TestLoad_BasicAndNameNormalization(t *testing.T) {
	dir := t.TempDir()
	writeFile(t, filepath.Join(dir, "foo.cap"), "one\ntwo\n")
	m := newTestManager(t, dir)

	a := mustLoad(t, m, "foo")
	b := mustLoad(t, m, "foo.cap")

	if a != b {
		t.Error("Load(foo) and Load(foo.cap) must return the same cached caplet (regression #13)")
	}
	if a.Name != "foo" {
		t.Errorf("Name = %q, want %q", a.Name, "foo")
	}
	if got, want := evalLines(t, a, nil), []string{"one", "two"}; !reflect.DeepEqual(got, want) {
		t.Errorf("lines = %q, want %q", got, want)
	}
}

func TestLoad_BOMAndCRLF(t *testing.T) {
	dir := t.TempDir()
	writeFile(t, filepath.Join(dir, "bom.cap"), "\uFEFFfirst\r\nsecond\r\n")
	m := newTestManager(t, dir)

	got := evalLines(t, mustLoad(t, m, "bom"), nil)
	want := []string{"first", "second"}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("lines = %q, want %q (BOM must be stripped, CR removed) (regression #3)", got, want)
	}
}

func TestLoad_SkipsCommentsAndBlankLines(t *testing.T) {
	dir := t.TempDir()
	writeFile(t, filepath.Join(dir, "c.cap"),
		"# comment\n\n   \n\t# indented comment\ncmd1\n  # another\ncmd2\n")
	m := newTestManager(t, dir)

	got := evalLines(t, mustLoad(t, m, "c"), nil)
	want := []string{"cmd1", "cmd2"}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("lines = %q, want %q (regression #4)", got, want)
	}
}

func TestLoad_LongLines(t *testing.T) {
	dir := t.TempDir()

	// 100 KB: over the default bufio.Scanner limit (64 KB), must still load.
	long := strings.Repeat("x", 100*1024)
	writeFile(t, filepath.Join(dir, "long.cap"), long+"\nshort\n")
	// 5 MiB: over our own limit, must fail cleanly.
	writeFile(t, filepath.Join(dir, "huge.cap"), strings.Repeat("y", 5*1024*1024)+"\n")

	m := newTestManager(t, dir)

	got := evalLines(t, mustLoad(t, m, "long"), nil)
	if len(got) != 2 || len(got[0]) != len(long) || got[1] != "short" {
		t.Errorf("long line not preserved: got %d lines, first len=%d (regression #2)", len(got), len(got[0]))
	}

	_, err := m.Load("huge")
	if err == nil {
		t.Fatal("expected an error for a line above the size limit")
	}
	if !errors.Is(err, bufio.ErrTooLong) {
		t.Errorf("error should wrap bufio.ErrTooLong, got: %v", err)
	}
}

func TestLoad_NotFound(t *testing.T) {
	m := newTestManager(t, t.TempDir())
	c, err := m.Load("does-not-exist")
	if err == nil {
		t.Fatalf("expected error, got caplet %+v", c)
	}
}

func TestLoad_DirectoryNamedLikeCapletIsIgnored(t *testing.T) {
	dir1, dir2 := t.TempDir(), t.TempDir()
	// dir1 contains a DIRECTORY called x.cap, dir2 a real file.
	if err := os.MkdirAll(filepath.Join(dir1, "x.cap"), 0o755); err != nil {
		t.Fatal(err)
	}
	writeFile(t, filepath.Join(dir2, "x.cap"), "real\n")

	m := newTestManager(t, dir1, dir2)
	got := evalLines(t, mustLoad(t, m, "x"), nil)
	if !reflect.DeepEqual(got, []string{"real"}) {
		t.Errorf("lines = %q (directory x.cap must be skipped) (regression #6)", got)
	}

	// With only the directory available, Load must fail.
	m2 := newTestManager(t, dir1)
	if _, err := m2.Load("x"); err == nil {
		t.Error("loading a directory as a caplet must fail")
	}
}

func TestLoad_PathWithGlobMetacharacters(t *testing.T) {
	// '[' is legal on every OS; '*' and '?' are not legal on Windows.
	dir := filepath.Join(t.TempDir(), "dir[1]")
	if runtime.GOOS != "windows" {
		dir = filepath.Join(t.TempDir(), "we[ir]d*dir?")
	}
	writeFile(t, filepath.Join(dir, "g.cap"), "ok\n")

	m := newTestManager(t, dir)

	if got := evalLines(t, mustLoad(t, m, "g"), nil); !reflect.DeepEqual(got, []string{"ok"}) {
		t.Errorf("lines = %q (regression #5)", got)
	}
	if names := listNames(m); !reflect.DeepEqual(names, []string{"g"}) {
		t.Errorf("List = %q, want [g] (regression #5)", names)
	}
}

func TestLoad_PathOrderFirstWins(t *testing.T) {
	first, second := t.TempDir(), t.TempDir()
	writeFile(t, filepath.Join(first, "a.cap"), "from-first\n")
	writeFile(t, filepath.Join(second, "a.cap"), "from-second\n")
	writeFile(t, filepath.Join(second, "only2.cap"), "only-second\n")

	m := newTestManager(t, first, second)

	if got := evalLines(t, mustLoad(t, m, "a"), nil); !reflect.DeepEqual(got, []string{"from-first"}) {
		t.Errorf("lines = %q, the first load path must win", got)
	}
	if got := evalLines(t, mustLoad(t, m, "only2"), nil); !reflect.DeepEqual(got, []string{"only-second"}) {
		t.Errorf("lines = %q, fallback to the second path failed", got)
	}
}

func TestLoad_CacheAndInvalidate(t *testing.T) {
	dir := t.TempDir()
	path := filepath.Join(dir, "k.cap")
	writeFile(t, path, "v1\n")
	m := newTestManager(t, dir)

	c1 := mustLoad(t, m, "k")
	writeFile(t, path, "v2\n")

	c2 := mustLoad(t, m, "k")
	if c1 != c2 {
		t.Error("second Load must hit the cache and return the same pointer")
	}
	if got := evalLines(t, c2, nil); !reflect.DeepEqual(got, []string{"v1"}) {
		t.Errorf("cached caplet changed: %q", got)
	}

	m.Invalidate()

	c3 := mustLoad(t, m, "k")
	if c3 == c1 {
		t.Error("Invalidate must drop the cache")
	}
	if got := evalLines(t, c3, nil); !reflect.DeepEqual(got, []string{"v2"}) {
		t.Errorf("after Invalidate lines = %q, want [v2]", got)
	}
}

// Run with -race. All goroutines must observe the very same *Caplet.
func TestLoad_ParallelSamePointer(t *testing.T) {
	dir := t.TempDir()
	writeFile(t, filepath.Join(dir, "p.cap"), "a\nb\n")
	m := newTestManager(t, dir)

	const n = 32
	var (
		wg    sync.WaitGroup
		start = make(chan struct{})
		got   = make([]*Caplet, n)
		errs  = make([]error, n)
	)
	for i := 0; i < n; i++ {
		wg.Add(1)
		go func(i int) {
			defer wg.Done()
			<-start
			got[i], errs[i] = m.Load("p")
		}(i)
	}
	close(start)
	wg.Wait()

	for i := 0; i < n; i++ {
		if errs[i] != nil {
			t.Fatalf("goroutine %d: %v", i, errs[i])
		}
		if got[i] != got[0] {
			t.Fatalf("goroutine %d got a different pointer (regression #7)", i)
		}
	}
}

// ---------------------------------------------------------------------------
// List
// ---------------------------------------------------------------------------

func TestList(t *testing.T) {
	dir := t.TempDir()
	writeFile(t, filepath.Join(dir, "a.cap"), "x\n")
	writeFile(t, filepath.Join(dir, "b.cap"), "x\n")
	writeFile(t, filepath.Join(dir, "notes.txt"), "x\n")
	if err := os.MkdirAll(filepath.Join(dir, "d.cap"), 0o755); err != nil {
		t.Fatal(err)
	}

	m := newTestManager(t, dir)
	if got, want := listNames(m), []string{"a", "b"}; !reflect.DeepEqual(got, want) {
		t.Errorf("List = %q, want %q (only regular *.cap files)", got, want)
	}
}

func TestList_EmptyAndMissingDirs(t *testing.T) {
	m := newTestManager(t, t.TempDir(), filepath.Join(t.TempDir(), "missing"))
	if got := listNames(m); len(got) != 0 {
		t.Errorf("List = %q, want empty", got)
	}
}

// ---------------------------------------------------------------------------
// Eval
// ---------------------------------------------------------------------------

func TestEval_Interpolation(t *testing.T) {
	eleven := make([]string, 11)
	for i := range eleven {
		eleven[i] = fmt.Sprintf("a%d", i)
	}

	tests := []struct {
		name string
		line string
		argv []string
		want string
	}{
		{"nil argv keeps placeholder", "echo $0", nil, "echo $0"},
		{"basic", "$0 $1", []string{"a", "b"}, "a b"},
		{"adjacent", "$1$0", []string{"a", "b"}, "ba"},
		{"out of range kept", "echo $5", []string{"a"}, "echo $5"},
		{"two digit out of range is not $1 followed by 0", "$10", []string{"a", "b"}, "$10"},
		{"two digit in range", "$10", eleven, "a10"},
		{"single digit with many args", "$1", eleven, "a1"},
		{"bare dollar literal", "cost $", []string{"a"}, "cost $"},
		{"dollar non digit literal", "$x", []string{"a"}, "$x"},
		{"single pass: no re-expansion", "$0", []string{"$1", "X"}, "$1"},
	}

	for _, tc := range tests {
		t.Run(tc.name, func(t *testing.T) {
			dir := t.TempDir()
			writeFile(t, filepath.Join(dir, "i.cap"), tc.line+"\n")
			m := newTestManager(t, dir)

			got := evalLines(t, mustLoad(t, m, "i"), tc.argv)
			if len(got) != 1 || got[0] != tc.want {
				t.Errorf("Eval(%q, %q) = %q, want [%q]", tc.line, tc.argv, got, tc.want)
			}
		})
	}
}

func TestEval_NilCallback(t *testing.T) {
	dir := t.TempDir()
	writeFile(t, filepath.Join(dir, "n.cap"), "x\n")
	c := mustLoad(t, newTestManager(t, dir), "n")

	if err := c.Eval(nil, nil); err == nil {
		t.Error("Eval with a nil callback must return an error")
	}
}

func TestEval_CallbackErrorStopsEvaluation(t *testing.T) {
	dir := t.TempDir()
	writeFile(t, filepath.Join(dir, "e.cap"), "one\ntwo\nthree\n")
	c := mustLoad(t, newTestManager(t, dir), "e")

	boom := errors.New("boom")
	var seen []string
	err := c.Eval(nil, func(line string) error {
		seen = append(seen, line)
		if line == "two" {
			return boom
		}
		return nil
	})

	if !errors.Is(err, boom) {
		t.Errorf("err = %v, want it to wrap %v", err, boom)
	}
	if !reflect.DeepEqual(seen, []string{"one", "two"}) {
		t.Errorf("seen = %q, evaluation must stop at the first error", seen)
	}
}

func TestEval_WorkdirIsCapletDirAndRestored(t *testing.T) {
	dir := t.TempDir()
	writeFile(t, filepath.Join(dir, "w.cap"), "a\nb\n")
	c := mustLoad(t, newTestManager(t, dir), "w")

	before, err := os.Getwd()
	if err != nil {
		t.Fatal(err)
	}

	var inside []string
	if err := c.Eval(nil, func(string) error {
		wd, err := os.Getwd()
		if err != nil {
			return err
		}
		inside = append(inside, wd)
		return nil
	}); err != nil {
		t.Fatalf("Eval: %v", err)
	}

	for _, wd := range inside {
		if !sameDir(t, wd, dir) {
			t.Errorf("cwd during Eval = %q, want %q", wd, dir)
		}
	}
	after, _ := os.Getwd()
	if after != before {
		t.Errorf("cwd not restored: before=%q after=%q", before, after)
	}

	// Also restored when the callback fails.
	_ = c.Eval(nil, func(string) error { return errors.New("fail") })
	after, _ = os.Getwd()
	if after != before {
		t.Errorf("cwd not restored after failing Eval: before=%q after=%q", before, after)
	}
}

// Regression #1: Eval used to hold a mutex across the callback, so a
// callback that evaluates another caplet deadlocked.
func TestEval_NestedDoesNotDeadlock(t *testing.T) {
	dir := t.TempDir()
	writeFile(t, filepath.Join(dir, "outer.cap"), "o1\no2\n")
	writeFile(t, filepath.Join(dir, "inner.cap"), "i1\n")
	m := newTestManager(t, dir)
	outer := mustLoad(t, m, "outer")

	var trace []string
	withTimeout(t, 5*time.Second, "nested Eval", func() {
		err := outer.Eval(nil, func(line string) error {
			trace = append(trace, line)
			inner, err := m.Load("inner")
			if err != nil {
				return err
			}
			return inner.Eval(nil, func(l string) error {
				trace = append(trace, "  "+l)
				return nil
			})
		})
		if err != nil {
			t.Errorf("nested Eval: %v", err)
		}
	})

	want := []string{"o1", "  i1", "o2", "  i1"}
	if !reflect.DeepEqual(trace, want) {
		t.Errorf("trace = %q, want %q", trace, want)
	}
}

// ---------------------------------------------------------------------------
// Config (environment handling)
// ---------------------------------------------------------------------------

func countPath(paths []string, dir string) int {
	want := filepath.Clean(dir)
	n := 0
	for _, p := range paths {
		abs, err := filepath.Abs(p)
		if err != nil {
			abs = p
		}
		if filepath.Clean(abs) == want {
			n++
		}
	}
	return n
}

func indexOfPath(paths []string, dir string) int {
	want := filepath.Clean(dir)
	for i, p := range paths {
		abs, err := filepath.Abs(p)
		if err != nil {
			abs = p
		}
		if filepath.Clean(abs) == want {
			return i
		}
	}
	return -1
}

func TestNewConfig_DeduplicatesLoadPaths(t *testing.T) {
	dir := t.TempDir()
	sep := string(os.PathListSeparator)

	t.Setenv(EnvVarName, dir+sep+dir+sep+dir+string(filepath.Separator))
	t.Setenv(legacyEnvVarName, dir)

	cfg := NewConfig(t.TempDir())
	if n := countPath(cfg.LoadPaths, dir); n != 1 {
		t.Errorf("dir appears %d times in %q, want exactly once (regression #10)", n, cfg.LoadPaths)
	}
}

func TestNewConfig_AzulaEnvBeatsLegacyEnv(t *testing.T) {
	azulaDir, legacyDir := t.TempDir(), t.TempDir()
	t.Setenv(EnvVarName, azulaDir)
	t.Setenv(legacyEnvVarName, legacyDir)

	cfg := NewConfig(t.TempDir())
	ai, li := indexOfPath(cfg.LoadPaths, azulaDir), indexOfPath(cfg.LoadPaths, legacyDir)
	if ai < 0 || li < 0 {
		t.Fatalf("both env dirs must be present: azula=%d legacy=%d in %q", ai, li, cfg.LoadPaths)
	}
	if ai > li {
		t.Errorf("%s must take priority over %s: indexes %d > %d", EnvVarName, legacyEnvVarName, ai, li)
	}
}

func TestNewConfig_LegacyEnvStillWorks(t *testing.T) {
	legacyDir := t.TempDir()
	t.Setenv(EnvVarName, "")
	t.Setenv(legacyEnvVarName, legacyDir)

	cfg := NewConfig(t.TempDir())
	if indexOfPath(cfg.LoadPaths, legacyDir) < 0 {
		t.Errorf("legacy %s directory missing from %q", legacyEnvVarName, cfg.LoadPaths)
	}
}

// ---------------------------------------------------------------------------
// Errors
// ---------------------------------------------------------------------------

// Regression #11: errors must stay inspectable, not be flattened to a string.
func TestLoad_ErrorsRemainInspectable(t *testing.T) {
	dir := t.TempDir()
	writeFile(t, filepath.Join(dir, "bad.cap"), strings.Repeat("z", 5*1024*1024))
	m := newTestManager(t, dir)

	_, err := m.Load("bad")
	if err == nil {
		t.Fatal("expected error")
	}
	if !errors.Is(err, bufio.ErrTooLong) {
		t.Errorf("errors.Is(err, bufio.ErrTooLong) = false; err = %v", err)
	}
	if !strings.Contains(err.Error(), "bad") {
		t.Errorf("error message should mention the caplet: %v", err)
	}
}
