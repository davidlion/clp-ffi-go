package ir

import (
	"bytes"
	"io"

	"github.com/y-scope/clp-ffi-go/ffi"
)

// StreamReader abstracts maintenance of a buffer containing a [Deserializer].
// It keeps track of the range [start, end) in the buffer containing valid,
// unconsumed CLP IR. [ReadPreamble] will construct a StreamReader with the
// appropriate Deserializer based on the consumed CLP IR preamble. The buffer
// will grow if it is not large enough to service a read call (e.g. it cannot
// hold the next log event in the IR). Close must be called to free the
// underlying memory and failure to do so will result in a memory leak.
type StreamReader struct {
	Deserializer
	Reader io.Reader
	buf    []byte
	start  int
	end    int
}

// ReadPreamble creates a new [StreamReader] and uses [DeserializePreamble] to
// read a CLP IR preamble from the [io.Reader], r. bufSize denotes the initial
// size to use for the StreamReader's buffer that the io.Reader is read into.
// This buffer will grow if it is too small to contain the preamble or next log
// event. Returns:
//   - success: valid [*StreamReader] and nil == error
//   - error: [*StreamReader] is nil and error propagated from
//     [DeserializePreamble] or [StreamReader.read]
func ReadPreamble(r io.Reader, bufSize int) (*StreamReader, error) {
	irr := &StreamReader{nil, r, make([]byte, bufSize), 0, 0}

	if err := irr.read(); nil != err {
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
			if err := irr.read(); nil != err {
				return nil, err
			}
		} else {
			return nil, err
		}
	}
}

// Close will delete the underlying C++ allocated memory used by the
// deserializer. Failure to call Close will result in a memory leak.
func (self *StreamReader) Close() error {
	return self.Deserializer.Close()
}

// ReadLogEvent uses [DeserializeLogEvent] to read from the CLP IR byte
// stream. The underlying buffer will grow if it is too small to contain the
// next log event. On error returns:
//   - nil *ffi.LogEventView
//   - error propagated from [DeserializeLogEvent] or [StreamReader.read]
func (self *StreamReader) ReadLogEvent() (*ffi.LogEventView, error) {
	for {
		event, pos, err := self.DeserializeLogEvent(self.buf[self.start:self.end])
		if nil == err {
			self.start += pos
			return event, nil
		}
		if IncompleteIR == err {
			self.adjust_buf()
			if err := self.read(); nil != err {
				return event, err
			}
		} else {
			return event, err
		}
	}
}

// Read the CLP IR byte stream using the io.Reader until f returns true for a
// [ffi.LogEvent]. The successful LogEvent is returned. Errors are propagated
// from [ReadLogEvent].
func (self *StreamReader) ReadToFunc(
	f func(*ffi.LogEventView) bool,
) (*ffi.LogEventView, error) {
	for {
		event, err := self.ReadLogEvent()
		if nil != err {
			return event, err
		}
		if f(event) {
			return event, nil
		}
	}
}

// Read the CLP IR stream using the io.Reader until [ffi.LogEvent.Timestamp] >=
// time. Errors are propagated from ReadLogEvent.
func (self *StreamReader) ReadToEpochTime(
	time ffi.EpochTimeMs,
) (*ffi.LogEventView, error) {
	return self.ReadToFunc(func(e *ffi.LogEventView) bool { return e.Timestamp >= time })
}

// Read the CLP IR stream using the io.Reader until [bytes/Contains] returns
// true for [ffi.LogEvent.Msg] and subslice. Errors are propagated from
// ReadLogEvent.
func (self *StreamReader) ReadToContains(subslice []byte) (*ffi.LogEventView, error) {
	return self.ReadToFunc(
		func(e *ffi.LogEventView) bool { return bytes.Contains(e.LogMessageView, subslice) },
	)
}

// Read the CLP IR stream using the io.Reader until [bytes/HasPrefix] returns
// true for [ffi.LogEvent.Msg] and prefix. Errors are propagated from
// ReadLogEvent.
func (self *StreamReader) ReadToPrefix(prefix []byte) (*ffi.LogEventView, error) {
	return self.ReadToFunc(
		func(e *ffi.LogEventView) bool { return bytes.HasPrefix(e.LogMessageView, prefix) },
	)
}

// Read the CLP IR stream using the io.Reader until [bytes/HasSuffix] returns
// true for [ffi.LogEvent.Msg] field and suffix. Errors are propagated from
// ReadLogEvent.
func (self *StreamReader) ReadToSuffix(suffix []byte) (*ffi.LogEventView, error) {
	return self.ReadToFunc(
		func(e *ffi.LogEventView) bool { return bytes.HasSuffix(e.LogMessageView, suffix) },
	)
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
// do not return [io.EOF] if n > 0 as we have not yet consumed the CLP IR.
func (self *StreamReader) read() error {
	n, err := self.Reader.Read(self.buf[self.end:])
	if nil != err && io.EOF != err {
		return err
	}
	self.end += n
	return nil
}
