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

// A Deserializer exports functions to deserialize log events from a CLP IR byte
// stream. Deserializatoin functions take an IR buffer as input, but how that
// buffer is materialized is left to the user. These functions return views
// (slices) of the log events extracted from the IR. Each Deserializer owns its
// own unique underlying memory for the views it produces/returns. This memory
// is reused for each view, so to persist the contents the memory must be copied
// into another object. Close must be called to free the underlying memory and
// failure to do so will result in a memory leak.
type Deserializer interface {
	DeserializeLogEvent(buf []byte) (*ffi.LogEventView, int, error)
	TimestampInfo() TimestampInfo
	Close() error
}

// DeserializePreamble attempts to read an IR stream preamble from buf,
// returning an Deserializer (of the correct stream encoding size), the position
// read to in buf (the end of the preamble), and an error. Note the metadata
// stored in the preamble is sparse and certain fields in TimestampInfo may be 0
// value. On error returns:
//   - nil Deserializer
//   - the position may still be non-zero for debugging purposes
//   - [IRError] error: CLP failed to successfully deserialize
//   - [encoding/json] error: unmarshalling the metadata failed
func DeserializePreamble(buf []byte) (Deserializer, int, error) {
	var pos C.size_t
	var irEncoding C.int8_t
	var metadataType C.int8_t
	var metadataPos C.size_t
	var metadataSize C.uint16_t
	var cptr unsafe.Pointer

	if err := IRError(C.ir_deserializer_deserialize_preamble(
		unsafe.Pointer(unsafe.SliceData(buf)),
		C.size_t(len(buf)),
		&pos,
		&irEncoding,
		&metadataType,
		&metadataPos,
		&metadataSize,
		&cptr,
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
		deserializer = &fourByteDeserializer{commonDeserializer{tsInfo, cptr}, refTs}
	} else {
		deserializer = &eightByteDeserializer{commonDeserializer{tsInfo, cptr}}
	}

	return deserializer, int(pos), nil
}

// commonDeserializer contains fields common to all types of CLP IR encoding.
// TimestampInfo stores information common to all timestamps found in the IR.
// cptr holds a reference to the underlying C++ objected used as backing storage
// for the Views returned by the deserializer. Close must be called to free this
// underlying memory and failure to do so will result in a memory leak.
type commonDeserializer struct {
	tsInfo TimestampInfo
	cptr   unsafe.Pointer
}

// Close will delete the underlying C++ allocated memory used by the
// deserializer. Failure to call Close will result in a memory leak.
func (self *commonDeserializer) Close() error {
	if nil != self.cptr {
		C.ir_deserializer_close(self.cptr)
		self.cptr = nil
	}
	return nil
}

// Returns the TimestampInfo used by the Deserializer.
func (self commonDeserializer) TimestampInfo() TimestampInfo {
	return self.tsInfo
}

type eightByteDeserializer struct {
	commonDeserializer
}

// DeserializeLogEvent attempts to read the next log event from the IR stream in
// buf, returning the deserialized [ffi.LogEventView], the position read to in
// buf (the end of the log event in buf), and an error. On error returns:
//   - nil *ffi.LogEventView
//   - the position may still be non-zero for debugging purposes
//   - [IRError] error: CLP failed to successfully deserialize
//   - [EOIR] error: CLP found the IR stream EOF tag
func (self *eightByteDeserializer) DeserializeLogEvent(
	buf []byte,
) (*ffi.LogEventView, int, error) {
	return deserializeLogEvent(self, buf)
}

// fourByteDeserializer contains both a common CLP IR deserializer and stores
// the previously seen log event's timestamp. The previous timestamp is
// necessary to calculate the current timestamp as four byte encoding only
// encodes the timestamp delta between the current log event and the previous.
type fourByteDeserializer struct {
	commonDeserializer
	prevTimestamp ffi.EpochTimeMs
}

// DeserializeLogEvent attempts to read the next log event from the IR stream in
// buf, returning the deserialized [ffi.LogEventView], the position read to in
// buf (the end of the log event in buf), and an error. On error returns:
//   - nil *ffi.LogEventView
//   - the position may still be non-zero for debugging purposes
//   - [IRError] error: CLP failed to successfully deserialize
//   - [EOIR] error: CLP found the IR stream EOF tag
func (self *fourByteDeserializer) DeserializeLogEvent(
	buf []byte,
) (*ffi.LogEventView, int, error) {
	return deserializeLogEvent(self, buf)
}

func deserializeLogEvent(
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
