package core

import (
	"flag"
	"fmt"
	"io"
	"os"
	"strings"
)

// Options holds startup parameters (Value Object): filled by the parser and
// only read afterwards. Structural invariants are checked by Validate.
type Options struct {
	InterfaceName string
	Gateway       string
	Caplet        string
	AutoStart     string
	Debug         bool
	Silent        bool
	NoColors      bool
	NoHistory     bool
	PrintVersion  bool
	EnvFile       string
	Commands      string
	CpuProfile    string
	MemProfile    string
	CapletsPath   string
	Script        string
	PcapBufSize   int
}

// Defaults returns Options populated with default values.
func Defaults() Options {
	var o Options
	// flag.*Var stores the default into the target at registration time, so
	// registering on a throwaway FlagSet yields a struct with all defaults.
	// This keeps RegisterFlags the single source of truth for them.
	o.RegisterFlags(flag.NewFlagSet(Name, flag.ContinueOnError))
	return o
}

// RegisterFlags registers every option on the given FlagSet. Binding to an
// explicit FlagSet instead of the global flag.CommandLine inverts the
// dependency: parsing is testable and never touches global state.
func (o *Options) RegisterFlags(fs *flag.FlagSet) {
	fs.StringVar(&o.InterfaceName, "iface", "", "Network interface to bind to, if empty the default interface will be auto selected.")
	fs.StringVar(&o.Gateway, "gateway-override", "", "Use the provided IP address instead of the default gateway. If not specified or invalid, the default gateway will be used.")
	fs.StringVar(&o.AutoStart, "autostart", "events.stream", "Comma separated list of modules to auto start.")
	fs.StringVar(&o.Caplet, "caplet", "", "Read commands from this file and execute them in the interactive session.")
	fs.BoolVar(&o.Debug, "debug", false, "Print debug messages.")
	fs.BoolVar(&o.PrintVersion, "version", false, "Print the version and exit.")
	fs.BoolVar(&o.Silent, "silent", false, "Suppress all logs which are not errors.")
	fs.BoolVar(&o.NoColors, "no-colors", false, "Disable output color effects.")
	fs.BoolVar(&o.NoHistory, "no-history", false, "Disable interactive session history file.")
	fs.StringVar(&o.EnvFile, "env-file", "", "Load environment variables from this file if found, set to empty to disable environment persistence.")
	fs.StringVar(&o.Commands, "eval", "", "Run one or more commands separated by ; in the interactive session, used to set variables via command line.")
	fs.StringVar(&o.CpuProfile, "cpu-profile", "", "Write cpu profile `file`.")
	fs.StringVar(&o.MemProfile, "mem-profile", "", "Write memory profile to `file`.")
	fs.StringVar(&o.CapletsPath, "caplets-path", "", "Specify an alternative base path for caplets.")
	fs.StringVar(&o.Script, "script", "", "Load a session script.")
	fs.IntVar(&o.PcapBufSize, "pcap-buf-size", -1, "PCAP buffer size, leave to -1 for the default value.")
}

// Validate checks structural invariants only. It deliberately does NOT check
// that interfaces or files exist: environment state is the session's
// responsibility, not the settings struct's (SRP).
func (o Options) Validate() error {
	if o.PcapBufSize < -1 {
		return fmt.Errorf("pcap-buf-size: must be -1 (auto) or >= 0, got %d", o.PcapBufSize)
	}
	return nil
}

// AutoStartModules returns the normalized auto-start module list: split by
// comma, trimmed, without empty items.
func (o Options) AutoStartModules() []string {
	return splitAndClean(o.AutoStart, ",")
}

// EvalCommands returns the commands passed via -eval, split by semicolon.
func (o Options) EvalCommands() []string {
	return splitAndClean(o.Commands, ";")
}

func splitAndClean(s, sep string) []string {
	if strings.TrimSpace(s) == "" {
		return nil
	}
	parts := strings.Split(s, sep)
	out := make([]string, 0, len(parts))
	for _, part := range parts {
		if part = strings.TrimSpace(part); part != "" {
			out = append(out, part)
		}
	}
	return out
}

// Parser turns command-line arguments into Options. It is a type rather than
// a function so that parsing (a) does not depend on the global
// flag.CommandLine and (b) can gain new configuration sources without
// changing call sites.
//
// The parser is stateless: every Parse call builds a fresh FlagSet, so it is
// safe to call repeatedly and concurrently.
type Parser struct {
	output io.Writer
}

// NewParser creates a parser. Usage and error text go to output
// (nil means io.Discard).
func NewParser(output io.Writer) *Parser {
	if output == nil {
		output = io.Discard
	}
	return &Parser{output: output}
}

// Parse parses args (without the program name) and validates the result.
// Instead of a hidden os.Exit it returns an error and lets main decide the
// exit code. For -h/--help it returns flag.ErrHelp.
func (p *Parser) Parse(args []string) (Options, error) {
	var o Options
	fs := flag.NewFlagSet(Name, flag.ContinueOnError)
	fs.SetOutput(p.output)
	o.RegisterFlags(fs)

	if err := fs.Parse(args); err != nil {
		return Options{}, err
	}
	if err := o.Validate(); err != nil {
		return Options{}, err
	}
	return o, nil
}

// ParseOptions parses os.Args[1:]. The signature is kept for compatibility.
func ParseOptions() (Options, error) {
	return NewParser(os.Stderr).Parse(os.Args[1:])
}
