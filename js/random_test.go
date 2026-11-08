package js

import (
	"net"
	"strings"
	"testing"
	"unicode/utf8"
)

func TestRandomString_LengthAndCharset(t *testing.T) {
	r := randomPackage{}

	got := r.String(200, "abc")
	if len(got) != 200 {
		t.Fatalf("len = %d, want 200", len(got))
	}
	seen := map[rune]bool{}
	for _, c := range got {
		if !strings.ContainsRune("abc", c) {
			t.Fatalf("rune %q outside the charset", c)
		}
		seen[c] = true
	}
	if len(seen) != 3 {
		t.Errorf("only %d of 3 runes used in 200 draws", len(seen))
	}
}

func TestRandomString_MultibyteCharset(t *testing.T) {
	got := randomPackage{}.String(50, "äöü")
	if n := utf8.RuneCountInString(got); n != 50 {
		t.Errorf("rune count = %d, want 50", n)
	}
	if !utf8.ValidString(got) {
		t.Error("result is not valid UTF-8")
	}
}

func TestRandomString_EdgeCases(t *testing.T) {
	r := randomPackage{}
	cases := []struct {
		name    string
		size    int
		charset string
	}{
		{"empty charset", 10, ""},
		{"zero size", 0, "abc"},
		{"negative size", -5, "abc"},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			// Must not panic (the original panicked in rand.Intn(0)).
			if got := r.String(tc.size, tc.charset); got != "" {
				t.Errorf("got %q, want empty string", got)
			}
		})
	}
	if got := r.String(5, "x"); got != "xxxxx" {
		t.Errorf("single-rune charset: got %q", got)
	}
}

func TestRandomString_NotConstant(t *testing.T) {
	const charset = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
	r := randomPackage{}
	if a, b := r.String(32, charset), r.String(32, charset); a == b {
		t.Errorf("two 32-char random strings are equal: %q", a)
	}
}

func TestRandomMac_LocallyAdministeredUnicast(t *testing.T) {
	r := randomPackage{}
	seen := map[string]bool{}

	for i := 0; i < 200; i++ {
		mac := r.Mac()
		hw, err := net.ParseMAC(mac)
		if err != nil {
			t.Fatalf("Mac() = %q is not a valid MAC: %v", mac, err)
		}
		if len(hw) != 6 {
			t.Fatalf("Mac() = %q has %d bytes", mac, len(hw))
		}
		if hw[0]&0x02 == 0 {
			t.Fatalf("Mac() = %q: locally-administered bit not set", mac)
		}
		if hw[0]&0x01 != 0 {
			t.Fatalf("Mac() = %q: multicast bit set", mac)
		}
		seen[mac] = true
	}
	if len(seen) < 195 {
		t.Errorf("only %d unique MACs out of 200", len(seen))
	}
}

func TestRandom_ThroughJS(t *testing.T) {
	vm := newVM(t)

	if n := runInt(t, vm, `random.String(8, "ab").length`); n != 8 {
		t.Errorf("random.String length = %d, want 8", n)
	}
	mac := runString(t, vm, `random.Mac()`)
	if _, err := net.ParseMAC(mac); err != nil {
		t.Errorf("random.Mac() = %q: %v", mac, err)
	}
}
