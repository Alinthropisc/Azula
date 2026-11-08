package core

import "sort"

// Ordered is a local copy of constraints.Ordered (cmp.Ordered in Go 1.21+),
// so the package does not depend on golang.org/x/exp for one type constraint.
type Ordered interface {
	~int | ~int8 | ~int16 | ~int32 | ~int64 | ~uint | ~uint8 | ~uint16 | ~uint32 | ~uint64 | ~uintptr | ~float32 | ~float64 | ~string
}

// Unique returns items without duplicates, preserving the order of first
// occurrence. Unlike the old map-iteration implementation, the result is
// deterministic. The input slice is never modified.
func Unique[T comparable](items []T) []T {
	seen := make(map[T]struct{}, len(items))
	out := make([]T, 0, len(items))

	for _, item := range items {
		if _, dup := seen[item]; dup {
			continue
		}
		seen[item] = struct{}{}
		out = append(out, item)
	}
	return out
}

// UniqueSorted is Unique followed by an ascending sort.
func UniqueSorted[T Ordered](items []T) []T {
	out := Unique(items)
	sort.Slice(out, func(i, j int) bool { return out[i] < out[j] })
	return out
}

// UniqueInts is kept for backward compatibility with existing modules.
//
// Deprecated: use Unique or UniqueSorted.
func UniqueInts(a []int, sorted bool) []int {
	if sorted {
		return UniqueSorted(a)
	}
	return Unique(a)
}
