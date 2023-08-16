#ifndef FFI_GO_DEF_H
#define FFI_GO_DEF_H

// header must support C, making modernize checks inapplicable
// NOLINTBEGIN(modernize-deprecated-headers)
// NOLINTBEGIN(modernize-use-using)

#include <stdint.h>
#include <stdlib.h>

// TODO: replace with clp c-compatible header once it exists
typedef int64_t epoch_time_ms_t;

/**
 * A view of a byte array passed down through Cgo.
 */
typedef struct {
    void* m_data;
    size_t m_size;
} ByteView;

/**
 * A view of a Go string passed down through Cgo.
 */
typedef struct {
    char const* m_data;
    size_t m_size;
} StringView;

/**
 * A view of a Go ffi.LogEvent passed down through Cgo.
 */
typedef struct {
    StringView m_log_message;
    epoch_time_ms_t m_timestamp;
} LogEventView;

/**
 * A timestamp interval of [m_lower, m_upper).
 */
typedef struct {
    epoch_time_ms_t m_lower;
    epoch_time_ms_t m_upper;
} TimestampInterval;

// NOLINTEND(modernize-use-using)
// NOLINTEND(modernize-deprecated-headers)

#endif  // FFI_GO_DEF_H
