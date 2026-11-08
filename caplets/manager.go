package caplets

import (
	"fmt"
	"io/fs"
	"os"
	"path/filepath"
	"sort"
	"strings"
	"sync"

	"github.com/Alinthropisc/Azula/core/log"
)

// Manager encapsulates caplet discovery, loading and a read-through cache
// (explicit state instead of package globals: tests create isolated instances).
type Manager struct {
	cfg Config // immutable after NewManager, so it is read without a lock

	mu    sync.RWMutex
	cache map[string]*Caplet
}

// NewManager creates a manager on top of cfg. The configuration is copied:
// later mutations of the passed value are invisible to the manager.
func NewManager(cfg Config) *Manager {
	cfg.LoadPaths = append([]string(nil), cfg.LoadPaths...)
	return &Manager{
		cfg:   cfg,
		cache: make(map[string]*Caplet),
	}
}

// Config returns a copy of the manager's configuration.
func (m *Manager) Config() Config {
	cfg := m.cfg
	cfg.LoadPaths = append([]string(nil), m.cfg.LoadPaths...)
	return cfg
}

// LoadPaths returns the search paths, highest priority first (a copy).
func (m *Manager) LoadPaths() []string {
	return append([]string(nil), m.cfg.LoadPaths...)
}

// Invalidate drops the cache (call after caplets.update).
func (m *Manager) Invalidate() {
	m.mu.Lock()
	m.cache = make(map[string]*Caplet)
	m.mu.Unlock()
}

// Load returns a caplet by name: with or without the suffix, relative or
// absolute. Repeated calls are served from the cache, so every caller gets
// the same *Caplet; treat it as read-only.
func (m *Manager) Load(name string) (*Caplet, error) {
	name = strings.TrimSpace(name)
	if name == "" {
		return nil, fmt.Errorf("caplet name is empty")
	}

	// Canonical form: forward slashes, no suffix. "foo" and "foo.cap" are the
	// same caplet and get the same Name and the same cache key.
	baseName := strings.TrimSuffix(filepath.ToSlash(name), Suffix)
	key := baseName + Suffix

	m.mu.RLock()
	cached, found := m.cache[key]
	m.mu.RUnlock()
	if found {
		log.Tracing("caplet %s: cache hit", key)
		return cached, nil
	}

	caplet, err := m.load(key, baseName)
	if err != nil {
		return nil, err
	}

	m.mu.Lock()
	defer m.mu.Unlock()
	// Two goroutines may have loaded the same caplet concurrently; the first
	// one wins so that all callers share one instance.
	if existing, ok := m.cache[key]; ok {
		return existing, nil
	}
	m.cache[key] = caplet
	return caplet, nil
}

func (m *Manager) load(fileName, baseName string) (*Caplet, error) {
	for _, path := range m.candidates(fileName) {
		info, err := os.Stat(path)
		if err != nil {
			log.Tracing("caplet %s: skipping %s: %v", fileName, path, err)
			continue
		}
		if info.IsDir() { // a directory named "x.cap" is not a caplet
			continue
		}

		caplet := NewCaplet(baseName, path, info.Size())
		if err := caplet.Script.read(); err != nil {
			return nil, err
		}

		// A caplet with a dedicated folder also brings its secondary .cap/.js
		// files. Same rule as the original: the name contains a separator.
		if strings.ContainsAny(baseName, `/\`) {
			m.loadAuxScripts(&caplet, path)
		}
		return &caplet, nil
	}
	return nil, fmt.Errorf("caplet %s not found (searched in %s)",
		fileName, strings.Join(m.cfg.LoadPaths, ", "))
}

// candidates returns candidate file paths in LoadPaths priority order.
// All paths are cleaned so that comparisons like "is this the main file?"
// are reliable.
func (m *Manager) candidates(fileName string) []string {
	if filepath.IsAbs(fileName) {
		return []string{filepath.Clean(fileName)}
	}
	out := make([]string, 0, len(m.cfg.LoadPaths))
	for _, dir := range m.cfg.LoadPaths {
		out = append(out, filepath.Join(dir, fileName))
	}
	return out
}

// loadAuxScripts reads the secondary scripts from the caplet's folder.
// A broken file is not fatal: it is reported and skipped. The order is
// deterministic because os.ReadDir sorts by file name.
func (m *Manager) loadAuxScripts(caplet *Caplet, mainFile string) {
	dir := filepath.Dir(mainFile)
	entries, err := os.ReadDir(dir)
	if err != nil {
		log.Warning("caplet %s: cannot list %s: %v", caplet.Name, dir, err)
		return
	}

	for _, entry := range entries {
		if entry.IsDir() {
			continue
		}
		subFile := filepath.Join(dir, entry.Name())
		if subFile == mainFile || !isAuxScript(subFile) {
			continue
		}
		info, err := entry.Info()
		if err != nil {
			log.Warning("caplet %s: cannot stat %s: %v", caplet.Name, subFile, err)
			continue
		}
		script := newScript(subFile, info.Size())
		if err := script.read(); err != nil {
			log.Warning("caplet %s: %v", caplet.Name, err)
			continue
		}
		caplet.Scripts = append(caplet.Scripts, script)
	}
}

func isAuxScript(path string) bool {
	switch strings.ToLower(filepath.Ext(path)) {
	case Suffix, ".js":
		return true
	}
	return false
}

// LoadErrors aggregates the failures of a partially successful List. It keeps
// the individual errors, so callers can use errors.Is/As on them (Go 1.20+).
type LoadErrors []error

func (e LoadErrors) Error() string {
	msgs := make([]string, len(e))
	for i, err := range e {
		msgs[i] = err.Error()
	}
	return fmt.Sprintf("%d caplets failed to load: %s", len(e), strings.Join(msgs, "; "))
}

// Unwrap exposes the individual errors to errors.Is and errors.As.
func (e LoadErrors) Unwrap() []error { return e }

// List returns every available caplet sorted by name. Caplets that fail to
// load are skipped; their errors are returned as LoadErrors together with
// the partial result. When the same name exists in several search paths,
// the highest-priority path wins.
func (m *Manager) List() ([]*Caplet, error) {
	defer log.TraceFunc("caplets.List")()

	var (
		out  = make([]*Caplet, 0)
		errs LoadErrors
		seen = make(map[string]struct{})
	)

	for _, dir := range m.cfg.LoadPaths {
		for _, name := range discover(dir) {
			if _, dup := seen[name]; dup {
				continue
			}
			seen[name] = struct{}{}

			caplet, err := m.Load(name)
			if err != nil {
				errs = append(errs, err)
				continue
			}
			out = append(out, caplet)
		}
	}

	sort.Slice(out, func(i, j int) bool { return out[i].Name < out[j].Name })

	if len(errs) == 0 {
		return out, nil // explicit nil: avoids the typed-nil-error pitfall
	}
	return out, errs
}

// discover returns the names of the caplets found in dir: top-level files
// ("foo") and files one level down ("dir/foo"). It uses os.ReadDir instead of
// filepath.Glob, because a directory name containing '[', '*' or '?' would
// be misread as a glob pattern.
func discover(dir string) []string {
	entries, err := os.ReadDir(dir)
	if err != nil {
		log.Tracing("caplets: cannot list %s: %v", dir, err) // missing dirs are normal
		return nil
	}

	var names []string
	for _, entry := range entries {
		if isDirEntry(dir, entry) {
			sub, err := os.ReadDir(filepath.Join(dir, entry.Name()))
			if err != nil {
				continue
			}
			for _, s := range sub {
				if !isDirEntry(filepath.Join(dir, entry.Name()), s) && isCapName(s.Name()) {
					names = append(names, entry.Name()+"/"+strings.TrimSuffix(s.Name(), Suffix))
				}
			}
			continue
		}
		if isCapName(entry.Name()) {
			names = append(names, strings.TrimSuffix(entry.Name(), Suffix))
		}
	}
	return names
}

func isCapName(name string) bool {
	return strings.HasSuffix(name, Suffix) && len(name) > len(Suffix)
}

// isDirEntry reports whether the entry is a directory, following symlinks
// (the original Glob-based lookup followed them too).
func isDirEntry(parent string, e fs.DirEntry) bool {
	if e.Type()&fs.ModeSymlink != 0 {
		info, err := os.Stat(filepath.Join(parent, e.Name()))
		return err == nil && info.IsDir()
	}
	return e.IsDir()
}

// ──────────────────── Package facades (compatibility) ─────────────────────────

// Load is a facade over Default().Load.
func Load(name string) (*Caplet, error) {
	return Default().Load(name)
}

// List is a facade over Default().List. Partial errors go to the log (the
// original printed "wtf: %v" to stderr).
func List() []*Caplet {
	caps, err := Default().List()
	if err != nil {
		log.Warning("%v", err)
	}
	return caps
}

// Invalidate is a facade over Default().Invalidate.
func Invalidate() {
	Default().Invalidate()
}
