package core

import (
	"fmt"
	"runtime"
	"strconv"
	"strings"
)

var (
	Name    = "Azula"
	Version = "0.1.0"
	Author  = "Azula Team"
	Website = "https://azula.org/"
)

// BuildInfo is an immutable value object describing the running build.
type BuildInfo struct {
	Name    string
	Version string
	Author  string
	Website string
}

// Info returns a snapshot of the current build metadata.
func BannerInfo() BuildInfo {
	return BuildInfo{Name: Name, Version: Version, Author: Author, Website: Website}
}

// String returns the short form: "Azula v0.1.0".
func (b BuildInfo) String() string {
	return fmt.Sprintf("%s v%s", b.Name, b.Version)
}

// Banner returns the greeting line with platform and runtime details:
// "Azula v0.1.0 (built for linux amd64 with go1.22.5)".
func (b BuildInfo) Banner() string {
	return fmt.Sprintf("%s v%s (built for %s %s with %s)", b.Name, b.Version, runtime.GOOS, runtime.GOARCH, runtime.Version())
}

// Full returns the banner followed by author and project website.
func (b BuildInfo) Full() string {
	return fmt.Sprintf("%s\n\nby %s — %s", b.Banner(), b.Author, b.Website)
}

// Semver parses Version as strict major.minor.patch. An optional leading "v"
// and "-prerelease" / "+build" suffixes are accepted and ignored.
// Anything else (missing parts, extra parts, non-numeric parts) is an error,
// so a malformed version fails fast here instead of at the call site.
func (b BuildInfo) Semver() (major, minor, patch int, err error) {
	v := strings.TrimPrefix(b.Version, "v")
	v, _, _ = strings.Cut(v, "+")
	v, _, _ = strings.Cut(v, "-")
	parts := strings.Split(v, ".")

	if len(parts) != 3 {
		return 0, 0, 0, fmt.Errorf("malformed version %q: want major.minor.patch", b.Version)
	}
	var nums [3]int

	for i, p := range parts {
		n, convErr := strconv.Atoi(p)
		if convErr != nil || n < 0 {
			return 0, 0, 0, fmt.Errorf("malformed version %q: bad component %q", b.Version, p)
		}
		nums[i] = n
	}
	return nums[0], nums[1], nums[2], nil
}
