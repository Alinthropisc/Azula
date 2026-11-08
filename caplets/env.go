package caplets

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"
	"sync"

	"github.com/Alinthropisc/Azula/core/log"
)

const (
	// EnvPrefix prefixes every environment variable of the project.
	EnvPrefix = "AZULA_"
	// EnvVarName lists extra caplet search paths (os.PathListSeparator-separated).
	EnvVarName = EnvPrefix + "CAPSPATH"
	// legacyEnvVarName is the pre-rebranding name, read as a fallback so
	// existing users migrate smoothly.
	legacyEnvVarName = "CAPSPATH"
	// Suffix is the caplet file extension.
	Suffix = ".cap"
)

// InstallArchive is the URL of the official caplets archive (caplets.update).
// It is a variable, not a constant, so a fork or mirror can be set at link time:
//
//	go build -ldflags "-X <module>/caplets.InstallArchive=https://mirror/..."
//
// NOTE: InstallPathArchive below assumes the archive unpacks into
// "caplets-master" (branch "master"). A mirror using another branch name
// unpacks elsewhere, so adjust Config.InstallPathArchive accordingly.
var InstallArchive = "https://github.com/azula-project/caplets/archive/master.zip"

// Config is an immutable snapshot of the caplet environment paths
// (Value Object): built once by NewConfig, only read afterwards.
type Config struct {
	InstallBase        string   // installation base directory
	InstallPathArchive string   // directory of the unpacked archive
	InstallPath        string   // final installation directory
	ArchivePath        string   // temporary archive file used by update
	LoadPaths          []string // search paths, highest priority first
}

// NewConfig builds a configuration from the base directory and the process
// environment. Path normalization errors are returned, not swallowed.
func NewConfig(base string) (Config, error) {
	cfg := Config{
		InstallBase:        base,
		InstallPathArchive: filepath.Join(base, "caplets-master"),
		InstallPath:        filepath.Join(base, "caplets"),
		ArchivePath:        filepath.Join(os.TempDir(), "azula_caplets.zip"),
	}

	candidates := []string{"./", "./caplets/", cfg.InstallPath}
	if home := userHomeDir(); home != "" {
		candidates = append(candidates, filepath.Join(home, "caplets"))
	}
	// Extra paths from the environment: the new name first, then legacy.
	candidates = appendEnvPaths(candidates, os.Getenv(EnvVarName))
	candidates = appendEnvPaths(candidates, os.Getenv(legacyEnvVarName))

	// Normalize to absolute paths and drop duplicates; the first occurrence
	// (highest priority) wins.
	seen := make(map[string]struct{}, len(candidates))
	loadPaths := make([]string, 0, len(candidates))
	for _, path := range candidates {
		abs, err := filepath.Abs(path)
		if err != nil {
			return Config{}, fmt.Errorf("invalid load path %q: %w", path, err)
		}
		if _, dup := seen[abs]; dup {
			continue
		}
		seen[abs] = struct{}{}
		loadPaths = append(loadPaths, abs)
	}

	cfg.LoadPaths = loadPaths
	return cfg, nil
}

func appendEnvPaths(paths []string, value string) []string {
	for _, path := range strings.Split(value, string(os.PathListSeparator)) {
		if path = strings.TrimSpace(path); path != "" {
			paths = append(paths, path)
		}
	}
	return paths
}

// userHomeDir returns the user's home directory. A failure is not fatal
// (~/caplets is optional), so it is only traced. os.UserHomeDir replaces the
// archived mitchellh/go-homedir dependency.
func userHomeDir() string {
	home, err := os.UserHomeDir()
	if err != nil {
		log.Tracing("caplets: home dir not resolved: %v", err)
		return ""
	}
	return home
}

// ───────────────── Default manager + legacy API compatibility ─────────────────

var (
	defMu      sync.RWMutex
	defManager *Manager
)

// Deprecated: global mirrors of the default configuration, kept for existing
// modules (modules/caplets reads them on update). They are written under
// defMu by Setup but read without a lock, so call Setup only during startup.
// New code should use Default().Config().
var (
	InstallBase        string
	InstallPathArchive string
	InstallPath        string
	ArchivePath        string
	LoadPaths          []string
)

// Setup rebuilds the configuration and the default manager. Errors are now
// really returned (the original always returned nil).
func Setup(base string) error {
	cfg, err := NewConfig(base)
	if err != nil {
		return err
	}
	setDefaultManager(NewManager(cfg))
	return nil
}

func setDefaultManager(m *Manager) {
	defMu.Lock()
	defer defMu.Unlock()

	defManager = m

	cfg := m.Config()
	InstallBase = cfg.InstallBase
	InstallPathArchive = cfg.InstallPathArchive
	InstallPath = cfg.InstallPath
	ArchivePath = cfg.ArchivePath
	LoadPaths = cfg.LoadPaths
}

// Default returns the default manager (never nil).
func Default() *Manager {
	defMu.RLock()
	defer defMu.RUnlock()
	return defManager
}

func init() {
	// Same as before: configured with defaults at package load. The logger is
	// still a no-op here; the fallback only guarantees a usable state.
	if err := Setup(defaultInstallBase()); err != nil {
		log.Error("caplets: default setup failed: %v", err)
		setDefaultManager(NewManager(Config{}))
	}
}
