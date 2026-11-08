package core

import (
	"fmt"
	"runtime"
	"strings"
	"testing"
)

// setBuildVars overrides the link-time variables and restores them on cleanup.
func setBuildVars(t *testing.T, name, version, author, website string) {
	t.Helper()
	oldN, oldV, oldA, oldW := Name, Version, Author, Website
	Name, Version, Author, Website = name, version, author, website
	t.Cleanup(func() { Name, Version, Author, Website = oldN, oldV, oldA, oldW })
}

func TestInfo_ReflectsPackageVariables(t *testing.T) {
	setBuildVars(t, "Tool", "9.8.7", "Me", "https://example.org/")

	want := BuildInfo{Name: "Tool", Version: "9.8.7", Author: "Me", Website: "https://example.org/"}
	if got := BannerInfo(); got != want {
		t.Errorf("Info() = %+v, want %+v", got, want)
	}
}

// Info must be a snapshot: later changes of the globals do not affect it.
func TestInfo_IsSnapshot(t *testing.T) {
	setBuildVars(t, "A", "1.0.0", "x", "y")
	snap := BannerInfo()
	Version = "2.0.0"

	if snap.Version != "1.0.0" {
		t.Errorf("snapshot changed: %q", snap.Version)
	}
}

func TestDefaultVersionIsValidSemver(t *testing.T) {
	// Guards against typos in the default Version (or a bad -ldflags value).
	if _, _, _, err := BannerInfo().Semver(); err != nil {
		t.Errorf("default Version %q is not valid semver: %v", Version, err)
	}
	if Name == "" || Author == "" || Website == "" {
		t.Error("Name, Author and Website must not be empty")
	}
}

func TestBuildInfo_String(t *testing.T) {
	b := BuildInfo{Name: "Azula", Version: "0.1.0"}
	if got, want := b.String(), "Azula v0.1.0"; got != want {
		t.Errorf("String() = %q, want %q", got, want)
	}
}

func TestBuildInfo_Banner(t *testing.T) {
	b := BuildInfo{Name: "Azula", Version: "0.1.0"}
	want := fmt.Sprintf("Azula v0.1.0 (built for %s %s with %s)",
		runtime.GOOS, runtime.GOARCH, runtime.Version())

	if got := b.Banner(); got != want {
		t.Errorf("Banner() = %q, want %q", got, want)
	}
}

func TestBuildInfo_Full(t *testing.T) {
	b := BuildInfo{Name: "Azula", Version: "0.1.0", Author: "Team", Website: "https://azula.org/"}
	got := b.Full()

	if !strings.HasPrefix(got, b.Banner()) {
		t.Errorf("Full() must start with the banner: %q", got)
	}
	if !strings.HasSuffix(got, "\n\nby Team — https://azula.org/") {
		t.Errorf("Full() tail is wrong: %q", got)
	}
}

func TestBuildInfo_Semver_Valid(t *testing.T) {
	cases := []struct {
		version         string
		major, min, pat int
	}{
		{"0.1.0", 0, 1, 0},
		{"1.2.3", 1, 2, 3},
		{"2.41.7", 2, 41, 7},
		{"10.20.30", 10, 20, 30},
		{"v1.2.3", 1, 2, 3},
		{"1.2.3-rc.1", 1, 2, 3},
		{"1.2.3+build.5", 1, 2, 3},
		{"1.2.3-rc.1+build.5", 1, 2, 3},
		{"v0.0.0", 0, 0, 0},
	}
	for _, tc := range cases {
		t.Run(tc.version, func(t *testing.T) {
			maj, min, pat, err := BuildInfo{Version: tc.version}.Semver()
			if err != nil {
				t.Fatalf("unexpected error: %v", err)
			}
			if maj != tc.major || min != tc.min || pat != tc.pat {
				t.Errorf("got %d.%d.%d, want %d.%d.%d", maj, min, pat, tc.major, tc.min, tc.pat)
			}
		})
	}
}

// Regression (bug #8): Sscanf("%d.%d.%d") silently accepted trailing garbage.
func TestBuildInfo_Semver_Invalid(t *testing.T) {
	cases := []string{
		"",
		"1",
		"1.2",
		"1.2.3.4",  // extra component
		"1.2.3abc", // trailing garbage
		"a.b.c",
		"1.2.x",
		"1..3",
		".1.2",
		"1.2.",
		"1.2.3.",
		"1.-2.3",
		"1.2.+3",
		"-1.2.3",
		" 1.2.3",
		"1.2.3 ",
		"V1.2.3", // only lowercase "v" is accepted
		"vv1.2.3",
		"0x1.2.3",
		"99999999999999999999.0.0", // overflows int
	}
	for _, v := range cases {
		t.Run(fmt.Sprintf("%q", v), func(t *testing.T) {
			maj, min, pat, err := BuildInfo{Version: v}.Semver()
			if err == nil {
				t.Fatalf("Semver(%q) = %d.%d.%d, want error", v, maj, min, pat)
			}
			if maj != 0 || min != 0 || pat != 0 {
				t.Errorf("on error all numbers must be zero, got %d.%d.%d", maj, min, pat)
			}
			if !strings.Contains(err.Error(), "malformed version") {
				t.Errorf("error should mention malformed version: %v", err)
			}
		})
	}
}
