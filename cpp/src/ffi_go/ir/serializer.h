#ifndef FFI_GO_IR_SERIALIZER_H
#define FFI_GO_IR_SERIALIZER_H

// header must support C, making modernize checks inapplicable
// NOLINTBEGIN(modernize-deprecated-headers)
// NOLINTBEGIN(modernize-use-trailing-return-type)
// NOLINTBEGIN(modernize-use-using)

#include <stdint.h>
#include <stdlib.h>

// TODO: replace with clp c-compatible header once it exists
typedef int64_t epoch_time_ms_t;

/**
 */
int ir_serializer_serialize_eight_byte_log_event(
        char const* message,
        size_t message_size,
        epoch_time_ms_t timestamp,
        void* log_event_serializer_ptr,
        void** ir_buf_ptr,
        void* ir_buf_size
);

int ir_serializer_serialize_four_byte_log_event(
        char const* message,
        size_t message_size,
        epoch_time_ms_t timestamp_delta,
        void* log_event_serializer_ptr,
        void** ir_buf_ptr,
        void* ir_buf_size
);

int ir_serializer_serialize_eight_byte_preamble(
        char const* ts_pattern,
        size_t ts_pattern_size,
        char const* ts_pattern_syntax,
        size_t ts_pattern_syntax_size,
        char const* time_zone_id,
        size_t time_zone_id_size,
        void** log_event_serializer_ptr,
        void** ir_buf_ptr,
        size_t* ir_buf_size
);

/**
 *
 * @return ffi::ir_steram::IRErrorCode forwared from either
 *  ffi::ir_stream::get_encoding_type or
 *  ffi::ir_stream::decode_preamble
 */
int ir_serializer_serialize_four_byte_preamble(
        char const* ts_pattern,
        size_t ts_pattern_size,
        char const* ts_pattern_syntax,
        size_t ts_pattern_syntax_size,
        char const* time_zone_id,
        size_t time_zone_id_size,
        epoch_time_ms_t reference_ts,
        void** log_event_serializer_ptr,
        void** ir_buf_ptr,
        size_t* ir_buf_size
);

/**
 * Clean up the underlying ir::LogEventSerializer of a Go ir.IRSerializer.
 * @param[in] log_event_serializer
 * @return ffi::ir_steram::IRErrorCode
 */
void ir_serializer_close(void* log_event_serializer);

// NOLINTEND(modernize-use-using)
// NOLINTEND(modernize-use-trailing-return-type)
// NOLINTEND(modernize-deprecated-headers)
#endif  // FFI_GO_IR_SERIALIZER_H
