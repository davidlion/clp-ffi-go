package search

import "github.com/y-scope/clp-ffi-go/ffi"

type WildcardQuery struct {
	Query         string
	CaseSensitive bool
}

type TimestampInterval struct {
	Lower ffi.EpochTimeMs
	Upper ffi.EpochTimeMs
}

// func wildcard_match(target string, queries []WildcardQuery) *WildcardQuery {
// 	for _, query := range queries {
// 		if false {
// 			return &query
// 		}
// 	}
// 	return nil
// }
