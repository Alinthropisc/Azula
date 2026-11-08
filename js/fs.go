// File: js/fs.go
package js

import (
	"fmt"
	"os"

	"github.com/robertkrimen/otto"
)

// pathArg handles the common case: exactly one path argument.
// Lenient on purpose: the original used ToString without type checks.
func pathArg(call otto.FunctionCall, fn string) (string, bool) {
	if !checkArgs(call, fn, 1) {
		return "", false
	}
	return coercedString(call, fn, 0)
}

// fileArgs handles writeFile/appendFile: path + data (lenient, see pathArg).
func fileArgs(call otto.FunctionCall, fn string) (string, string, bool) {
	if !checkArgs(call, fn, 2) {
		return "", "", false
	}
	filename, _ := coercedString(call, fn, 0)
	data, _ := coercedString(call, fn, 1)
	return filename, data, true
}

func mkdirAll(call otto.FunctionCall) otto.Value {
	path, ok := pathArg(call, "mkdirAll")
	if !ok {
		return NullValue
	}
	if err := os.MkdirAll(path, 0o755); err != nil {
		return ReportError("mkdirAll: could not create directory %s: %s", path, err)
	}
	return otto.NullValue()
}

func readDir(call otto.FunctionCall) otto.Value {
	path, ok := pathArg(call, "readDir")
	if !ok {
		return NullValue
	}
	entries, err := os.ReadDir(path)
	if err != nil {
		return ReportError("readDir: could not read directory %s: %s", path, err)
	}
	names := make([]string, 0, len(entries))
	for _, entry := range entries {
		names = append(names, entry.Name())
	}
	return toValue(call, "readDir", names)
}

func readFile(call otto.FunctionCall) otto.Value {
	filename, ok := pathArg(call, "readFile")
	if !ok {
		return NullValue
	}
	raw, err := os.ReadFile(filename)
	if err != nil {
		return ReportError("readFile: could not read file %s: %s", filename, err)
	}
	return toValue(call, "readFile", string(raw))
}

func writeFile(call otto.FunctionCall) otto.Value {
	filename, data, ok := fileArgs(call, "writeFile")
	if !ok {
		return NullValue
	}
	if err := os.WriteFile(filename, []byte(data), 0o644); err != nil {
		return ReportError("writeFile: could not write %d bytes to %s: %s", len(data), filename, err)
	}
	return otto.NullValue()
}

// appendTo appends data and reports the Close error too: on a full disk it
// is often the only place where the failure shows up.
func appendTo(filename, data string) (err error) {
	file, err := os.OpenFile(filename, os.O_APPEND|os.O_CREATE|os.O_WRONLY, 0o644)
	if err != nil {
		return fmt.Errorf("could not open file %s for appending: %w", filename, err)
	}
	defer func() {
		if cerr := file.Close(); err == nil && cerr != nil {
			err = fmt.Errorf("could not close %s: %w", filename, cerr)
		}
	}()
	if _, err = file.WriteString(data); err != nil {
		return fmt.Errorf("could not append %d bytes to %s: %w", len(data), filename, err)
	}
	return nil
}

func appendFile(call otto.FunctionCall) otto.Value {
	filename, data, ok := fileArgs(call, "appendFile")
	if !ok {
		return NullValue
	}
	if err := appendTo(filename, data); err != nil {
		return ReportError("appendFile: %s", err)
	}
	return otto.NullValue()
}
