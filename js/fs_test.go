package js

import (
	"errors"
	"os"
	"path/filepath"
	"testing"
)

func writeTestFile(t *testing.T, path, content string) {
	t.Helper()
	if err := os.WriteFile(path, []byte(content), 0o644); err != nil {
		t.Fatal(err)
	}
}

func readTestFile(t *testing.T, path string) string {
	t.Helper()
	b, err := os.ReadFile(path)
	if err != nil {
		t.Fatal(err)
	}
	return string(b)
}

func TestMkdirAll(t *testing.T) {
	sink := captureLogs(t)
	vm := newVM(t)
	target := filepath.Join(t.TempDir(), "a", "b", "c")
	setVars(t, vm, map[string]interface{}{"p": target})

	if v := run(t, vm, `mkdirAll(p)`); !v.IsNull() {
		t.Errorf("result = %v, want null", v)
	}
	if info, err := os.Stat(target); err != nil || !info.IsDir() {
		t.Errorf("directory not created: %v", err)
	}
	// Idempotent.
	run(t, vm, `mkdirAll(p)`)
	if n := len(sink.errors()); n != 0 {
		t.Errorf("unexpected errors: %q", sink.errors())
	}
}

func TestMkdirAll_UnderRegularFile(t *testing.T) {
	sink := captureLogs(t)
	vm := newVM(t)
	file := filepath.Join(t.TempDir(), "f")
	writeTestFile(t, file, "x")
	setVars(t, vm, map[string]interface{}{"p": filepath.Join(file, "sub")})

	requireFailed(t, sink, run(t, vm, `mkdirAll(p)`), "mkdirAll under a file")
}

func TestReadDir(t *testing.T) {
	dir := t.TempDir()
	writeTestFile(t, filepath.Join(dir, "b.txt"), "x")
	writeTestFile(t, filepath.Join(dir, "a.txt"), "x")
	if err := os.Mkdir(filepath.Join(dir, "sub"), 0o755); err != nil {
		t.Fatal(err)
	}

	vm := newVM(t)
	setVars(t, vm, map[string]interface{}{"d": dir, "empty": t.TempDir()})

	if n := runInt(t, vm, `readDir(d).length`); n != 3 {
		t.Fatalf("length = %d, want 3", n)
	}
	for i, want := range []string{"a.txt", "b.txt", "sub"} { // os.ReadDir sorts by name
		src := "readDir(d)[" + string(rune('0'+i)) + "]"
		if got := runString(t, vm, src); got != want {
			t.Errorf("%s = %q, want %q", src, got, want)
		}
	}
	if n := runInt(t, vm, `readDir(empty).length`); n != 0 {
		t.Errorf("empty dir length = %d, want 0", n)
	}
}

func TestReadDir_Missing(t *testing.T) {
	sink := captureLogs(t)
	vm := newVM(t)
	setVars(t, vm, map[string]interface{}{"p": filepath.Join(t.TempDir(), "missing")})
	requireFailed(t, sink, run(t, vm, `readDir(p)`), "readDir")
}

func TestReadFile(t *testing.T) {
	dir := t.TempDir()
	path := filepath.Join(dir, "f.txt")
	writeTestFile(t, path, "héllo\nwörld\n")

	vm := newVM(t)
	setVars(t, vm, map[string]interface{}{"p": path})
	if got := runString(t, vm, `readFile(p)`); got != "héllo\nwörld\n" {
		t.Errorf("readFile = %q", got)
	}
}

func TestReadFile_Errors(t *testing.T) {
	dir := t.TempDir()
	for name, path := range map[string]string{
		"missing":   filepath.Join(dir, "missing"),
		"directory": dir,
	} {
		t.Run(name, func(t *testing.T) {
			sink := captureLogs(t)
			vm := newVM(t)
			setVars(t, vm, map[string]interface{}{"p": path})
			requireFailed(t, sink, run(t, vm, `readFile(p)`), "readFile")
		})
	}
}

func TestWriteFile(t *testing.T) {
	path := filepath.Join(t.TempDir(), "w.txt")
	vm := newVM(t)
	setVars(t, vm, map[string]interface{}{"p": path})

	if v := run(t, vm, `writeFile(p, "first")`); !v.IsNull() {
		t.Errorf("result = %v, want null", v)
	}
	if got := readTestFile(t, path); got != "first" {
		t.Errorf("content = %q", got)
	}
	run(t, vm, `writeFile(p, "second")`)
	if got := readTestFile(t, path); got != "second" {
		t.Errorf("overwrite failed, content = %q", got)
	}
}

// Backward compatibility: the original coerced any value with ToString.
func TestWriteFile_LenientTypes(t *testing.T) {
	sink := captureLogs(t)
	path := filepath.Join(t.TempDir(), "n.txt")
	vm := newVM(t)
	setVars(t, vm, map[string]interface{}{"p": path})

	run(t, vm, `writeFile(p, 42)`)
	if got := readTestFile(t, path); got != "42" {
		t.Errorf("content = %q, want 42", got)
	}
	if n := len(sink.errors()); n != 0 {
		t.Errorf("lenient call logged errors: %q", sink.errors())
	}
}

func TestWriteFile_Errors(t *testing.T) {
	dir := t.TempDir()
	for name, path := range map[string]string{
		"missing parent": filepath.Join(dir, "missing", "f"),
		"is a directory": dir,
	} {
		t.Run(name, func(t *testing.T) {
			sink := captureLogs(t)
			vm := newVM(t)
			setVars(t, vm, map[string]interface{}{"p": path})
			requireFailed(t, sink, run(t, vm, `writeFile(p, "x")`), "writeFile")
		})
	}
}

func TestAppendFile(t *testing.T) {
	sink := captureLogs(t)
	path := filepath.Join(t.TempDir(), "a.txt")
	vm := newVM(t)
	setVars(t, vm, map[string]interface{}{"p": path})

	// First call creates the file, the next ones append.
	run(t, vm, `appendFile(p, "a")`)
	run(t, vm, `appendFile(p, "b")`)
	run(t, vm, `appendFile(p, 3)`)

	if got := readTestFile(t, path); got != "ab3" {
		t.Errorf("content = %q, want ab3", got)
	}
	if n := len(sink.errors()); n != 0 {
		t.Errorf("unexpected errors: %q", sink.errors())
	}
}

func TestAppendFile_Error(t *testing.T) {
	sink := captureLogs(t)
	vm := newVM(t)
	setVars(t, vm, map[string]interface{}{"p": t.TempDir()}) // a directory
	requireFailed(t, sink, run(t, vm, `appendFile(p, "x")`), "appendFile")
}

func TestAppendTo_WrapsCause(t *testing.T) {
	err := appendTo(filepath.Join(t.TempDir(), "missing", "f"), "x")
	if err == nil {
		t.Fatal("expected an error")
	}
	if !errors.Is(err, os.ErrNotExist) {
		t.Errorf("error must wrap os.ErrNotExist (%%w), got: %v", err)
	}
}
