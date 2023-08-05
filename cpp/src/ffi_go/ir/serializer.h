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
 * Given the fields of a log event, serialize them into an IR byte stream with
 * eight byte encoding. An ir::Serializer must be provided to use as the backing
 * storage for the corresponding Go ir.Serializer. All pointer parameters must
 * be non-null (non-nil Cgo C.<type> pointer or unsafe.Pointer from Go).
 * @param[in] message The log message to serialize
 * @param[in] message_size Size of message
 * @param[in] timestamp_delta The teimstamp delta to the previous log event in
 *   the CLP IR stream
 * @param[in] ir_serializer The ir::Serializer to be used as storage for the IR
 *   buffer
 * @param[out] ir_buf_ptr Returns the address of a buffer containing the log
 *   event encoded as CLP IR with eight byte encoding
 * @param[out] ir_buf_size Returns the size of the buffer
 * @return ffi::ir_stream::IRErrorCode forwared from
 *   ffi::ir_stream::eight_byte_encoding::encode_message
 */
int ir_serializer_serialize_eight_byte_log_event(
        char const* message,
        size_t message_size,
        epoch_time_ms_t timestamp,
        void* ir_serializer,
        void** ir_buf_ptr,
        void* ir_buf_size
);

/**
 * Given the fields of a log event, serialize them into an IR byte stream with
 * four byte encoding. An ir::Serializer must be provided to use as the backing
 * storage for the corresponding Go ir.Serializer. All pointer parameters must
 * be non-null (non-nil Cgo C.<type> pointer or unsafe.Pointer from Go).
 * @param[in] message The log message to serialize
 * @param[in] message_size Size of message
 * @param[in] timestamp_delta The teimstamp delta to the previous log event in
 *   the CLP IR stream
 * @param[in] ir_serializer The ir::Serializer to be used as storage for the IR
 *   buffer
 * @param[out] ir_buf_ptr Returns the address of a buffer containing the log
 *   event encoded as CLP IR with four byte encoding
 * @param[out] ir_buf_size Returns the size of the buffer
 * @return ffi::ir_stream::IRErrorCode forwared from
 *   ffi::ir_stream::four_byte_encoding::encode_message
 */
int ir_serializer_serialize_four_byte_log_event(
        char const* message,
        size_t message_size,
        epoch_time_ms_t timestamp_delta,
        void* ir_serializer,
        void** ir_buf_ptr,
        void* ir_buf_size
);

/**
 * Given the fields of a CLP IR premable, serialize them into an IR byte stream
 * with eight byte encoding. An ir::Serializer will be allocated to use as the
 * backing storage for a Go ir.Serializer (i.e. subsequent calls to
 * ir_serializer_serialize_*_log_event). All pointer parameters must be non-null
 * (non-nil Cgo C.<type> pointer or unsafe.Pointer from Go).
 * @param[in] ts_pattern A format string for the timestamp to be used when
 *   deserializing the IR
 * @param[in] ts_pattern_size Size of ts_pattern
 * @param[in] ts_pattern_syntax The type of the format string for understanding
 *   how to parse it
 * @param[in] ts_pattern_syntax_size Size of ts_pattern_syntax
 * @param[in] time_zone_id The TZID timezone of the timestamps in the IR
 * @param[in] time_zone_id_size Size of time_zone_id
 * @param[out] ir_serializer_ptr Returns the address of a new ir::Serializer
 * @param[out] ir_buf_ptr Returns the address of a buffer containing a CLP IR
 *   preamble with eight byte encoding
 * @param[out] ir_buf_size Returns the size of the buffer
 * @return ffi::ir_stream::IRErrorCode forwared from
 *   ffi::ir_stream::eight_byte_encoding::encode_preamble
 */
int ir_serializer_serialize_eight_byte_preamble(
        char const* ts_pattern,
        size_t ts_pattern_size,
        char const* ts_pattern_syntax,
        size_t ts_pattern_syntax_size,
        char const* time_zone_id,
        size_t time_zone_id_size,
        void** ir_serializer_ptr,
        void** ir_buf_ptr,
        size_t* ir_buf_size
);

/**
 * Given the fields of a CLP IR premable, serialize them into an IR byte stream
 * with four byte encoding. An ir::Serializer will be allocated to use as the
 * backing storage for a Go ir.Serializer (i.e. subsequent calls to
 * ir_serializer_serialize_*_log_event). All pointer parameters must be non-null
 * (non-nil Cgo C.<type> pointer or unsafe.Pointer from Go).
 * @param[in] ts_pattern A format string for the timestamp to be used when
 *   deserializing the IR
 * @param[in] ts_pattern_size Size of ts_pattern
 * @param[in] ts_pattern_syntax The type of the format string for understanding
 *   how to parse it
 * @param[in] ts_pattern_syntax_size Size of ts_pattern_syntax
 * @param[in] time_zone_id The TZID timezone of the timestamps in the IR
 * @param[in] time_zone_id_size Size of time_zone_id
 * @param[in] reference_ts The timestamp to use as the reference starting point
 *   of the CLP IR stream
 * @param[out] ir_serializer_ptr Returns the address of a new ir::Serializer
 * @param[out] ir_buf_ptr Returns the address of a buffer containing a CLP IR
 *   preamble with four byte encoding
 * @param[out] ir_buf_size Returns the size of the buffer
 * @return ffi::ir_stream::IRErrorCode forwared from
 *   ffi::ir_stream::four_byte_encoding::encode_preamble
 */
int ir_serializer_serialize_four_byte_preamble(
        char const* ts_pattern,
        size_t ts_pattern_size,
        char const* ts_pattern_syntax,
        size_t ts_pattern_syntax_size,
        char const* time_zone_id,
        size_t time_zone_id_size,
        epoch_time_ms_t reference_ts,
        void** ir_serializer_ptr,
        void** ir_buf_ptr,
        size_t* ir_buf_size
);

/**
 * Clean up the underlying ir::Serializer of a Go ir.Serializer.
 * @param[in] ir_serializer The address of a ir::Serializer created and returned
 *   by ir_serializer_serialize_*_preamble
 */
void ir_serializer_close(void* ir_serializer);

// NOLINTEND(modernize-use-using)
// NOLINTEND(modernize-use-trailing-return-type)
// NOLINTEND(modernize-deprecated-headers)
#endif  // FFI_GO_IR_SERIALIZER_H
