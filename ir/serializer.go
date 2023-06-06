package ir

/*
#include <ffi_go/ir/serializer.h>
*/
import "C"

import (
	"unsafe"

	"github.com/y-scope/clp-ffi-go/ffi"
)

type Serializer interface {
	SerializeLogEvent(event ffi.LogEvent) (BufView, error)
	TimestampInfo() TimestampInfo
	Close() error
}

func EightByteSeriealizer(
	ts_pattern string,
	ts_pattern_syntax string,
	time_zone_id string,
) (Serializer, BufView, error) {
	var buf unsafe.Pointer
	var bufSize C.size_t
	irs := eightByteSerializer{
		commonSeriealizer{TimestampInfo{ts_pattern, ts_pattern_syntax, time_zone_id}, nil},
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

func FourByteSeriealizer(
	ts_pattern string,
	ts_pattern_syntax string,
	time_zone_id string,
	reference_ts ffi.EpochTimeMs,
) (Serializer, BufView, error) {
	var buf unsafe.Pointer
	var bufSize C.size_t
	irs := fourByteSerializer{
		commonSeriealizer{TimestampInfo{ts_pattern, ts_pattern_syntax, time_zone_id}, nil},
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

type commonSeriealizer struct {
	tsInfo TimestampInfo
	cptr   unsafe.Pointer
}

// Close will delete cptr, making it undefined to use the irStream after this
// call. Failure to call Close will leak the underlying C-allocated memory.
func (self *commonSeriealizer) Close() error {
	if nil != self.cptr {
		C.ir_serializer_close(self.cptr)
		self.cptr = nil
	}
	return nil
}

// Returns the TimestampInfo of the irStream.
func (self commonSeriealizer) TimestampInfo() TimestampInfo {
	return self.tsInfo
}

type eightByteSerializer struct {
	commonSeriealizer
}

func (self *eightByteSerializer) SerializeLogEvent(
	event ffi.LogEvent,
) (BufView, error) {
	return serializeLogEvent(self, event)
}

type fourByteSerializer struct {
	commonSeriealizer
	prevTimestamp ffi.EpochTimeMs
}

func (self *fourByteSerializer) SerializeLogEvent(
	event ffi.LogEvent,
) (BufView, error) {
	return serializeLogEvent(self, event)
}

// returns 0 on success, >0 on error, <0 on c error
// returned byte slice points to c memory and is only valid until the next call
// to serializeLogEvent (from either SeriealizeLogEvent or SeriealizeLogEventIRView)
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
			(*C.char)(unsafe.Pointer(unsafe.SliceData(event.LogMessage))),
			C.size_t(len(event.LogMessage)),
			C.int64_t(event.Timestamp),
			irs.cptr,
			&buf,
			unsafe.Pointer(&bufSize)))
	case *fourByteSerializer:
		err = IRError(C.ir_serializer_serialize_four_byte_log_event(
			(*C.char)(unsafe.Pointer(unsafe.SliceData(event.LogMessage))),
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
