package ir

/*
#include <ffi_go/defs.h>
#include <ffi_go/ir/deserializer.h>
#include <ffi_go/search/wildcard.h>
*/
import "C"

import (
	"encoding/json"
	"strconv"
	"unsafe"

	"github.com/y-scope/clp-ffi-go/ffi"
	"github.com/y-scope/clp-ffi-go/search"
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
	DeserializeLogEvent(irBuf []byte) (*ffi.LogEventView, int, error)
	DeserializeWildcardMatch(
		irBuf []byte,
		timeInterval search.TimestampInterval,
		queries []search.WildcardQuery,
	) (*ffi.LogEventView, int, int, error)
	TimestampInfo() TimestampInfo
	Close() error
}

// DeserializePreamble attempts to read an IR stream preamble from irBuf,
// returning an Deserializer (of the correct stream encoding size), the position
// read to in irBuf (the end of the preamble), and an error. Note the metadata
// stored in the preamble is sparse and certain fields in TimestampInfo may be 0
// value. On error returns:
//   - nil Deserializer
//   - 0 position
//   - [IRError] error: CLP failed to successfully deserialize
//   - [encoding/json] error: unmarshalling the metadata failed
func DeserializePreamble(irBuf []byte) (Deserializer, int, error) {
	if 0 >= len(irBuf) {
		return nil, 0, IncompleteIR
	}

	var pos C.size_t
	var irEncoding C.int8_t
	var metadataType C.int8_t
	var metadataPos C.size_t
	var metadataSize C.uint16_t
	var cptr unsafe.Pointer
	if err := IRError(C.ir_deserializer_deserialize_preamble(
		newCByteView(irBuf),
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
		return nil, 0, UnsupportedVersion
	}

	var metadata map[string]interface{}
	if err := json.Unmarshal(
		irBuf[metadataPos:metadataPos+C.size_t(metadataSize)],
		&metadata,
	); nil != err {
		return nil, 0, err
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
// irBuf, returning the deserialized [ffi.LogEventView], the position read to in
// irBuf (the end of the log event in irBuf), and an error. On error returns:
//   - nil *ffi.LogEventView
//   - 0 position
//   - [IRError] error: CLP failed to successfully deserialize
//   - [EOIR] error: CLP found the IR stream EOF tag
func (self *eightByteDeserializer) DeserializeLogEvent(
	irBuf []byte,
) (*ffi.LogEventView, int, error) {
	return deserializeLogEvent(self, irBuf)
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
// irBuf, returning the deserialized [ffi.LogEventView], the position read to in
// irBuf (the end of the log event in irBuf), and an error. On error returns:
//   - nil *ffi.LogEventView
//   - 0 position
//   - [IRError] error: CLP failed to successfully deserialize
//   - [EOIR] error: CLP found the IR stream EOF tag
func (self *fourByteDeserializer) DeserializeLogEvent(
	irBuf []byte,
) (*ffi.LogEventView, int, error) {
	return deserializeLogEvent(self, irBuf)
}

func (self *eightByteDeserializer) DeserializeWildcardMatch(
	irBuf []byte,
	timeInterval search.TimestampInterval,
	queries []search.WildcardQuery,
) (*ffi.LogEventView, int, int, error) {
	return deserializeWildcardMatch(self, irBuf, timeInterval, queries)
}

func (self *fourByteDeserializer) DeserializeWildcardMatch(
	irBuf []byte,
	timeInterval search.TimestampInterval,
	queries []search.WildcardQuery,
) (*ffi.LogEventView, int, int, error) {
	return deserializeWildcardMatch(self, irBuf, timeInterval, queries)
}

func deserializeLogEvent(
	deserializer Deserializer,
	irBuf []byte,
) (*ffi.LogEventView, int, error) {
	if 0 >= len(irBuf) {
		return nil, 0, IncompleteIR
	}

	var pos C.size_t
	var logevent C.LogEventView
	var err error
	switch irs := deserializer.(type) {
	case *eightByteDeserializer:
		err = IRError(C.ir_deserializer_deserialize_eight_byte_log_event(
			newCByteView(irBuf),
			irs.cptr,
			&pos,
			&logevent,
		))
	case *fourByteDeserializer:
		err = IRError(C.ir_deserializer_deserialize_four_byte_log_event(
			newCByteView(irBuf),
			irs.cptr,
			&pos,
			&logevent,
		))
	}
	if Success != err {
		return nil, 0, err
	}

	var event ffi.LogEventView
	event.LogMessageView = unsafe.String(
		(*byte)((unsafe.Pointer)(logevent.m_log_message.m_data)),
		logevent.m_log_message.m_size,
	)
	event.Timestamp = ffi.EpochTimeMs(logevent.m_timestamp)
	return &event, int(pos), nil
}

func deserializeWildcardMatch(
	deserializer Deserializer,
	irBuf []byte,
	timeInterval search.TimestampInterval,
	queries []search.WildcardQuery,
) (*ffi.LogEventView, int, int, error) {
	if 0 >= len(irBuf) {
		return nil, 0, -1, IncompleteIR
	}

	cqueries := make([]C.WildcardQueryView, len(queries))
	for i, query := range queries {
		cqueries[i].m_query.m_data = (*C.char)(unsafe.Pointer(unsafe.StringData(query.Query)))
		cqueries[i].m_query.m_size = C.size_t(len(query.Query))
		if query.CaseSensitive {
			cqueries[i].m_case_sensitive = 1
		}
	}
	var pos C.size_t
	var logevent C.LogEventView
	var matching_query_idx C.size_t
	var err error
	switch irs := deserializer.(type) {
	case *eightByteDeserializer:
		err = IRError(C.ir_deserializer_deserialize_eight_byte_wildcard_match(
			newCByteView(irBuf),
			irs.cptr,
			C.TimestampInterval{C.int64_t(timeInterval.Lower), C.int64_t(timeInterval.Upper)},
			(*C.WildcardQueryView)(unsafe.Pointer(unsafe.SliceData(cqueries))),
			C.size_t(len(cqueries)),
			&pos,
			&logevent,
			&matching_query_idx,
		))
	case *fourByteDeserializer:
		err = IRError(C.ir_deserializer_deserialize_four_byte_wildcard_match(
			newCByteView(irBuf),
			irs.cptr,
			C.TimestampInterval{C.int64_t(timeInterval.Lower), C.int64_t(timeInterval.Upper)},
			(*C.WildcardQueryView)(unsafe.Pointer(unsafe.SliceData(cqueries))),
			C.size_t(len(cqueries)),
			&pos,
			&logevent,
			&matching_query_idx,
		))
	}
	if Success != err {
		return nil, 0, -1, err
	}

	var event ffi.LogEventView
	event.LogMessageView = unsafe.String(
		(*byte)((unsafe.Pointer)(logevent.m_log_message.m_data)),
		logevent.m_log_message.m_size,
	)
	event.Timestamp = ffi.EpochTimeMs(logevent.m_timestamp)
	return &event, int(pos), int(matching_query_idx), nil
}
