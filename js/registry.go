// File: js/registry.go
package js

import (
	"github.com/Alinthropisc/Azula/log"
	"github.com/evilsocket/islazy/plugin"
	"github.com/robertkrimen/otto"
)

// NullValue is returned by builtins without a result (API contract kept).
// Note: otto.Value{} is `undefined`, not `null`.
var NullValue = otto.Value{}

// ReportError logs a builtin error through the project log facade and
// returns NullValue.
func ReportError(format string, args ...interface{}) otto.Value {
	log.Error(format, args...)
	return NullValue
}

// Definitions is the declarative registry of JS builtins (Registry pattern).
// It is the single source of truth: init() copies it into plugin.Defines,
// and once session no longer depends on islazy/plugin it can be consumed
// directly.
//
// Builtin names are public API of caplets and therefore NOT renamed;
// branding lives in the global `azula` object.
func Definitions() map[string]interface{} {
	return map[string]interface{}{
		// fs
		"mkdirAll":   mkdirAll,
		"readDir":    readDir,
		"readFile":   readFile,
		"writeFile":  writeFile,
		"appendFile": appendFile,

		// log
		"log":       jsLog(log.LevelInfo),
		"log_debug": jsLog(log.LevelDebug),
		"log_info":  jsLog(log.LevelInfo),
		"log_warn":  jsLog(log.LevelWarning),
		"log_error": jsLog(log.LevelError),
		"log_fatal": jsFatal(),
		// Tracing events reach only tracing subscribers, not the logger.
		"log_trace": jsLog(log.LevelTracing),

		// crypto
		"Crypto": map[string]interface{}{
			"sha1":      cryptoSha1,
			"sha256":    cryptoSha256,
			"md5":       cryptoMD5,
			"sha1Hex":   cryptoSha1Hex,
			"sha256Hex": cryptoSha256Hex,
			"md5Hex":    cryptoMD5Hex,
		},

		// data
		"btoa":           btoa,
		"atob":           atob,
		"gzipCompress":   gzipCompress,
		"gzipDecompress": gzipDecompress,
		"textEncode":     textEncode,
		"textDecode":     textDecode,

		// http
		"httpRequest": httpRequest,
		"http":        httpPackage{},

		// random
		"random": randomPackage{},

		// build metadata for scripts
		"azula": newAzulaPackage(),
	}
}

func init() {
	defs := Definitions()
	for name, value := range defs {
		plugin.Defines[name] = value
	}
	log.Tracing("js: registered %d builtins", len(defs))
}
