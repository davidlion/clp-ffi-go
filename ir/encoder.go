package ir

/*
#include <ffi_go/ir/encoder.h>
*/
import "C"

import (
	"unsafe"

	"github.com/y-scope/clp-ffi-go/ffi"
)

// An Encoder takes logging objects (commonly used/created by logging libraries)
// and encodes them as CLP IR.
// Close must be called to free the underlying memory and failure to do so will
// result in a memory leak.
type Encoder[T EightByteEncoding | FourByteEncoding] interface {
	EncodeLogMessage(logMsg ffi.LogMessage) (*LogMessageView[T], error)
	Close() error
}

// Return a new Encoder that produces IR using [EightByteEncoding].
func EightByteEncoder() (Encoder[EightByteEncoding], error) {
	return &eightByteEncoder{C.ir_encoder_eight_byte_new()}, nil
}

// Return a new Encoder that produces IR using [FourByteEncoding].
func FourByteEncoder() (Encoder[FourByteEncoding], error) {
	return &fourByteEncoder{C.ir_encoder_four_byte_new()}, nil
}

type eightByteEncoder struct {
	cptr unsafe.Pointer
}

// Close will delete the underlying C++ allocated memory used by the
// deserializer. Failure to call Close will result in a memory leak.
func (self *eightByteEncoder) Close() error {
	if nil != self.cptr {
		C.ir_encoder_eight_byte_close(self.cptr)
		self.cptr = nil
	}
	return nil
}

// Encode a log message into CLP IR, returning a view of the encoded message.
func (self *eightByteEncoder) EncodeLogMessage(
	logMsg ffi.LogMessage,
) (*LogMessageView[EightByteEncoding], error) {
	var logMsgIR logMessageCVars[C.int64_t]
	err := IRError(C.ir_encoder_encode_eight_byte_log_message(
		(*C.char)(unsafe.Pointer(unsafe.SliceData(logMsg))),
		C.size_t(len(logMsg)),
		&logMsgIR.logtype,
		&logMsgIR.logtypeSize,
		&logMsgIR.vars,
		&logMsgIR.varsSize,
		&logMsgIR.dictVars,
		&logMsgIR.dictVarsSize,
		&logMsgIR.dictVarEndOffsets,
		&logMsgIR.dictVarEndOffsetsSize,
		self.cptr))
	if Success != err {
		return nil, EncoderError
	}
	var logMsgView LogMessageView[EightByteEncoding]
	logMsgView.Logtype = unsafe.Slice(
		(*byte)((unsafe.Pointer)(logMsgIR.logtype)),
		logMsgIR.logtypeSize,
	)
	if 0 != logMsgIR.varsSize {
		logMsgView.Vars = unsafe.Slice((*int64)(logMsgIR.vars), logMsgIR.varsSize)
		if nil == logMsgView.Vars {
			return nil, EncoderError
		}
	}
	if 0 != logMsgIR.dictVarsSize {
		logMsgView.DictVars = unsafe.Slice(
			(*byte)((unsafe.Pointer)(logMsgIR.dictVars)),
			logMsgIR.dictVarsSize,
		)
		if nil == logMsgView.DictVars {
			return nil, EncoderError
		}
	}
	if 0 != logMsgIR.dictVarEndOffsetsSize {
		logMsgView.DictVarEndOffsets = unsafe.Slice(
			(*int32)(logMsgIR.dictVarEndOffsets),
			logMsgIR.dictVarEndOffsetsSize,
		)
		if nil == logMsgView.DictVarEndOffsets {
			return nil, EncoderError
		}
	}
	return &logMsgView, nil
}

type fourByteEncoder struct {
	cptr unsafe.Pointer
}

// Close will delete the underlying C++ allocated memory used by the
// deserializer. Failure to call Close will result in a memory leak.
func (self *fourByteEncoder) Close() error {
	if nil != self.cptr {
		C.ir_encoder_four_byte_close(self.cptr)
		self.cptr = nil
	}
	return nil
}

// Encode a log message into CLP IR, returning a view of the encoded message.
func (self *fourByteEncoder) EncodeLogMessage(
	logMsg ffi.LogMessage,
) (*LogMessageView[FourByteEncoding], error) {
	var logMsgIR logMessageCVars[C.int32_t]
	err := IRError(C.ir_encoder_encode_four_byte_log_message(
		(*C.char)(unsafe.Pointer(unsafe.SliceData(logMsg))),
		C.size_t(len(logMsg)),
		&logMsgIR.logtype,
		&logMsgIR.logtypeSize,
		&logMsgIR.vars,
		&logMsgIR.varsSize,
		&logMsgIR.dictVars,
		&logMsgIR.dictVarsSize,
		&logMsgIR.dictVarEndOffsets,
		&logMsgIR.dictVarEndOffsetsSize,
		self.cptr))
	if Success != err {
		return nil, EncoderError
	}
	var logMsgView LogMessageView[FourByteEncoding]
	logMsgView.Logtype = unsafe.Slice(
		(*byte)((unsafe.Pointer)(logMsgIR.logtype)),
		logMsgIR.logtypeSize,
	)
	if 0 != logMsgIR.varsSize {
		logMsgView.Vars = unsafe.Slice((*int32)(logMsgIR.vars), logMsgIR.varsSize)
		if nil == logMsgView.Vars {
			return nil, EncoderError
		}
	}
	if 0 != logMsgIR.dictVarsSize {
		logMsgView.DictVars = unsafe.Slice(
			(*byte)((unsafe.Pointer)(logMsgIR.dictVars)),
			logMsgIR.dictVarsSize,
		)
		if nil == logMsgView.DictVars {
			return nil, EncoderError
		}
	}
	if 0 != logMsgIR.dictVarEndOffsetsSize {
		logMsgView.DictVarEndOffsets = unsafe.Slice(
			(*int32)(logMsgIR.dictVarEndOffsets),
			logMsgIR.dictVarEndOffsetsSize,
		)
		if nil == logMsgView.DictVarEndOffsets {
			return nil, EncoderError
		}
	}
	return &logMsgView, nil
}

type logMessageCVars[T C.int64_t | C.int32_t] struct {
	logtype               *C.char
	logtypeSize           C.size_t
	vars                  *T
	varsSize              C.size_t
	dictVars              *C.char
	dictVarsSize          C.size_t
	dictVarEndOffsets     *C.int32_t
	dictVarEndOffsetsSize C.size_t
}
