package ir

/*
#include <ffi_go/ir/decoder.h>
*/
import "C"

import (
	"unsafe"

	"github.com/y-scope/clp-ffi-go/ffi"
)

// A Decoder takes objects encoded in CLP IR as input and returns them in their
// natural state prior to encoding. Close must be called to free the underlying
// memory and failure to do so will result in a memory leak.
type Decoder[T EightByteEncoding | FourByteEncoding] interface {
	DecodeLogMessage(logMsg LogMessage[T]) (*ffi.LogMessageView, error)
	Close() error
}

// Return a new Decoder for IR using [EightByteEncoding].
func EightByteDecoder() (Decoder[EightByteEncoding], error) {
	return &eightByteDecoder{commonDecoder{C.ir_decoder_new()}}, nil
}

// Return a new Decoder for IR using [FourByteEncoding].
func FourByteDecoder() (Decoder[FourByteEncoding], error) {
	return &fourByteDecoder{commonDecoder{C.ir_decoder_new()}}, nil
}

type commonDecoder struct {
	cptr unsafe.Pointer
}

// Close will delete the underlying C++ allocated memory used by the
// deserializer. Failure to call Close will result in a memory leak.
func (self *commonDecoder) Close() error {
	if nil != self.cptr {
		C.ir_decoder_close(self.cptr)
		self.cptr = nil
	}
	return nil
}

type eightByteDecoder struct {
	commonDecoder
}

// Decode an IR encoded log message, returning a view of the original
// (non-encoded) log message.
func (self *eightByteDecoder) DecodeLogMessage(
	logMsg LogMessage[EightByteEncoding],
) (*ffi.LogMessageView, error) {
	return decodeLogMessage(func() (*C.char, C.size_t, error) {
		var msg *C.char
		var msgSize C.size_t
		err := IRError(C.ir_decoder_decode_eight_byte_log_message(
			(*C.char)(unsafe.Pointer(unsafe.SliceData(logMsg.Logtype))),
			C.size_t(len(logMsg.Logtype)),
			(*C.int64_t)(unsafe.Pointer(unsafe.SliceData(logMsg.Vars))),
			C.size_t(len(logMsg.Vars)),
			(*C.char)(unsafe.Pointer(unsafe.SliceData(logMsg.DictVars))),
			C.size_t(len(logMsg.DictVars)),
			(*C.int32_t)(unsafe.SliceData(logMsg.DictVarEndOffsets)),
			C.size_t(len(logMsg.DictVarEndOffsets)),
			self.cptr,
			&msg,
			&msgSize))
		return msg, msgSize, err
	})
}

type fourByteDecoder struct {
	commonDecoder
}

// Decode an IR encoded log message, returning a view of the original
// (non-encoded) log message.
func (self *fourByteDecoder) DecodeLogMessage(
	logMsg LogMessage[FourByteEncoding],
) (*ffi.LogMessageView, error) {
	return decodeLogMessage(func() (*C.char, C.size_t, error) {
		var msg *C.char
		var msgSize C.size_t
		err := IRError(C.ir_decoder_decode_four_byte_log_message(
			(*C.char)(unsafe.Pointer(unsafe.SliceData(logMsg.Logtype))),
			C.size_t(len(logMsg.Logtype)),
			(*C.int32_t)(unsafe.Pointer(unsafe.SliceData(logMsg.Vars))),
			C.size_t(len(logMsg.Vars)),
			(*C.char)(unsafe.Pointer(unsafe.SliceData(logMsg.DictVars))),
			C.size_t(len(logMsg.DictVars)),
			(*C.int32_t)(unsafe.SliceData(logMsg.DictVarEndOffsets)),
			C.size_t(len(logMsg.DictVarEndOffsets)),
			self.cptr,
			&msg,
			&msgSize))
		return msg, msgSize, err
	})
}

func decodeLogMessage(
	decode func() (*C.char, C.size_t, error),
) (*ffi.LogMessageView, error) {
	msg, msgSize, err := decode()
	if Success != err {
		return nil, DecodeError
	}
	view := unsafe.String((*byte)((unsafe.Pointer)(msg)), msgSize)
	return &view, nil
}
