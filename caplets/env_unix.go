//go:build !windows

package caplets

// defaultInstallBase is the platform install directory (OCP: a new OS means
// a new file, no existing code changes).
func defaultInstallBase() string {
	return "/usr/local/share/azula/"
}
