// File: js/azula.go
package js

import "github.com/Alinthropisc/Azula/core"

// newAzulaPackage builds the global `azula` object exposed to scripts:
//
//	azula.name, azula.version, azula.author, azula.website
//
// A plain map is used (not a struct) so that the lowercase keys are exactly
// what scripts see, independent of how otto maps struct fields.
// Values come from core.Info(), so -ldflags -X overrides are honoured.
func newAzulaPackage() map[string]interface{} {
	info := core.Info()
	return map[string]interface{}{
		"name":    info.Name,
		"version": info.Version,
		"author":  info.Author,
		"website": info.Website,
	}
}
