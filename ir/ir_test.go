package ir

import (
	"github.com/y-scope/clp-ffi-go/ffi"
)

type testArg int

const (
	eightByteEncoding testArg = iota
	fourByteEncoding
	noCompression
	zstdCompression
)

var testArgStr = []string{
	"eightByteEncoding",
	"fourByteEncoding",
	"noCompression",
	"zstdCompression",
}

type testArgs struct {
	encoding    testArg
	compression testArg
	name        string
	filePath    string
}

type preambleFields struct {
	TimestampInfo
	prevTimestamp ffi.EpochTimeMs
}
