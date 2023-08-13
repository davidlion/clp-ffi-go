package ir

/*
#include <ffi_go/ir/serializer.h>
*/
import "C"

import (
	"unsafe"

	"github.com/y-scope/clp-ffi-go/ffi"
)

// A Serializer exports functions to serialize log events into a CLP IR byte
// stream. Serialization functions only return views (slices) of IR bytes,
// leaving their use to the user. Each Serializer owns its own unique underlying
// memory for the views it produces/returns. This memory is reused for each
// view, so to persist the contents the memory must be copied into another
// object. Close must be called to free the underlying memory and failure to do
// so will result in a memory leak.
type Serializer interface {
	SerializeLogEvent(event ffi.LogEvent) (BufView, error)
	TimestampInfo() TimestampInfo
	Close() error
}

// EightByteSerializer creates and returns a new Serializer that writes eight
// byte encoded CLP IR and serializes a IR preamble into a BufView using it. On
// error returns:
//   - nil Serializer
//   - nil BufView
//   - [IRError] error: CLP failed to successfully serialize
func EightByteSerializer(
	ts_pattern string,
	ts_pattern_syntax string,
	time_zone_id string,
) (Serializer, BufView, error) {
	var buf unsafe.Pointer
	var bufSize C.size_t
	irs := eightByteSerializer{
		commonSerializer{TimestampInfo{ts_pattern, ts_pattern_syntax, time_zone_id}, nil},
	}
	if err := IRError(C.ir_serializer_serialize_eight_byte_preamble(
		(*C.char)(unsafe.Pointer(unsafe.StringData(ts_pattern))),
		C.size_t(len(ts_pattern)),
		(*C.char)(unsafe.Pointer(unsafe.StringData(ts_pattern_syntax))),
		C.size_t(len(ts_pattern_syntax)),
		(*C.char)(unsafe.Pointer(unsafe.StringData(time_zone_id))),
		C.size_t(len(time_zone_id)),
		&irs.cptr,
		&buf,
		&bufSize,
	)); Success != err {
		return nil, nil, err
	}
	return &irs, unsafe.Slice((*byte)(buf), bufSize), nil
}

// FourByteSerializer creates and returns a new Serializer that writes four byte
// encoded CLP IR and serializes a IR preamble into a BufView using it. On error
// returns:
//   - nil Serializer
//   - nil BufView
//   - [IRError] error: CLP failed to successfully serialize
func FourByteSerializer(
	ts_pattern string,
	ts_pattern_syntax string,
	time_zone_id string,
	reference_ts ffi.EpochTimeMs,
) (Serializer, BufView, error) {
	var buf unsafe.Pointer
	var bufSize C.size_t
	irs := fourByteSerializer{
		commonSerializer{TimestampInfo{ts_pattern, ts_pattern_syntax, time_zone_id}, nil},
		reference_ts,
	}
	if err := IRError(C.ir_serializer_serialize_four_byte_preamble(
		(*C.char)(unsafe.Pointer(unsafe.StringData(ts_pattern))),
		C.size_t(len(ts_pattern)),
		(*C.char)(unsafe.Pointer(unsafe.StringData(ts_pattern_syntax))),
		C.size_t(len(ts_pattern_syntax)),
		(*C.char)(unsafe.Pointer(unsafe.StringData(time_zone_id))),
		C.size_t(len(time_zone_id)),
		C.int64_t(reference_ts),
		&irs.cptr,
		&buf,
		&bufSize,
	)); Success != err {
		return nil, nil, err
	}
	return &irs, unsafe.Slice((*byte)(buf), bufSize), nil
}

// commonSerializer contains fields common to all types of CLP IR encoding.
// TimestampInfo stores information common to all timestamps found in the IR.
// cptr holds a reference to the underlying C++ objected used as backing storage
// for the Views returned by the serializer. Close must be called to free this
// underlying memory and failure to do so will result in a memory leak.
type commonSerializer struct {
	tsInfo TimestampInfo
	cptr   unsafe.Pointer
}

// Close will delete the underlying C++ allocated memory used by the
// deserializer. Failure to call Close will result in a memory leak.
func (self *commonSerializer) Close() error {
	if nil != self.cptr {
		C.ir_serializer_close(self.cptr)
		self.cptr = nil
	}
	return nil
}

// Returns the TimestampInfo of the Serializer.
func (self commonSerializer) TimestampInfo() TimestampInfo {
	return self.tsInfo
}

type eightByteSerializer struct {
	commonSerializer
}

// SerializeLogEvent attempts to serialize the log event, event, into a eight
// byte encoded CLP IR byte stream. On error returns:
//   - a nil BufView
//   - [IRError] based on the failure of the Cgo call
func (self *eightByteSerializer) SerializeLogEvent(
	event ffi.LogEvent,
) (BufView, error) {
	return serializeLogEvent(self, event)
}

// fourByteSerializer contains both a common CLP IR serializer and stores the
// previously seen log event's timestamp. The previous timestamp is necessary to
// calculate the current timestamp as four byte encoding only encodes the
// timestamp delta between the current log event and the previous.
type fourByteSerializer struct {
	commonSerializer
	prevTimestamp ffi.EpochTimeMs
}

// SerializeLogEvent attempts to serialize the log event, event, into a four
// byte encoded CLP IR byte stream. On error returns:
//   - nil BufView
//   - [IRError] based on the failure of the Cgo call
func (self *fourByteSerializer) SerializeLogEvent(
	event ffi.LogEvent,
) (BufView, error) {
	return serializeLogEvent(self, event)
}

func serializeLogEvent(
	serializer Serializer,
	event ffi.LogEvent,
) (BufView, error) {
	var err error
	var buf unsafe.Pointer
	var bufSize uint64

	switch irs := serializer.(type) {
	case *eightByteSerializer:
		err = IRError(C.ir_serializer_serialize_eight_byte_log_event(
			(*C.char)(unsafe.Pointer(unsafe.StringData(event.LogMessage))),
			C.size_t(len(event.LogMessage)),
			C.int64_t(event.Timestamp),
			irs.cptr,
			&buf,
			unsafe.Pointer(&bufSize)))
	case *fourByteSerializer:
		err = IRError(C.ir_serializer_serialize_four_byte_log_event(
			(*C.char)(unsafe.Pointer(unsafe.StringData(event.LogMessage))),
			C.size_t(len(event.LogMessage)),
			C.int64_t(irs.prevTimestamp-event.Timestamp),
			irs.cptr,
			&buf,
			unsafe.Pointer(&bufSize)))
		if Success == err {
			irs.prevTimestamp = event.Timestamp
		}
	}
	if Success != err {
		return nil, err
	}
	return unsafe.Slice((*byte)(buf), bufSize), nil
}
