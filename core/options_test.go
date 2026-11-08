package core

import (
	"bytes"
	"errors"
	"flag"
	"fmt"
	"io"
	"os"
	"sort"
	"strings"
	"sync"
	"testing"
)

func parse(t *testing.T, args ...string) Options {
	t.Helper()
	o, err := NewParser(io.Discard).Parse(args)
	if err != nil {
		t.Fatalf("Parse(%v) failed: %v", args, err)
	}
	return o
}

// ═════════════════════════════════ Defaults ═══════════════════════════════════

func TestDefaults(t *testing.T) {
	want := Options{AutoStart: "events.stream", PcapBufSize: -1}

	if got := Defaults(); got != want {
		t.Errorf("Defaults() = %+v, want %+v", got, want)
	}
}

// Defaults() and the parser must agree: both derive from RegisterFlags.
func TestDefaults_MatchParsedDefaults(t *testing.T) {
	if got := parse(t); got != Defaults() {
		t.Errorf("Parse(nil) = %+v, Defaults() = %+v", got, Defaults())
	}
}

// ═══════════════════════════════ RegisterFlags ════════════════════════════════

func TestRegisterFlags_RegistersEveryFlag(t *testing.T) {
	want := []string{
		"autostart", "caplet", "caplets-path", "cpu-profile", "debug", "env-file",
		"eval", "gateway-override", "iface", "mem-profile", "no-colors",
		"no-history", "pcap-buf-size", "script", "silent", "version",
	}

	var o Options
	fs := flag.NewFlagSet("test", flag.ContinueOnError)
	o.RegisterFlags(fs)

	var got []string
	fs.VisitAll(func(f *flag.Flag) {
		got = append(got, f.Name)
		if f.Usage == "" {
			t.Errorf("flag -%s has no usage text", f.Name)
		}
	})
	sort.Strings(got)

	if !sameStrings(got, want) {
		t.Errorf("flags = %v\nwant    %v", got, want)
	}
}

// ═══════════════════════════════════ Parser ═══════════════════════════════════

func TestParser_AllFlags(t *testing.T) {
	got := parse(t,
		"-iface", "eth0",
		"-gateway-override", "10.0.0.1",
		"-autostart", "net.recon,net.probe",
		"-caplet", "my.cap",
		"-debug",
		"-silent",
		"-no-colors",
		"-no-history",
		"-version",
		"-env-file", "/tmp/env",
		"-eval", "set a b; set c d",
		"-cpu-profile", "cpu.prof",
		"-mem-profile", "mem.prof",
		"-caplets-path", "/opt/caplets",
		"-script", "s.js",
		"-pcap-buf-size", "4096",
	)

	want := Options{
		InterfaceName: "eth0",
		Gateway:       "10.0.0.1",
		AutoStart:     "net.recon,net.probe",
		Caplet:        "my.cap",
		Debug:         true,
		Silent:        true,
		NoColors:      true,
		NoHistory:     true,
		PrintVersion:  true,
		EnvFile:       "/tmp/env",
		Commands:      "set a b; set c d",
		CpuProfile:    "cpu.prof",
		MemProfile:    "mem.prof",
		CapletsPath:   "/opt/caplets",
		Script:        "s.js",
		PcapBufSize:   4096,
	}
	if got != want {
		t.Errorf("\n got: %+v\nwant: %+v", got, want)
	}
}

func TestParser_EqualsSyntaxAndExplicitFalse(t *testing.T) {
	o := parse(t, "-debug=false", "-iface=wlan0", "-silent=true")

	if o.Debug || !o.Silent || o.InterfaceName != "wlan0" {
		t.Errorf("o = %+v", o)
	}
}

func TestParser_DoubleDashPrefixIsAccepted(t *testing.T) {
	if o := parse(t, "--iface", "eth1", "--debug"); o.InterfaceName != "eth1" || !o.Debug {
		t.Errorf("o = %+v", o)
	}
}

func TestParser_UnknownFlag(t *testing.T) {
	var out bytes.Buffer

	o, err := NewParser(&out).Parse([]string{"-iface", "eth0", "-nope"})

	if err == nil {
		t.Fatal("expected an error")
	}
	if o != (Options{}) {
		t.Errorf("on error Options must be zero, got %+v", o)
	}
	if !strings.Contains(out.String(), "flag provided but not defined") {
		t.Errorf("output: %q", out.String())
	}
}

func TestParser_InvalidInt(t *testing.T) {
	if _, err := NewParser(io.Discard).Parse([]string{"-pcap-buf-size", "abc"}); err == nil {
		t.Error("expected an error for a non-numeric -pcap-buf-size")
	}
}

func TestParser_MissingValue(t *testing.T) {
	if _, err := NewParser(io.Discard).Parse([]string{"-iface"}); err == nil {
		t.Error("expected an error: -iface has no value")
	}
}

func TestParser_Help(t *testing.T) {
	var out bytes.Buffer

	_, err := NewParser(&out).Parse([]string{"-h"})

	if !errors.Is(err, flag.ErrHelp) {
		t.Fatalf("err = %v, want flag.ErrHelp", err)
	}
	for _, name := range []string{"-iface", "-autostart", "-pcap-buf-size", "-caplets-path"} {
		if !strings.Contains(out.String(), name) {
			t.Errorf("usage does not mention %s", name)
		}
	}
}

func TestNewParser_NilOutputIsSilent(t *testing.T) {
	p := NewParser(nil)

	if _, err := p.Parse([]string{"-bogus"}); err == nil {
		t.Error("expected an error")
	}
	if _, err := p.Parse([]string{"-h"}); !errors.Is(err, flag.ErrHelp) {
		t.Errorf("err = %v, want flag.ErrHelp", err)
	}
}

// Regression (bug #7): the FlagSet used to live in the Parser, so a second
// Parse call registered the flags again and panicked ("flag redefined").
func TestParser_CanBeReused(t *testing.T) {
	p := NewParser(io.Discard)

	a, err := p.Parse([]string{"-iface", "eth0"})
	if err != nil {
		t.Fatal(err)
	}
	b, err := p.Parse([]string{"-debug"})
	if err != nil {
		t.Fatal(err)
	}

	if a.InterfaceName != "eth0" || a.Debug {
		t.Errorf("a = %+v", a)
	}
	if b.InterfaceName != "" || !b.Debug {
		t.Errorf("b = %+v: state of the first call leaked", b)
	}
}

func TestParser_ConcurrentParse(t *testing.T) {
	p := NewParser(io.Discard)

	var wg sync.WaitGroup
	for i := 0; i < 8; i++ {
		wg.Add(1)
		go func(i int) {
			defer wg.Done()
			iface := fmt.Sprintf("eth%d", i)
			o, err := p.Parse([]string{"-iface", iface})
			if err != nil || o.InterfaceName != iface {
				t.Errorf("worker %d: o = %+v, err = %v", i, o, err)
			}
		}(i)
	}
	wg.Wait()
}

func TestParser_DoesNotTouchGlobalFlagSet(t *testing.T) {
	parse(t, "-iface", "eth0")

	if flag.Lookup("iface") != nil {
		t.Error("flags must not be registered in the global flag.CommandLine")
	}
}

func TestParser_PositionalArgumentStopsParsing(t *testing.T) {
	o := parse(t, "-iface", "eth0", "extra", "-debug")

	if o.InterfaceName != "eth0" || o.Debug {
		t.Errorf("parsing must stop at the first positional argument: %+v", o)
	}
}

// ═══════════════════════════════════ Validate ═════════════════════════════════

func TestOptions_Validate(t *testing.T) {
	cases := []struct {
		pcap    int
		wantErr bool
	}{
		{-1, false}, {0, false}, {4096, false}, {-2, true}, {-100, true},
	}
	for _, tc := range cases {
		o := Defaults()
		o.PcapBufSize = tc.pcap

		err := o.Validate()
		if (err != nil) != tc.wantErr {
			t.Errorf("PcapBufSize=%d: err = %v, wantErr = %v", tc.pcap, err, tc.wantErr)
		}
		if err != nil && !strings.Contains(err.Error(), "pcap-buf-size") {
			t.Errorf("error should name the option: %v", err)
		}
	}
}

func TestOptions_Validate_Defaults(t *testing.T) {
	if err := Defaults().Validate(); err != nil {
		t.Errorf("defaults must be valid: %v", err)
	}
}

func TestParser_ValidationErrorReturnsZeroOptions(t *testing.T) {
	for _, arg := range [][]string{{"-pcap-buf-size", "-5"}, {"-pcap-buf-size=-2"}} {
		o, err := NewParser(io.Discard).Parse(arg)
		if err == nil {
			t.Errorf("Parse(%v): expected a validation error", arg)
		}
		if o != (Options{}) {
			t.Errorf("Parse(%v): Options must be zero on error, got %+v", arg, o)
		}
	}
}

// ═══════════════════════ AutoStartModules / EvalCommands ══════════════════════

func TestOptions_AutoStartModules(t *testing.T) {
	cases := []struct {
		in   string
		want []string
	}{
		{"", nil},
		{"   ", nil},
		{"events.stream", []string{"events.stream"}},
		{"a,b,c", []string{"a", "b", "c"}},
		{" a , b ,, c ,", []string{"a", "b", "c"}},
		{",,", nil},
		{"a,a", []string{"a", "a"}}, // splitting only; deduplication is the caller's job
	}
	for _, tc := range cases {
		got := Options{AutoStart: tc.in}.AutoStartModules()
		if !sameStrings(got, tc.want) {
			t.Errorf("AutoStartModules(%q) = %q, want %q", tc.in, got, tc.want)
		}
	}
}

func TestOptions_AutoStartModules_Default(t *testing.T) {
	if got := Defaults().AutoStartModules(); !sameStrings(got, []string{"events.stream"}) {
		t.Errorf("got %q", got)
	}
}

func TestOptions_EvalCommands(t *testing.T) {
	cases := []struct {
		in   string
		want []string
	}{
		{"", nil},
		{"set a b", []string{"set a b"}},
		{"set a b; set c d", []string{"set a b", "set c d"}},
		{" ; set a b ;; ", []string{"set a b"}},
		{"a,b", []string{"a,b"}}, // comma is not a separator here
	}
	for _, tc := range cases {
		got := Options{Commands: tc.in}.EvalCommands()
		if !sameStrings(got, tc.want) {
			t.Errorf("EvalCommands(%q) = %q, want %q", tc.in, got, tc.want)
		}
	}
}

// ═══════════════════════════════ ParseOptions (os.Args) ═══════════════════════

func withOSArgs(t *testing.T, args ...string) {
	t.Helper()
	old := os.Args
	os.Args = append([]string{"azula"}, args...)
	t.Cleanup(func() { os.Args = old })
}

func TestParseOptions_ReadsOSArgs(t *testing.T) {
	withOSArgs(t, "-iface", "eth9", "-debug")

	o, err := ParseOptions()
	if err != nil {
		t.Fatal(err)
	}
	if o.InterfaceName != "eth9" || !o.Debug {
		t.Errorf("o = %+v", o)
	}
}

func TestParseOptions_NoArgsGivesDefaults(t *testing.T) {
	withOSArgs(t)

	o, err := ParseOptions()
	if err != nil || o != Defaults() {
		t.Errorf("o = %+v, err = %v", o, err)
	}
}

func TestParseOptions_ErrorsGoToStderrAndAreReturned(t *testing.T) {
	withOSArgs(t, "-bogus")

	f, err := os.CreateTemp(t.TempDir(), "stderr")
	if err != nil {
		t.Fatal(err)
	}
	oldStderr := os.Stderr
	os.Stderr = f
	defer func() { os.Stderr = oldStderr }()

	_, parseErr := ParseOptions()
	f.Close()

	if parseErr == nil {
		t.Fatal("expected an error")
	}
	data, _ := os.ReadFile(f.Name())
	if !strings.Contains(string(data), "flag provided but not defined") {
		t.Errorf("stderr: %q", data)
	}
}
