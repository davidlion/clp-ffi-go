package ir

import (
	"os"
	"testing"
	"time"

	"github.com/klauspost/compress/zstd"
	"github.com/y-scope/clp-ffi-go/ffi"
)

func TestFourByteIRReader(t *testing.T) {
	var fpath string = os.Getenv("go_test_ir")
	if "" == fpath {
		t.Skip("Set an input ir stream using the env variable: go_test_ir")
	}
	var err error
	var file *os.File
	if file, err = os.Open(fpath); nil != err {
		t.Fatalf("os.Open failed: %v", err)
	}
	defer file.Close()

	reader, _ := zstd.NewReader(file)
	defer reader.Close()

	var irr *Reader
	if irr, err = NewReaderSize(reader, 4096); nil != err {
		t.Fatalf("NewReader failed: %v", err)
	}
	defer irr.Close()

	for {
		var log *ffi.LogEventView
		// log, err = irr.Read()
		log, err = irr.ReadToContains("ERROR")
		if nil != err {
			break
		}
		t.Logf("msg: %v | %v", time.UnixMilli(int64(log.Timestamp)), string(log.LogMessageView))
	}
	if EOIR != err {
		t.Fatalf("Reader.Read failed: %v", err)
	}
}
