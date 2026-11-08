package core

import (
	"reflect"
	"testing"
)

type point struct{ X, Y int }

// Named type to verify that the ~ in Ordered works for derived types.
type label string

func TestUnique_Ints(t *testing.T) {
	cases := []struct {
		name string
		in   []int
		want []int
	}{
		{"nil", nil, []int{}},
		{"empty", []int{}, []int{}},
		{"single", []int{5}, []int{5}},
		{"no duplicates", []int{3, 1, 2}, []int{3, 1, 2}},
		{"keeps first occurrence order", []int{3, 1, 3, 2, 1}, []int{3, 1, 2}},
		{"all same", []int{7, 7, 7}, []int{7}},
		{"negatives and zero", []int{0, -1, 0, -1, 2}, []int{0, -1, 2}},
	}
	for _, tc := range cases {
		t.Run(tc.name, func(t *testing.T) {
			got := Unique(tc.in)
			if !reflect.DeepEqual(got, tc.want) {
				t.Errorf("Unique(%v) = %v, want %v", tc.in, got, tc.want)
			}
		})
	}
}

func TestUnique_ReturnsNonNilSlice(t *testing.T) {
	if got := Unique[int](nil); got == nil {
		t.Error("Unique(nil) must return an empty non-nil slice")
	}
}

func TestUnique_Strings(t *testing.T) {
	got := Unique([]string{"b", "a", "b", "", "a", ""})
	want := []string{"b", "a", ""}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("got %v, want %v", got, want)
	}
}

func TestUnique_ComparableStructs(t *testing.T) {
	got := Unique([]point{{1, 2}, {3, 4}, {1, 2}, {2, 1}, {3, 4}})
	want := []point{{1, 2}, {3, 4}, {2, 1}}
	if !reflect.DeepEqual(got, want) {
		t.Errorf("got %v, want %v", got, want)
	}
}

func TestUnique_DoesNotMutateInput(t *testing.T) {
	in := []int{3, 1, 3, 2}
	orig := append([]int(nil), in...)

	_ = Unique(in)

	if !reflect.DeepEqual(in, orig) {
		t.Errorf("input mutated: %v, was %v", in, orig)
	}
}

func TestUnique_ResultDoesNotAliasInput(t *testing.T) {
	in := []int{1, 2, 3}
	out := Unique(in)
	out[0] = 99

	if in[0] != 1 {
		t.Error("modifying the result changed the input")
	}
}

func TestUnique_Deterministic(t *testing.T) {
	in := make([]int, 1000)
	for i := range in {
		in[i] = (i * 7) % 37
	}
	first := Unique(in)

	for i := 0; i < 20; i++ {
		if got := Unique(in); !reflect.DeepEqual(got, first) {
			t.Fatalf("run %d produced a different order", i)
		}
	}
	if len(first) != 37 {
		t.Errorf("len = %d, want 37", len(first))
	}
}

func TestUniqueSorted(t *testing.T) {
	t.Run("ints", func(t *testing.T) {
		got := UniqueSorted([]int{3, 1, 3, 2, 1, -5})
		want := []int{-5, 1, 2, 3}
		if !reflect.DeepEqual(got, want) {
			t.Errorf("got %v, want %v", got, want)
		}
	})
	t.Run("strings", func(t *testing.T) {
		got := UniqueSorted([]string{"pear", "apple", "pear", "fig"})
		want := []string{"apple", "fig", "pear"}
		if !reflect.DeepEqual(got, want) {
			t.Errorf("got %v, want %v", got, want)
		}
	})
	t.Run("floats", func(t *testing.T) {
		got := UniqueSorted([]float64{2.5, -1, 2.5, 0})
		want := []float64{-1, 0, 2.5}
		if !reflect.DeepEqual(got, want) {
			t.Errorf("got %v, want %v", got, want)
		}
	})
	t.Run("named string type", func(t *testing.T) {
		got := UniqueSorted([]label{"b", "a", "b"})
		want := []label{"a", "b"}
		if !reflect.DeepEqual(got, want) {
			t.Errorf("got %v, want %v", got, want)
		}
	})
	t.Run("empty and nil", func(t *testing.T) {
		if got := UniqueSorted[int](nil); len(got) != 0 {
			t.Errorf("got %v", got)
		}
	})
	t.Run("does not mutate input", func(t *testing.T) {
		in := []int{3, 1, 2, 1}
		orig := append([]int(nil), in...)
		_ = UniqueSorted(in)
		if !reflect.DeepEqual(in, orig) {
			t.Errorf("input mutated: %v", in)
		}
	})
}

func TestUniqueInts_BackwardCompatibility(t *testing.T) {
	in := []int{3, 1, 3, 2, 1}

	if got, want := UniqueInts(in, true), []int{1, 2, 3}; !reflect.DeepEqual(got, want) {
		t.Errorf("sorted: got %v, want %v", got, want)
	}
	if got, want := UniqueInts(in, false), []int{3, 1, 2}; !reflect.DeepEqual(got, want) {
		t.Errorf("unsorted: got %v, want %v", got, want)
	}
	if got := UniqueInts(nil, false); len(got) != 0 {
		t.Errorf("nil input: got %v", got)
	}
}
