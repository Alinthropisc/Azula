// File: js/args.go
package js

import (
	"github.com/Alinthropisc/Azula/log"
	"github.com/robertkrimen/otto"
)

// builtin is the signature of every function exposed to scripts.
type builtin = func(otto.FunctionCall) otto.Value

// checkArgs verifies the exact argument count and logs on mismatch.
func checkArgs(call otto.FunctionCall, fn string, want int) bool {
	if argc := len(call.ArgumentList); argc != want {
		ReportError("%s: expected %d argument(s), %d given instead.", fn, want, argc)
		return false
	}
	return true
}

// checkMinArgs verifies the minimum argument count and logs on mismatch.
func checkMinArgs(call otto.FunctionCall, fn string, min int) bool {
	if argc := len(call.ArgumentList); argc < min {
		ReportError("%s: expected %d or more arguments, %d given instead.", fn, min, argc)
		return false
	}
	return true
}

// argReader extracts the i-th argument as a Go string (Strategy pattern).
type argReader func(call otto.FunctionCall, fn string, i int) (string, bool)

// strictString accepts only JS string primitives.
func strictString(call otto.FunctionCall, fn string, i int) (string, bool) {
	arg := call.Argument(i)
	if !arg.IsString() {
		ReportError("%s: argument #%d must be a string.", fn, i+1)
		return "", false
	}
	return arg.String(), true
}

// coercedString keeps the legacy behaviour: any value goes through ToString.
// Used by builtins that never validated their argument types.
func coercedString(call otto.FunctionCall, _ string, i int) (string, bool) {
	return call.Argument(i).String(), true
}

// toValue converts a Go value into an otto.Value bound to the calling VM.
// Values created through a throw-away otto.New() are bound to the wrong VM.
func toValue(call otto.FunctionCall, fn string, v interface{}) otto.Value {
	value, err := call.Otto.ToValue(v)
	if err != nil {
		return ReportError("%s: could not convert result: %s", fn, err)
	}
	return value
}

// unary is a Decorator: it turns f(string) (interface{}, error) into a full
// builtin with argument-count validation, pluggable argument reading and
// result conversion.
func unary(fn string, read argReader, f func(string) (interface{}, error)) builtin {
	return func(call otto.FunctionCall) otto.Value {
		if !checkArgs(call, fn, 1) {
			return NullValue
		}
		arg, ok := read(call, fn, 0)
		if !ok {
			return NullValue
		}
		log.Tracing("js: %s called with %d-byte argument", fn, len(arg))

		out, err := f(arg)
		if err != nil {
			return ReportError("%s: %s", fn, err)
		}
		return toValue(call, fn, out)
	}
}

// unaryString is unary with strict string-typed input.
func unaryString(fn string, f func(string) (interface{}, error)) builtin {
	return unary(fn, strictString, f)
}

// unaryCoerced is unary with legacy lenient input.
func unaryCoerced(fn string, f func(string) (interface{}, error)) builtin {
	return unary(fn, coercedString, f)
}
