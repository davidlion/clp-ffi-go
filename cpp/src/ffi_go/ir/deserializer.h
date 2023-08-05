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
 * Given a CLP IR buffer with eight byte encoding, deserialize the next log
 * event. The buffer position is updated to the end of the found log event. All
 * pointer parameters must be non-null (non-nil Cgo C.<type> pointer or
 * unsafe.Pointer from Go).
 * @param[in] buf Byte buffer/slice containing CLP IR
 * @param[in] buf_size Size of buf
 * @param[in,out] buf_pos The current position in buf to begin reading from and
 *   returns the position read to
 * @param[in] ir_deserializer The ir::Deserializer to be used as storage for a
 *   found log event
 * @param[out] message Returns a pointer to the log message of the event stored
 *   in ir_deserializer
 * @param[out] message_size Returns the size of message
 * @param[out] timestamp Returns the timestamp of the log event
 * @return ffi::ir_stream::IRErrorCode forwarded from
 *   ffi::ir_stream::eight_byte_encoding::decode_next_message
 */
int ir_deserializer_deserialize_eight_byte_log_event(
        void* buf,
        size_t buf_size,
        size_t* buf_pos,
        void* ir_deserializer,
        char** message,
        size_t* message_size,
        epoch_time_ms_t* timestamp
);

/**
 * Given a CLP IR buffer with four byte encoding, deserialize the next log
 * event. The buffer position is updated to the end of the found log event. All
 * pointer parameters must be non-null (non-nil Cgo C.<type> pointer or
 * unsafe.Pointer from Go).
 * @param[in] buf Byte buffer/slice containing CLP IR
 * @param[in] buf_size Size of buf
 * @param[in,out] buf_pos The current position in buf to begin reading from and
 *   returns the position read to
 * @param[in] ir_deserializer The ir::Deserializer to be used as storage for a
 *   found log event
 * @param[out] message Returns a pointer to the log message of the event stored
 *   in ir_deserializer
 * @param[out] message_size Returns the size of message
 * @param[out] timestamp_delta Returns the timestamp delta of the log event to
 *   the previous log event in the IR
 * @return ffi::ir_stream::IRErrorCode forwarded from
 *   ffi::ir_stream::four_byte_encoding::decode_next_message
 */
int ir_deserializer_deserialize_four_byte_log_event(
        void* buf,
        size_t buf_size,
        size_t* buf_pos,
        void* ir_deserializer,
        char** message,
        size_t* message_size,
        epoch_time_ms_t* timestamp_delta
);

/**
 * Given a CLP IR buffer (any encoding), attempt to deserialize a premable and
 * extract its information. An ir::Deserializer will be allocated to use as the
 * backing storage for a Go ir.Deserializer (i.e. subsequent calls to
 * ir_deserializer_deserialize_*_log_event). It is left to the Go layer to read
 * the metadata based on the returned type. All pointer parameters must be
 * non-null (non-nil Cgo C.<type> pointer or unsafe.Pointer from Go).
 * @param[in] buf Buffer containing CLP IR
 * @param[in] buf_size Size of buf
 * @param[in,out] buf_pos The current position in buf to begin reading from and
 *   returns the position read to
 * @param[out] ir_encoding Returns the encoding type (1: four byte, 0: eight byte)
 * @param[out] metadata_type Returns the metadata type (e.g. json)
 * @param[out] metadata_pos Returns the position in buf where the metadata begins
 * @param[out] metadata_size Returns the size of the metadata inside the
 *   buffer/preamble
 * @param[out] ir_deserializer_ptr Returns the address of a new Deserializer
 * @return ffi::ir_stream::IRErrorCode forwarded from either
 *   ffi::ir_stream::get_encoding_type or ffi::ir_stream::decode_preamble
 */
int ir_deserializer_deserialize_preamble(
        void* buf,
        size_t buf_size,
        size_t* buf_pos,
        int8_t* ir_encoding,
        int8_t* metadata_type,
        size_t* metadata_pos,
        uint16_t* metadata_size,
        void** ir_deserializer_ptr
);

/**
 * Clean up the underlying ir::Deserializer of a Go ir.Deserializer.
 * @param[in] ir_deserializer An address of a deserializer created and returned
 *   by ir_deserializer_deserialize_preamble
 */
void ir_deserializer_close(void* ir_deserializer);

// NOLINTEND(modernize-use-using)
// NOLINTEND(modernize-use-trailing-return-type)
// NOLINTEND(modernize-deprecated-headers)
#endif  // FFI_GO_IR_DESERIALIZER_H
