// File: js/crypto.go
package js

import (
	"crypto/md5"  // #nosec G501 -- exposed for legacy protocols
	"crypto/sha1" // #nosec G505 -- exposed for legacy protocols
	"crypto/sha256"
	"encoding/hex"
)

func sha1Sum(b []byte) []byte   { s := sha1.Sum(b); return s[:] }
func sha256Sum(b []byte) []byte { s := sha256.Sum256(b); return s[:] }
func md5Sum(b []byte) []byte    { s := md5.Sum(b); return s[:] }

// rawString keeps the legacy output format. NOTE: arbitrary bytes inside a
// JS string are not binary-safe, prefer the *Hex variants in new scripts.
func rawString(b []byte) string { return string(b) }

// digest builds a hash builtin from a sum function and an output encoder.
func digest(fn string, sum func([]byte) []byte, encode func([]byte) string) builtin {
	return unaryString(fn, func(s string) (interface{}, error) {
		return encode(sum([]byte(s))), nil
	})
}

var (
	cryptoSha1   = digest("Crypto.sha1", sha1Sum, rawString)
	cryptoSha256 = digest("Crypto.sha256", sha256Sum, rawString)
	cryptoMD5    = digest("Crypto.md5", md5Sum, rawString)

	cryptoSha1Hex   = digest("Crypto.sha1Hex", sha1Sum, hex.EncodeToString)
	cryptoSha256Hex = digest("Crypto.sha256Hex", sha256Sum, hex.EncodeToString)
	cryptoMD5Hex    = digest("Crypto.md5Hex", md5Sum, hex.EncodeToString)
)
