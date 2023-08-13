package ir

import (
	"fmt"
	"io"
	"math"
	"os"
	"path/filepath"
	"strings"
	"testing"
	"time"

	"github.com/klauspost/compress/zstd"
	"github.com/y-scope/clp-ffi-go/ffi"
)

const (
	defaultTimestampPattern       string = "yyyy-MM-dd HH:mm:ss,SSS"
	defaultTimestampPatternSyntax string = "java::SimpleDateFormat"
	defaultTimeZoneId             string = "America/Toronto"
)

func TestPreamble(t *testing.T) {
	preamble := preambleFields{
		TimestampInfo{defaultTimestampPattern, defaultTimestampPatternSyntax, defaultTimeZoneId},
		ffi.EpochTimeMs(time.Now().UnixMilli()),
	}
	for _, args := range generateTestArgs(t) {
		args := args // capture range variable for func literal
		t.Run(args.name, func(t *testing.T) { t.Parallel(); testPreamble(t, args, preamble) })
	}
}

func TestLogMessagesCombo(t *testing.T) {
	messages := []ffi.LogMessage{
		"static text dict=var notint123 -1.234 4321.",
		"static123 text321 dict=var0123 321.1234 -3210.",
	}
	testLogMessagesBasic(t, messages)
}

func TestLogMessagesDict(t *testing.T) {
	messages := []ffi.LogMessage{
		"textint1234 textequal=variable",
		fmt.Sprintf("test=bigint %v", math.MaxInt32+1),
	}
	testLogMessagesBasic(t, messages)
}

func TestLogMessagesFloat(t *testing.T) {
	messages := []ffi.LogMessage{
		"float 1.0 1.2 1.23 1.234",
		"-float -1.0 -1.2 -1.23 -1.234",
	}
	testLogMessagesBasic(t, messages)
}

func TestLogMessagesInt(t *testing.T) {
	messages := []ffi.LogMessage{
		"int 1 12 123 1234",
		"-int -1 -12 -123 -1234",
	}
	testLogMessagesBasic(t, messages)
}

func TestLogMessagesStatic(t *testing.T) {
	messages := []ffi.LogMessage{
		"static text log zero.",
		"static text log one.",
	}
	testLogMessagesBasic(t, messages)
}

func TestLogMessagesLongLogs(t *testing.T) {
	const eightMB int = 8 * 1024 * 1024
	messages := []ffi.LogMessage{
		strings.Repeat("x", eightMB),
		strings.Repeat("x", eightMB-1),
	}
	testLogMessagesBasic(t, messages)
}

func testLogMessagesBasic(t *testing.T, messages []ffi.LogMessage) {
	preamble := preambleFields{
		TimestampInfo{defaultTimestampPattern, defaultTimestampPatternSyntax, defaultTimeZoneId},
		ffi.EpochTimeMs(time.Now().UnixMilli()),
	}
	for _, args := range generateTestArgs(t) {
		args := args // capture range variable for func literal
		t.Run(
			args.name,
			func(t *testing.T) { t.Parallel(); testLogMessages(t, args, preamble, messages) },
		)
	}
}

func generateTestArgs(t *testing.T) []testArgs {
	var tests []testArgs
	tmpdir := t.TempDir()
	for _, encoding := range []testArg{eightByteEncoding, fourByteEncoding} {
		for _, compression := range []testArg{noCompression, zstdCompression} {
			testName := testArgStr[encoding] + "-" + testArgStr[compression]
			fileName := testName + ".clp"
			if zstdCompression == compression {
				fileName += ".zst"
			}
			filePath := filepath.Join(tmpdir, fileName)
			tests = append(tests, testArgs{encoding, compression, testName, filePath})
		}
	}
	return tests
}

func testPreamble(t *testing.T, args testArgs, preamble preambleFields) {
	writer := openWriter(t, args)
	irSerializer := serializeIRPreamble(t, args, preamble, writer)

	writer.Close()
	irSerializer.Close()

	reader := openReader(t, args)
	assertIRPreamble(t, args, reader, preamble)
}

func testLogMessages(
	t *testing.T,
	args testArgs,
	preamble preambleFields,
	logMessages []ffi.LogMessage,
) {
	writer := openWriter(t, args)
	irSerializer := serializeIRPreamble(t, args, preamble, writer)
	for _, msg := range logMessages {
		writeIRLogMessage(t, writer, irSerializer, msg)
	}

	writer.Write([]byte{0x0})
	writer.Close()
	irSerializer.Close()

	reader := openReader(t, args)
	defer reader.Close()
	irreader := assertIRPreamble(t, args, reader, preamble)
	defer irreader.Close()

	for _, msg := range logMessages {
		assertIRLogEvent(t, reader, irreader, msg)
	}
	assertEOIR(t, reader, irreader)
}

type zstdReader struct {
	*zstd.Decoder
}

func newZstdReader(reader io.Reader) (*zstdReader, error) {
	zreader, err := zstd.NewReader(reader)
	return &zstdReader{zreader}, err
}

func (self *zstdReader) Close() error {
	self.Decoder.Close()
	return nil
}

func openReader(t *testing.T, args testArgs) io.ReadCloser {
	file, err := os.Open(args.filePath)
	if nil != err {
		t.Fatalf("os.Open: %v", err)
	}
	var reader io.ReadCloser
	switch args.compression {
	case noCompression:
		reader = file
	case zstdCompression:
		reader, err = newZstdReader(file)
		if nil != err {
			t.Fatalf("zstd.NewReader failed: %v", err)
		}
	default:
		t.Fatalf("unsupported compression: %v", args.compression)
	}
	return reader
}

func openWriter(t *testing.T, args testArgs) io.WriteCloser {
	file, err := os.Create(args.filePath)
	if nil != err {
		t.Fatalf("os.Create: %v", err)
	}
	var writer io.WriteCloser
	switch args.compression {
	case noCompression:
		writer = file
	case zstdCompression:
		writer, err = zstd.NewWriter(file)
		if nil != err {
			t.Fatalf("zstd.NewWriter failed: %v", err)
		}
	default:
		t.Fatalf("unsupported compression: %v", args.compression)
	}
	return writer
}

func serializeIRPreamble(
	t *testing.T,
	args testArgs,
	preamble preambleFields,
	writer io.Writer,
) Serializer {
	var err error
	var serializer Serializer
	var preambleIR BufView
	switch args.encoding {
	case eightByteEncoding:
		serializer, preambleIR, err = EightByteSerializer(
			preamble.Pattern,
			preamble.PatternSyntax,
			preamble.TimeZoneId,
		)
	case fourByteEncoding:
		serializer, preambleIR, err = FourByteSerializer(
			preamble.Pattern,
			preamble.PatternSyntax,
			preamble.TimeZoneId,
			preamble.prevTimestamp,
		)
	default:
		t.Fatalf("unsupported encoding: %v", args.encoding)
	}
	if nil != err {
		t.Fatalf("constructor failed: %v", err)
	}
	n, err := writer.Write(preambleIR)
	if n != len(preambleIR) {
		t.Fatalf("short write for preamble: %v/%v", n, len(preambleIR))
	}
	if nil != err {
		t.Fatalf("io.Writer.Write preamble: %v", err)
	}
	return serializer
}

func assertIRPreamble(
	t *testing.T,
	args testArgs,
	reader io.Reader,
	preamble preambleFields,
) *Reader {
	irreader, err := NewReaderSize(reader, 4096)
	if nil != err {
		t.Fatalf("NewReader failed: %v", err)
	}
	if irreader.TimestampInfo().Pattern != preamble.Pattern {
		t.Fatalf(
			"NewReader wrong pattern: '%v' != '%v'",
			irreader.TimestampInfo().Pattern,
			preamble.Pattern,
		)
	}
	if irreader.TimestampInfo().PatternSyntax != preamble.PatternSyntax {
		t.Fatalf(
			"NewReader wrong pattern syntax: '%v' != '%v'",
			irreader.TimestampInfo().PatternSyntax,
			preamble.PatternSyntax,
		)
	}
	if irreader.TimestampInfo().TimeZoneId != preamble.TimeZoneId {
		t.Fatalf(
			"NewReader wrong time zone id: '%v' != '%v'",
			irreader.TimestampInfo().TimeZoneId,
			preamble.TimeZoneId,
		)
	}
	if fourByteEncoding == args.encoding {
		deserializer, ok := irreader.Deserializer.(*fourByteDeserializer)
		if false == ok {
			t.Fatalf("casting Deserializer to *fourByteDeserializer failed for fourByteEncoding.")
		}
		if deserializer.prevTimestamp != preamble.prevTimestamp {
			t.Fatalf(
				"NewReader wrong reference timestamp: '%v' != '%v'",
				deserializer.prevTimestamp,
				preamble.prevTimestamp,
			)
		}
	}
	return irreader
}

func writeIRLogMessage(
	t *testing.T,
	writer io.Writer,
	serializer Serializer,
	message ffi.LogMessage,
) {
	event := ffi.LogEvent{
		LogMessage: message,
		Timestamp:  ffi.EpochTimeMs(time.Now().UnixMilli()),
	}
	irview, err := serializer.SerializeLogEvent(event)
	if nil != err {
		t.Fatalf("SerializeLogEvent failed: %v", err)
	}
	n, err := writer.Write(irview)
	if n != len(irview) {
		t.Fatalf("short write for message: %v/%v", n, len(irview))
	}
	if nil != err {
		t.Fatalf("io.Writer.Write message: %v", err)
	}
}

func assertIRLogEvent(
	t *testing.T,
	reader io.Reader,
	irreader *Reader,
	message ffi.LogMessage,
) {
	log, err := irreader.Read()
	if nil != err {
		t.Fatalf("Reader.Read failed: %v", err)
	}
	if message != log.LogMessageView {
		t.Fatalf("Reader.Read wrong message: '%v' != '%v'", log.LogMessageView, message)
	}
	t.Logf("'%v' : '%.128v'\n", log.Timestamp, log.LogMessageView)
}

func assertEOIR(
	t *testing.T,
	reader io.Reader,
	irreader *Reader,
) {
	_, err := irreader.Read()
	if EOIR != err {
		t.Fatalf("assertEOIR failed got: %v", err)
	}
}
