package core

import (
	"runtime"
	"testing"
)

// Verifies that the right platform file was selected by the build tags
// (regression for bug #1: files without tags caused "shellPath redeclared").
func TestShellConstants_MatchPlatform(t *testing.T) {
	var wantPath, wantFlag string
	switch runtime.GOOS {
	case "windows":
		wantPath, wantFlag = "cmd.exe", "/c"
	case "android":
		wantPath, wantFlag = "/system/bin/sh", "-c"
	default:
		wantPath, wantFlag = "/bin/sh", "-c"
	}

	if shellPath != wantPath || shellFlag != wantFlag {
		t.Errorf("shell = %q %q, want %q %q on %s", shellPath, shellFlag, wantPath, wantFlag, runtime.GOOS)
	}
}
