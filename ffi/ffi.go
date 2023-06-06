package ffi

// Mirrors cpp type epoch_time_ms_t
type EpochTimeMs int64

// The underlying memory of LogMessageView is C-allocated and owned by the
// object (e.g. encoder, decoder, etc) that returned it. Using an existing
// LogMessageView after a new view has been returned by the same producing object
// is undefined (different producing objects own their own memory for views).
type (
	LogMessageView = []byte
	LogMessage     = []byte
)

// LogEvent provides programmatic access to the various components of a log
// event.
type LogEvent struct {
	LogMessage
	Timestamp EpochTimeMs
}

// The underlying memory of LogEventView is C-allocated and owned by the object
// (e.g. reader, desializer, etc) that returned it. Using an existing
// LogEventView after a new view has been returned by the same producing object
// is undefined (different producing objects own their own memory for views).
type LogEventView struct {
	LogMessageView
	Timestamp EpochTimeMs
}
