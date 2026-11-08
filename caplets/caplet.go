package caplets

import (
	"bufio"
	"errors"
	"fmt"
	"os"
	"path/filepath"
	"strconv"
	"strings"

	"github.com/Alinthropisc/Azula/core/log"
)

const (
	// maxLineSize is the longest accepted script line. bufio.Scanner's
	// default (64 KiB) rejects long one-liners such as minified JS.
	maxLineSize = 4 * 1024 * 1024
	utf8BOM     = "\ufeff"
)

// Script is a file with commands (Value Object).
type Script struct {
	Path string   `json:"path"`
	Size int64    `json:"size"`
	Code []string `json:"code"`
}

func newScript(path string, size int64) Script {
	return Script{
		Path: path,
		Size: size,
		Code: make([]string, 0),
	}
}

// read loads the script line by line. Code is replaced only on success, so a
// failed or repeated read never leaves partial or duplicated state.
func (s *Script) read() error {
	file, err := os.Open(s.Path)
	if err != nil {
		return fmt.Errorf("error reading script %s: %w", s.Path, err)
	}
	defer file.Close()

	code := make([]string, 0, 64)

	scanner := bufio.NewScanner(file)
	scanner.Buffer(make([]byte, 0, 64*1024), maxLineSize)
	for first := true; scanner.Scan(); first = false {
		line := scanner.Text() // ScanLines already strips a trailing "\r"
		if first {
			// A BOM would hide a leading '#' and turn a comment into a command.
			line = strings.TrimPrefix(line, utf8BOM)
		}
		code = append(code, line)
	}
	if err := scanner.Err(); err != nil {
		return fmt.Errorf("error reading script %s: %w", s.Path, err)
	}

	s.Code = code
	return nil
}

// Caplet is a main script plus the secondary .cap/.js files of its folder.
type Caplet struct {
	Script
	Name    string   `json:"name"`
	Scripts []Script `json:"scripts"`
}

// NewCaplet creates a caplet without touching the disk (reading is done by
// the Manager).
func NewCaplet(name string, path string, size int64) Caplet {
	return Caplet{
		Script:  newScript(path, size),
		Name:    name,
		Scripts: make([]Script, 0),
	}
}

// Eval feeds every command line of the caplet to lineCb, substituting $0..$N
// from argv, with the working directory temporarily switched to the caplet's
// folder (include directives and proxy modules rely on that). argv may be nil.
//
// NOTE: the working directory is process-global. Eval may be nested (a
// callback may run another caplet), but two goroutines evaluating caplets
// from different folders at the same time will still see each other's cwd,
// exactly like the original implementation.
func (c *Caplet) Eval(argv []string, lineCb func(line string) error) error {
	if lineCb == nil {
		return errors.New("caplet: nil line callback")
	}
	defer log.TraceFunc("caplets.Eval")()

	return withWorkdir(filepath.Dir(c.Path), func() error {
		for n, line := range c.Code {
			if isSkippable(line) {
				continue
			}
			line = interpolate(line, argv)
			log.Tracing("caplet %s:%d: %s", c.Name, n+1, line)

			if err := lineCb(line); err != nil {
				return err
			}
		}
		return nil
	})
}

// isSkippable reports whether a line is blank or a comment (indented
// comments included). The line itself is passed on unchanged.
func isSkippable(line string) bool {
	t := strings.TrimSpace(line)
	return t == "" || t[0] == '#'
}

// interpolate substitutes positional arguments $0..$N in a single pass.
//
// The original applied strings.Replace sequentially, which corrupted
// multi-digit indexes ("$10" became "<argv[1]>0") and re-expanded "$N"
// sequences found inside argument values. Indexes outside argv are kept as is.
func interpolate(line string, argv []string) string {
	if len(argv) == 0 || !strings.Contains(line, "$") {
		return line
	}

	var b strings.Builder
	b.Grow(len(line))

	for i := 0; i < len(line); {
		if line[i] != '$' {
			b.WriteByte(line[i])
			i++
			continue
		}
		j := i + 1
		for j < len(line) && line[j] >= '0' && line[j] <= '9' {
			j++
		}
		if j == i+1 { // '$' without digits is a literal
			b.WriteByte('$')
			i++
			continue
		}
		idx, err := strconv.Atoi(line[i+1 : j])
		if err != nil || idx >= len(argv) {
			b.WriteString(line[i:j])
		} else {
			b.WriteString(argv[idx])
		}
		i = j
	}
	return b.String()
}

// withWorkdir runs fn inside dir and restores the previous directory.
//
// No lock is held while fn runs: fn is arbitrary user code that may call
// withWorkdir again (nested caplets), and a non-reentrant mutex held across
// the callback would deadlock. Nested calls are safe because each level
// restores exactly the directory it found.
func withWorkdir(dir string, fn func() error) (err error) {
	prev, err := os.Getwd()
	if err != nil {
		return fmt.Errorf("cannot get working directory: %w", err)
	}
	if err := os.Chdir(dir); err != nil {
		return fmt.Errorf("cannot chdir to %s: %w", dir, err)
	}
	defer func() {
		if rerr := os.Chdir(prev); rerr != nil {
			log.Warning("cannot restore working directory %s: %v", prev, rerr)
			// A corrupted cwd is worth reporting, but never mask fn's own error.
			if err == nil {
				err = fmt.Errorf("cannot restore working directory %s: %w", prev, rerr)
			}
		}
	}()

	return fn()
}
