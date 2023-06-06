package ir

/*
#include <ffi_go/ir/deserializer.h>
*/
import "C"

import (
	"encoding/json"
	"strconv"
	"unsafe"

	"github.com/y-scope/clp-ffi-go/ffi"
)

// Deserializer exports functions to deserialize log events in an IR stream and also
// inspect the timestamp information of the stream. An Deserializer manages the
// current state of the IR stream, but management of the buffer containing the
// IR stream is left to the caller. Note that, failure to call [Close] will
// leak the underlying C-allocated memory.
// [DeserializePreamble] will return an Deserializer of the correct underlying type.
type Deserializer interface {
	DeserializeLogEventView(buf []byte) (*ffi.LogEventView, int, error)
	TimestampInfo() TimestampInfo
	Close() error
}

// DeserializePreamble attempts to read an IR stream preamble from buf, returning an
// IRDeserializer (of the correct stream encoding size), the position read to in buf
// (the end of the preamble), and an error. Note the metadata stored in the
// preamble is sparse and certain fields in TimestampInfo may be 0 value.
// Return values:
//   - nil == error: successful deserialize
//   - nil != error: IRDeserializer will be nil, pos may be non-zero for debugging purposes
//   - type [IRError]: CLP failed to successfully deserialize
//   - type from [encoding/json]: unmarshalling the metadata failed
func DeserializePreamble(buf []byte) (Deserializer, int, error) {
	var pos C.size_t
	var irEncoding C.int8_t
	var metadataType C.int8_t
	var metadataPos C.size_t
	var metadataSize C.uint16_t
	var logEventPtr unsafe.Pointer

	if err := IRError(C.ir_deserializer_deserialize_preamble(
		unsafe.Pointer(unsafe.SliceData(buf)),
		C.size_t(len(buf)),
		&pos,
		&irEncoding,
		&metadataType,
		&metadataPos,
		&metadataSize,
		&logEventPtr,
	)); Success != err {
		return nil, int(pos), err
	}

	if 1 != metadataType {
		return nil, int(pos), UnsupportedVersion
	}

	var metadata map[string]interface{}
	if err := json.Unmarshal(
		buf[metadataPos:metadataPos+C.size_t(metadataSize)],
		&metadata,
	); nil != err {
		return nil, int(pos), err
	}

	var tsInfo TimestampInfo
	if tsPat, ok := metadata["TIMESTAMP_PATTERN"].(string); ok {
		tsInfo.Pattern = tsPat
	}
	if tsSyn, ok := metadata["TIMESTAMP_PATTERN_SYNTAX"].(string); ok {
		tsInfo.PatternSyntax = tsSyn
	}
	if tzid, ok := metadata["TZ_ID"].(string); ok {
		tsInfo.TimeZoneId = tzid
	}

	var deserializer Deserializer
	if 1 == irEncoding {
		var refTs ffi.EpochTimeMs = 0
		if tsStr, ok := metadata["REFERENCE_TIMESTAMP"].(string); ok {
			if tsInt, err := strconv.ParseInt(tsStr, 10, 64); nil == err {
				refTs = ffi.EpochTimeMs(tsInt)
			}
		}
		deserializer = &fourByteDeserializer{commonDeseriealizer{tsInfo, logEventPtr}, refTs}
	} else {
		deserializer = &eightByteDeserializer{commonDeseriealizer{tsInfo, logEventPtr}}
	}

	return deserializer, int(pos), nil
}

// irStream contains fields common to all types of CLP IR streams. All streams
// must have information regarding their timestamp to make sense of the
// timestamp of the log events.
// [cptr] holds a reference to the underlying C object used for either encoding
// or decoding. Each ir stream owns a unique C object to allow concurrent
// encoding/decoding and all calls using a irStream instance will reuse this
// same object. [irStream.Close] will delete cptr, making it undefined to use
// the irStream after this call. Failure to call Close will leak the underlying
// C-allocated memory.
type commonDeseriealizer struct {
	tsInfo TimestampInfo
	cptr   unsafe.Pointer
}

// Close will delete cptr, making it undefined to use the irStream after this
// call. Failure to call Close will leak the underlying C-allocated memory.
func (self *commonDeseriealizer) Close() error {
	if nil != self.cptr {
		C.ir_deserializer_close(self.cptr)
		self.cptr = nil
	}
	return nil
}

// Returns the TimestampInfo of the irStream.
func (self commonDeseriealizer) TimestampInfo() TimestampInfo {
	return self.tsInfo
}

type eightByteDeserializer struct {
	commonDeseriealizer
}

// DeserializeLogEventView attempts to read the next log event from the IR
// stream in buf, returning the LogEventView, the position read to in buf (the
// end of the LogEventView in buf), and an error.
// Return values:
//   - nil == error: successful deserialize
//   - nil != error: ffi.LogEventView will be nil, position  may be non-zero
//     for debugging purposes
//   - [EOIR]: CLP found the IR stream EOF tag
//   - [IRError]: CLP failed to successfully deserialize
func (self *eightByteDeserializer) DeserializeLogEventView(
	buf []byte,
) (*ffi.LogEventView, int, error) {
	return deserializeLogEventView(self, buf)
}

// FourByteIrStream contains both a common CLP IR stream (irStream) and keeps
// track of the previous timestamp seen in the stream. Four byte encoding
// encodes log event timestamps as time deltas from the previous log event.
// Therefore, we must track the previous timestamp to be able to calculate the
// full timestamp of a log event.
type fourByteDeserializer struct {
	commonDeseriealizer
	prevTimestamp ffi.EpochTimeMs
}

// DeserializeLogEventView attempts to read the next log event from the IR
// stream in buf, returning the LogEventView, the position read to in buf (the
// end of the LogEventView in buf), and an error.
// Return values:
//   - nil == error: successful deserialize
//   - nil != error: ffi.LogEventView will be nil, position may be non-zero for
//     debugging purposes
//   - [EOIR]: CLP found the IR stream EOF tag
//   - [IRError]: CLP failed to successfully deserialize
func (self *fourByteDeserializer) DeserializeLogEventView(
	buf []byte,
) (*ffi.LogEventView, int, error) {
	return deserializeLogEventView(self, buf)
}

func deserializeLogEventView(
	deserializer Deserializer,
	buf []byte,
) (*ffi.LogEventView, int, error) {
	if 0 >= len(buf) {
		return nil, 0, IncompleteIR
	}
	var pos C.size_t
	var msg *C.char
	var msgSize C.size_t
	var event ffi.LogEventView

	var err error
	switch irs := deserializer.(type) {
	case *eightByteDeserializer:
		var timestamp C.int64_t
		err = IRError(C.ir_deserializer_deserialize_eight_byte_log_event(
			unsafe.Pointer(unsafe.SliceData(buf)),
			C.size_t(len(buf)),
			&pos,
			irs.cptr,
			&msg,
			&msgSize,
			&timestamp))
		if Success == err {
			event.Timestamp = ffi.EpochTimeMs(timestamp)
		}
	case *fourByteDeserializer:
		var timestampDelta C.int64_t
		err = IRError(C.ir_deserializer_deserialize_four_byte_log_event(
			unsafe.Pointer(unsafe.SliceData(buf)),
			C.size_t(len(buf)),
			&pos,
			irs.cptr,
			&msg,
			&msgSize,
			&timestampDelta))
		if Success == err {
			irs.prevTimestamp += ffi.EpochTimeMs(timestampDelta)
			event.Timestamp = irs.prevTimestamp
		}
	}
	if Success != err {
		return nil, int(pos), err
	}
	event.LogMessageView = unsafe.Slice((*byte)((unsafe.Pointer)(msg)), msgSize)
	return &event, int(pos), nil
}
