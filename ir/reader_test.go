package ir

import (
	"io"
	"os"
	"testing"
	"time"

	"github.com/klauspost/compress/zstd"
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

	var irr *StreamReader
	if irr, err = ReadPreamble(reader, 4096); nil != err {
		t.Fatalf("ReadPreamble failed: %v", err)
	}
	defer irr.Close()

	for {
		// log, err := irr.ReadLogEvent()
		log, err := irr.ReadToContains([]byte("ERROR"))
		if nil == err {
			t.Logf("msg: %v | %v", time.UnixMilli(int64(log.Timestamp)), string(log.LogMessageView))
		} else if EOIR == err || io.EOF == err {
			break
		} else {
			t.Fatalf("ReadLogEvent failed: %v", err)
		}
	}
}
