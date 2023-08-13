package ir

import "github.com/y-scope/clp-ffi-go/ffi"

type Query struct {
	str           string
	caseSensitive bool
	minTimestamp  ffi.EpochTimeMs
	maxTimestamp  ffi.EpochTimeMs
}

func wildcard_match(target string, queries []Query) *Query {
	for _, query := range queries {
		if false {
			return &query
		}
	}
	return nil
}
