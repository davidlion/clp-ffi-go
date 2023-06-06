package ir

/*
#include <ffi_go/ir/decoder.h>
*/
import "C"

import (
	"unsafe"

	"github.com/y-scope/clp-ffi-go/ffi"
)

type Decoder[T EightByteEncoding | FourByteEncoding] interface {
	DecodeLogMessage(logMsg LogMessage[T]) (*ffi.LogMessageView, error)
	Close() error
}

func EightByteDecoder() (Decoder[EightByteEncoding], error) {
	return &eightByteDecoder{commonDecoder{C.ir_decoder_new()}}, nil
}

func FourByteDecoder() (Decoder[FourByteEncoding], error) {
	return &fourByteDecoder{commonDecoder{C.ir_decoder_new()}}, nil
}

type eightByteDecoder struct {
	commonDecoder
}

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
	view := unsafe.Slice((*byte)((unsafe.Pointer)(msg)), msgSize)
	return &view, nil
}

type commonDecoder struct {
	cptr unsafe.Pointer
}

func (self *commonDecoder) Close() error {
	if nil != self.cptr {
		C.ir_decoder_close(self.cptr)
		self.cptr = nil
	}
	return nil
}
