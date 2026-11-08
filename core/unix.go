//go:build !windows && !android

package core

// Platform-specific shell parameters. The Shell function itself lives in
// exec.go; each platform only supplies constants (OCP: a new OS means a new
// file, no existing code changes).
const (
	shellPath = "/bin/sh"
	shellFlag = "-c"
)
