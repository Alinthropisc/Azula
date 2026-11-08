package js

import (
	"bytes"
	"compress/gzip"
	"crypto/md5"
	"crypto/sha1"
	"crypto/sha256"
	"encoding/base64"
	"encoding/hex"
	"io"
	"strings"
	"testing"
)

// ---------------------------------------------------------------------------
// Crypto
// ---------------------------------------------------------------------------

var hashers = map[string]func(string) []byte{
	"sha1":   func(s string) []byte { h := sha1.Sum([]byte(s)); return h[:] },
	"sha256": func(s string) []byte { h := sha256.Sum256([]byte(s)); return h[:] },
	"md5":    func(s string) []byte { h := md5.Sum([]byte(s)); return h[:] },
}

func TestCrypto_KnownVectors(t *testing.T) {
	vectors := []struct{ fn, in, want string }{
		{"sha1Hex", "", "da39a3ee5e6b4b0d3255bfef95601890afd80709"},
		{"sha1Hex", "abc", "a9993e364706816aba3e25717850c26c9cd0d89d"},
		{"sha256Hex", "", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"},
		{"sha256Hex", "abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"},
		{"md5Hex", "", "d41d8cd98f00b204e9800998ecf8427e"},
		{"md5Hex", "abc", "900150983cd24fb0d6963f7d28e17f72"},
	}
	for _, tc := range vectors {
		t.Run(tc.fn+"/"+tc.in, func(t *testing.T) {
			vm := newVM(t)
			setVars(t, vm, map[string]interface{}{"in": tc.in})
			if got := runString(t, vm, "Crypto."+tc.fn+"(in)"); got != tc.want {
				t.Errorf("got %s, want %s", got, tc.want)
			}
		})
	}
}

func TestCrypto_HexAndRawAgreeWithGo(t *testing.T) {
	inputs := []string{"", "abc", "héllo wörld", strings.Repeat("x", 10000)}

	for algo, sum := range hashers {
		for _, in := range inputs {
			vm := newVM(t)
			setVars(t, vm, map[string]interface{}{"in": in})

			wantRaw := string(sum(in))
			wantHex := hex.EncodeToString(sum(in))

			if got := runString(t, vm, "Crypto."+algo+"Hex(in)"); got != wantHex {
				t.Errorf("%sHex(len=%d) = %s, want %s", algo, len(in), got, wantHex)
			}
			// Legacy raw output. If this fails, raw bytes are NOT preserved
			// by otto strings (bug #8 confirmed): use the Hex variants.
			if got := runString(t, vm, "Crypto."+algo+"(in)"); got != wantRaw {
				t.Errorf("%s(len=%d): raw digest not preserved through otto (got %d bytes, want %d)",
					algo, len(in), len(got), len(wantRaw))
			}
		}
	}
}

// ---------------------------------------------------------------------------
// base64
// ---------------------------------------------------------------------------

func TestBtoaAtob(t *testing.T) {
	cases := []struct{ plain, enc string }{
		{"", ""},
		{"hello", "aGVsbG8="},
		{"héllo", base64.StdEncoding.EncodeToString([]byte("héllo"))},
	}
	for _, tc := range cases {
		vm := newVM(t)
		setVars(t, vm, map[string]interface{}{"plain": tc.plain, "enc": tc.enc})

		if got := runString(t, vm, "btoa(plain)"); got != tc.enc {
			t.Errorf("btoa(%q) = %q, want %q", tc.plain, got, tc.enc)
		}
		if got := runString(t, vm, "atob(enc)"); got != tc.plain {
			t.Errorf("atob(%q) = %q, want %q", tc.enc, got, tc.plain)
		}
		if got := runString(t, vm, "atob(btoa(plain))"); got != tc.plain {
			t.Errorf("round trip of %q = %q", tc.plain, got)
		}
	}
}

func TestAtob_InvalidInput(t *testing.T) {
	sink := captureLogs(t)
	vm := newVM(t)
	requireFailed(t, sink, run(t, vm, `atob("!!!not base64!!!")`), "atob")
	requireReported(t, sink, "could not decode string")
}

// ---------------------------------------------------------------------------
// textEncode / textDecode
// ---------------------------------------------------------------------------

func TestTextEncodeDecode(t *testing.T) {
	vm := newVM(t)
	setVars(t, vm, map[string]interface{}{"s": "héllo wörld"})

	if n := runInt(t, vm, `textEncode("abc").length`); n != 3 {
		t.Errorf("textEncode(abc).length = %d, want 3", n)
	}
	if n := runInt(t, vm, `textEncode("abc")[0]`); n != 'a' {
		t.Errorf("textEncode(abc)[0] = %d, want %d", n, 'a')
	}
	// Both calls happen in the same VM: this is the regression for values
	// that used to be created in a throw-away otto.New().
	if got := runString(t, vm, `textDecode(textEncode(s))`); got != "héllo wörld" {
		t.Errorf("round trip = %q", got)
	}
}

func TestTextDecode_RejectsNonBytes(t *testing.T) {
	sink := captureLogs(t)
	vm := newVM(t)
	requireFailed(t, sink, run(t, vm, `textDecode("abc")`), "textDecode(string)")
	requireReported(t, sink, "[]uint8")
}

// ---------------------------------------------------------------------------
// gzip
// ---------------------------------------------------------------------------

func gzipString(t *testing.T, s string) string {
	t.Helper()
	var buf bytes.Buffer
	w := gzip.NewWriter(&buf)
	if _, err := w.Write([]byte(s)); err != nil {
		t.Fatal(err)
	}
	if err := w.Close(); err != nil {
		t.Fatal(err)
	}
	return buf.String()
}

func gunzipString(t *testing.T, s string) string {
	t.Helper()
	r, err := gzip.NewReader(strings.NewReader(s))
	if err != nil {
		t.Fatalf("gzip.NewReader: %v", err)
	}
	b, err := io.ReadAll(r)
	if err != nil {
		t.Fatalf("gunzip: %v", err)
	}
	return string(b)
}

func TestGzipCompress_ProducesValidStream(t *testing.T) {
	for _, in := range []string{"", "hello", strings.Repeat("abc", 5000)} {
		vm := newVM(t)
		setVars(t, vm, map[string]interface{}{"in": in})

		compressed := runString(t, vm, `gzipCompress(in)`)
		if got := gunzipString(t, compressed); got != in {
			t.Errorf("Go could not read the stream (len=%d): got %d bytes", len(in), len(got))
		}
	}
}

func TestGzipDecompress_GoMadeStream(t *testing.T) {
	for _, in := range []string{"", "hello", "héllo wörld", strings.Repeat("z", 100000)} {
		vm := newVM(t)
		setVars(t, vm, map[string]interface{}{"blob": gzipString(t, in)})

		v := run(t, vm, `gzipDecompress(blob)`)
		if !v.IsString() {
			t.Fatalf("len=%d: result = %v, want a string", len(in), v)
		}
		if got, _ := v.ToString(); got != in {
			t.Errorf("len=%d: decompressed %d bytes, want %d", len(in), len(got), len(in))
		}
	}
}

// Regression for binary data inside JS strings: the gzip magic bytes
// (0x1f 0x8b) are not valid UTF-8. If this fails, bug #8 is real and the
// gzip builtins need a base64/hex transport.
func TestGzip_RoundTripInsideJS(t *testing.T) {
	vm := newVM(t)
	setVars(t, vm, map[string]interface{}{"in": "hello world, hello world"})
	if got := runString(t, vm, `gzipDecompress(gzipCompress(in))`); got != "hello world, hello world" {
		t.Errorf("round trip = %q", got)
	}
}

func TestGzipDecompress_InvalidInput(t *testing.T) {
	good := gzipString(t, strings.Repeat("hello ", 100))

	t.Run("not gzip", func(t *testing.T) {
		sink := captureLogs(t)
		vm := newVM(t)
		requireFailed(t, sink, run(t, vm, `gzipDecompress("hello")`), "not gzip")
		requireReported(t, sink, "could not create gzip reader")
	})

	t.Run("truncated stream", func(t *testing.T) {
		sink := captureLogs(t)
		vm := newVM(t)
		setVars(t, vm, map[string]interface{}{"blob": good[:len(good)-5]})
		requireFailed(t, sink, run(t, vm, `gzipDecompress(blob)`), "truncated")
		requireReported(t, sink, "could not decompress data")
	})
}

func TestGzipDecompress_SizeLimit(t *testing.T) {
	overrideInt64(t, &maxDecompressedSize, 100)

	t.Run("exactly at the limit", func(t *testing.T) {
		vm := newVM(t)
		setVars(t, vm, map[string]interface{}{"blob": gzipString(t, strings.Repeat("a", 100))})
		if got := runString(t, vm, `gzipDecompress(blob)`); len(got) != 100 {
			t.Errorf("got %d bytes, want 100", len(got))
		}
	})

	t.Run("one byte over", func(t *testing.T) {
		sink := captureLogs(t)
		vm := newVM(t)
		setVars(t, vm, map[string]interface{}{"blob": gzipString(t, strings.Repeat("a", 101))})
		requireFailed(t, sink, run(t, vm, `gzipDecompress(blob)`), "over limit")
		requireReported(t, sink, "exceeds")
	})

	t.Run("bomb", func(t *testing.T) {
		sink := captureLogs(t)
		vm := newVM(t)
		setVars(t, vm, map[string]interface{}{"blob": gzipString(t, strings.Repeat("a", 5<<20))})
		requireFailed(t, sink, run(t, vm, `gzipDecompress(blob)`), "bomb")
	})
}
