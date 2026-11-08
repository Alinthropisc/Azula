// File: js/log.go
package js

import (
	"os"

	"github.com/Alinthropisc/Azula/log"
	"github.com/robertkrimen/otto"
)

// exit terminates the process for log_fatal. It is a variable so that tests
// can replace it.
var exit = os.Exit

// jsLog is a Factory: it builds a logging builtin for the given level.
// It replaces six near-identical functions of the original (DRY) and routes
// everything through the project log facade (Tracing support).
func jsLog(level log.Level) builtin {
	return func(call otto.FunctionCall) otto.Value {
		for _, v := range call.ArgumentList {
			log.Log(level, "%s", v.String())
		}
		return NullValue
	}
}

// jsFatal keeps the original contract: log_fatal(...) terminates the process.
// islazy/log.Fatal called os.Exit, while log.Fatal of the facade does not.
// With no arguments nothing happens, as before.
func jsFatal() builtin {
	logAll := jsLog(log.LevelFatal)
	return func(call otto.FunctionCall) otto.Value {
		if len(call.ArgumentList) == 0 {
			return NullValue
		}
		logAll(call)
		exit(1)
		return NullValue
	}
}
