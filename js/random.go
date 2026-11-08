// File: js/random.go
package js

import (
	"crypto/rand"
	"math/big"
	"net"

	"github.com/Alinthropisc/Azula/log"
	"github.com/Alinthropisc/Azula/network"
)

type randomPackage struct{}

// String returns a cryptographically secure random string made of charset
// runes. Fixes over the original:
//   - math/rand replaced by crypto/rand (predictable PRNG in a security tool);
//   - empty charset / size <= 0 return "" instead of panicking in rand.Intn(0).
func (randomPackage) String(size int, charset string) string {
	runes := []rune(charset)
	if len(runes) == 0 || size <= 0 {
		return ""
	}

	out := make([]rune, size)
	limit := big.NewInt(int64(len(runes)))
	for i := range out {
		n, err := rand.Int(rand.Reader, limit)
		if err != nil {
			log.Error("random.String: crypto/rand failure: %v", err)
			return ""
		}
		out[i] = runes[n.Int64()]
	}
	return string(out)
}

// Mac returns a random locally administered unicast MAC address.
// The 0x02 bit is forced, so the address never impersonates a vendor OUI and
// cannot collide with real hardware; the multicast bit is cleared.
func (randomPackage) Mac() string {
	hw := make([]byte, 6)
	if _, err := rand.Read(hw); err != nil {
		log.Error("random.Mac: crypto/rand failure: %v", err)
		return ""
	}
	hw[0] = (hw[0] | 0x02) & 0xfe
	return network.NormalizeMac(net.HardwareAddr(hw).String())
}
