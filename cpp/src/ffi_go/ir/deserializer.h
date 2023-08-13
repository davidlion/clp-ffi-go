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

typedef struct {
    void* m_buf;
    size_t m_buf_size;
} BufView;

typedef struct {
    char* m_log_message;
    size_t m_log_message_size;
    epoch_time_ms_t m_timestamp;
} CgoLogEvent;

/**
 * Given a CLP IR buffer with eight byte encoding, deserialize the next log
 * event. Return the components of the found log event and the buffer position
 * it ends at. All pointer parameters must be non-null (non-nil Cgo C.<type>
 * pointer or unsafe.Pointer from Go).
 * @param[in] buf Byte buffer/slice containing CLP IR
 * @param[in] buf_size Size of buf
 * @param[in] ir_deserializer ir::Deserializer to be used as storage for a found
 *     log event
 * @param[out] buf_pos Position in buf read to
 * @param[out] log_msg_ptr Log message of the event stored in ir_deserializer
 * @param[out] log_msg_size Size of the log message of log_msg_ptr
 * @param[out] timestamp Timestamp of the log event
 * @return ffi::ir_stream::IRErrorCode forwarded from
 *     ffi::ir_stream::eight_byte_encoding::decode_next_message
 */
int ir_deserializer_deserialize_eight_byte_log_event(
        BufView buf_view,
        void* ir_deserializer,
        size_t* buf_pos,
        CgoLogEvent* log_event
);

/**
 * Given a CLP IR buffer with four byte encoding, deserialize the next log
 * event. Return the components of the found log event and the buffer position
 * it ends at. All pointer parameters must be non-null (non-nil Cgo C.<type>
 * pointer or unsafe.Pointer from Go).
 * @param[in] buf Byte buffer/slice containing CLP IR
 * @param[in] buf_size Size of buf
 * @param[in] ir_deserializer ir::Deserializer to be used as storage for a found
 *     log event
 * @param[out] buf_pos Position in buf read to
 * @param[out] log_msg_ptr Log message of the event stored in ir_deserializer
 * @param[out] log_msg_size Size of the log message of log_msg_ptr
 * @param[out] timestamp Timestamp of the log event
 * @return ffi::ir_stream::IRErrorCode forwarded from
 *     ffi::ir_stream::four_byte_encoding::decode_next_message
 */
int ir_deserializer_deserialize_four_byte_log_event(
        BufView buf_view,
        void* ir_deserializer,
        size_t* buf_pos,
        CgoLogEvent* log_event
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
 * @param[out] buf_pos Position in buf read to
 * @param[out] ir_encoding IR encoding type (1: four byte, 0: eight byte)
 * @param[out] metadata_type Type of metadata in preamble (e.g. json)
 * @param[out] metadata_pos Position in buf where the metadata begins
 * @param[out] metadata_size Size of the metadata inside the buffer/preamble
 * @param[out] ir_deserializer_ptr Address of a new ir::Deserializer
 * @return ffi::ir_stream::IRErrorCode forwarded from either
 *     ffi::ir_stream::get_encoding_type or ffi::ir_stream::decode_preamble
 */
int ir_deserializer_deserialize_preamble(
        BufView buf_view,
        size_t* buf_pos,
        int8_t* ir_encoding,
        int8_t* metadata_type,
        size_t* metadata_pos,
        uint16_t* metadata_size,
        void** ir_deserializer_ptr
);

/**
 * Clean up the underlying ir::Deserializer of a Go ir.Deserializer.
 * @param[in] ir_deserializer The address of a ir::Deserializer created and
 *     returned by ir_deserializer_deserialize_preamble
 */
void ir_deserializer_close(void* ir_deserializer);

// NOLINTEND(modernize-use-using)
// NOLINTEND(modernize-use-trailing-return-type)
// NOLINTEND(modernize-deprecated-headers)
#endif  // FFI_GO_IR_DESERIALIZER_H
