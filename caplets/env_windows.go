//go:build windows

package caplets

import (
	"os"
	"path/filepath"
)

func defaultInstallBase() string {
	if base := os.Getenv("ALLUSERSPROFILE"); base != "" {
		return filepath.Join(base, "azula")
	}
	return `C:\ProgramData\azula`
}
