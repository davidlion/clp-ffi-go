#ifndef FFI_GO_IR_DESERIALIZER_H
#define FFI_GO_IR_DESERIALIZER_H

// header must support C, making modernize checks inapplicable
// NOLINTBEGIN(modernize-deprecated-headers)
// NOLINTBEGIN(modernize-use-trailing-return-type)
// NOLINTBEGIN(modernize-use-using)

#include <stdint.h>
#include <stdlib.h>

// TODO: replace with clp c-compatible header once it exists
typedef int64_t epoch_time_ms_t;

/**
 * Given an IR buffer, deserialize the next log event. The buffer position is
 * updated to the end of the found log event.
 * All pointer parameters must be non-null (non-nil cgo C.<type> pointer or
 * unsafe.Pointer from Go).
 * @param[in] buf Buffer containing CLP IR
 * @param[in] buf_size Size of buf
 * @param[in,out] buf_pos The current position in buf to begin reading from and
 * returns the position read to
 * @param[in] log_event_ptr The LogEvent object to be used as storage for a
 * found log event
 * @param[out] message Returns a pointer to the log message of the event stored
 *  in log_event_ptr
 * @param[out] message_size Returns the size of message
 * @param[out] timestamp Returns the timestamp of the log event
 * @return ffi::ir_steram::IRErrorCode forwarded from
 *  ffi::ir_stream::eight_byte_encoding::decode_next_message
 */
int ir_deserializer_deserialize_eight_byte_log_event(
        void* buf,
        size_t buf_size,
        size_t* buf_pos,
        void* log_event_ptr,
        char** message,
        size_t* message_size,
        epoch_time_ms_t* timestamp
);

/**
 * Given an IR buffer, deserialize the next log event. The buffer position is
 * updated to the end of the found log event.
 * All pointer parameters must be non-null (non-nil cgo C.<type> pointer or
 * unsafe.Pointer from Go).
 * @param[in] buf Buffer containing CLP IR
 * @param[in] buf_size Size of buf
 * @param[in,out] buf_pos The current position in buf to begin reading from and
 * returns the position read to
 * @param[in] log_event_ptr The LogEvent object to be used as storage for a
 * found log event
 * @param[out] message Returns a pointer to the log message of the event stored
 *  in log_event_ptr
 * @param[out] message_size Returns the size of message
 * @param[out] timestamp_delta Returns the timestamp delta of the log event with
 * its predecessor in the IR
 * @return ffi::ir_steram::IRErrorCode forwarded from
 *  ffi::ir_stream::eight_byte_encoding::decode_next_message
 */
int ir_deserializer_deserialize_four_byte_log_event(
        void* buf,
        size_t buf_size,
        size_t* buf_pos,
        void* log_event_ptr,
        char** message,
        size_t* message_size,
        epoch_time_ms_t* timestamp_delta
);

/**
 * Given an IR buffer, attempt to deserialize a premable and extract its
 * information. A LogEvent object will be allocated to use as the backing
 * storage for a go IR deserializer (i.e. subsequent calls to
 * *_decode_next_log_event). It is left to the Go layer to read the metadata
 * based on the returned type. All pointer parameters must be non-null (non-nil
 * cgo C.<type> pointer or unsafe.Pointer from Go).
 * @param[in] buf Buffer containing CLP IR
 * @param[in] buf_size Size of buf
 * @param[in,out] buf_pos The current position in buf to begin reading from and
 * returns the position read to
 * @param[out] ir_encoding Returns the encoding type (1: four byte, 0: eight byte)
 * @param[out] metadata_type Returns the metadata type (e.g. json)
 * @param[out] metadata_pos Returns the position in buf where the metadata begins
 * @param[out] metadata_size Returns the size of the metadata inside the
 * buffer/preamble
 * @param[out] log_event_ptr Returns a new LogEvent object
 * @return ffi::ir_steram::IRErrorCode forwarded from either
 *  ffi::ir_stream::get_encoding_type or
 *  ffi::ir_stream::decode_preamble
 */
int ir_deserializer_deserialize_preamble(
        void* buf,
        size_t buf_size,
        size_t* buf_pos,
        int8_t* ir_encoding,
        int8_t* metadata_type,
        size_t* metadata_pos,
        uint16_t* metadata_size,
        void** log_event_ptr
);

/**
 * Clean up the underlying LogEvent of a Go ir.IRDeserializer.
 * @param[in] log_event
 * @return ffi::ir_steram::IRErrorCode
 */
void ir_deserializer_close(void* log_event);

// NOLINTEND(modernize-use-using)
// NOLINTEND(modernize-use-trailing-return-type)
// NOLINTEND(modernize-deprecated-headers)
#endif  // FFI_GO_IR_DESERIALIZER_H
