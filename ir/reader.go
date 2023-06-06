package ir

import (
	"bytes"
	"io"

	"github.com/y-scope/clp-ffi-go/ffi"
)

// StreamReader abstracts maintenance of a buffer containing an IRDeserializer. It
// keeps track of the range in the buffer containing valid, unconsumed IR. It
// does not store a Reader to allow callers to mutate the Reader as necessary.
type StreamReader struct {
	Deserializer
	buf   []byte
	start int
	end   int
}

// adjust_buf mutates [StreamReader.buf] so that the next read call has space to
// fill. If the start of StreamReader.buf is not 0 the contents of buf will be
// shifted back, so that end -= start and start = 0. If start is already 0 the
// buffer is grown.
func (self *StreamReader) adjust_buf() int {
	if self.end+4096 < len(self.buf) {
		return 0
	}
	if 0 == self.start {
		buf := make([]byte, len(self.buf)*2)
		copy(buf, self.buf[self.start:self.end])
		self.buf = buf
	} else {
		copy(self.buf, self.buf[self.start:self.end])
	}
	self.end -= self.start
	self.start = 0
	return len(self.buf)
}

// read is a wrapper around a io.Reader.Read call. It uses the correct range in
// buf and adjusts the range accordingly. On success nil is returned. On
// failure an error whose type depends on the [io.Reader] is returned. Note we
// do not return [io.EOF] if n > 0 as we have not yet consumed the IR.
func (self *StreamReader) read(r io.Reader) error {
	n, err := r.Read(self.buf[self.end:])
	if nil != err && io.EOF != err {
		return err
	}
	self.end += n
	return nil
}

// ReadPreamble uses [DeserializePreamble] to read an IR stream preamble from r.
// bufSize denotes the initial size to use for the underlying buffer io.Reader
// is read into. This buffer will grow if it is too small to contain the
// preamble or next log event.
// Return values:
//   - nil == error: success
//   - [IRError] or [encoding/json]: error propagated from [DeserializePreamble]
//   - [io] error type or underlying reader type: io.Reader.Read failed
func ReadPreamble(r io.Reader, bufSize int) (*StreamReader, error) {
	irr := &StreamReader{nil, make([]byte, bufSize), 0, 0}

	if err := irr.read(r); nil != err {
		return irr, err
	}

	for {
		var err error
		irr.Deserializer, irr.start, err = DeserializePreamble(irr.buf[irr.start:irr.end])
		if nil == err {
			return irr, nil
		}
		if IncompleteIR == err {
			irr.adjust_buf()
			if err := irr.read(r); nil != err {
				return irr, err
			}
		} else {
			return irr, err
		}
	}
}

// ReadLogEvent uses [DeserializeLogEventView] to read from the IR stream in r.
// The underlying buffer will grow if it is too small to contain the next log
// event.
// Return values:
//   - nil == error: success
//   - IRError.EOIR: CLP found the IR stream EOF tag
//   - io.EOF: io.Reader.Read got EOF
//   - else:
//   - type [IRError]: error propagated from [DeserializeLogEventView]
//   - type from io.Reader: io.Reader.Read failed
func (self *StreamReader) ReadLogEvent(r io.Reader) (*ffi.LogEventView, error) {
	for {
		event, pos, err := self.DeserializeLogEventView(self.buf[self.start:self.end])
		if nil == err {
			self.start += pos
			return event, nil
		}
		if IncompleteIR == err {
			self.adjust_buf()
			if err := self.read(r); nil != err {
				return event, err
			}
		} else {
			return event, err
		}
	}
}

// Read the IR stream using the io.Reader until f returns true for a
// [ffi.LogEvent]. The succeeding LogEvent is returned. Errors are propagated
// from ReadLogEvent.
func (self *StreamReader) ReadToFunc(
	r io.Reader,
	f func(*ffi.LogEventView) bool,
) (*ffi.LogEventView, error) {
	for {
		event, err := self.ReadLogEvent(r)
		if nil != err {
			return event, err
		}
		if f(event) {
			return event, nil
		}
	}
}

// Read the IR stream using the io.Reader until [ffi.LogEvent.Timestamp] >=
// time. Errors are propagated from ReadLogEvent.
func (self *StreamReader) ReadToEpochTime(
	r io.Reader,
	time ffi.EpochTimeMs,
) (*ffi.LogEventView, error) {
	return self.ReadToFunc(r, func(e *ffi.LogEventView) bool { return e.Timestamp >= time })
}

// Read the IR stream using the io.Reader until [bytes/Contains] returns true
// for [ffi.LogEvent.Msg] and subslice. Errors are propagated from ReadLogEvent.
func (self *StreamReader) ReadToContains(r io.Reader, subslice []byte) (*ffi.LogEventView, error) {
	return self.ReadToFunc(
		r,
		func(e *ffi.LogEventView) bool { return bytes.Contains(e.LogMessageView, subslice) },
	)
}

// Read the IR stream using the io.Reader until [bytes/HasPrefix] returns true
// for [ffi.LogEvent.Msg] and prefix. Errors are propagated from ReadLogEvent.
func (self *StreamReader) ReadToPrefix(r io.Reader, prefix []byte) (*ffi.LogEventView, error) {
	return self.ReadToFunc(
		r,
		func(e *ffi.LogEventView) bool { return bytes.HasPrefix(e.LogMessageView, prefix) },
	)
}

// Read the IR stream using the io.Reader until [bytes/HasSuffix] returns true
// for [ffi.LogEvent.Msg] field and suffix. Errors are propagated from ReadLogEvent.
func (self *StreamReader) ReadToSuffix(r io.Reader, suffix []byte) (*ffi.LogEventView, error) {
	return self.ReadToFunc(
		r,
		func(e *ffi.LogEventView) bool { return bytes.HasSuffix(e.LogMessageView, suffix) },
	)
}
