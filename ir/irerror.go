package ir

// IRError mirrors cpp type IRErrorCode defined in:
// clp/components/core/src/ffi/ir_stream/decoding_methods.hpp
//
//go:generate stringer -type=IRError
type IRError int

const (
	Success IRError = iota
	DecodeError
	EOIR
	CorruptedIR
	CorruptedMetadata
	IncompleteIR
	UnsupportedVersion
	EncoderError // not from clp
)

func (self IRError) Error() string {
	return self.String()
}
