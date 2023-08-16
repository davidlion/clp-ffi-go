#ifndef FFI_GO_IR_DESERIALIZER_H
#define FFI_GO_IR_DESERIALIZER_H

// header must support C, making modernize checks inapplicable
// NOLINTBEGIN(modernize-deprecated-headers)
// NOLINTBEGIN(modernize-use-trailing-return-type)

#include <stdint.h>
#include <stdlib.h>

#include <ffi_go/defs.h>
#include <ffi_go/search/wildcard.h>

/**
 * Given a CLP IR buffer with eight byte encoding, deserialize the next log
 * event. Return the components of the found log event and the buffer position
 * it ends at. All pointer parameters must be non-null (non-nil Cgo C.<type>
 * pointer or unsafe.Pointer from Go).
 * @param[in] ir_view Byte buffer/slice containing CLP IR
 * @param[in] ir_deserializer ir::Deserializer to be used as storage for a found
 *     log event
 * @param[out] ir_pos Position in ir_view read to
 * @param[out] log_msg_ptr Log message of the event stored in ir_deserializer
 * @param[out] log_msg_size Size of the log message of log_msg_ptr
 * @param[out] timestamp Timestamp of the log event
 * @return ffi::ir_stream::IRErrorCode forwarded from
 *     ffi::ir_stream::eight_byte_encoding::decode_next_message
 */
int ir_deserializer_deserialize_eight_byte_log_event(
        ByteView ir_view,
        void* ir_deserializer,
        size_t* ir_pos,
        LogEventView* log_event
);

/**
 * Given a CLP IR buffer with four byte encoding, deserialize the next log
 * event. Return the components of the found log event and the buffer position
 * it ends at. All pointer parameters must be non-null (non-nil Cgo C.<type>
 * pointer or unsafe.Pointer from Go).
 * @param[in] ir_view Byte buffer/slice containing CLP IR
 * @param[in] ir_deserializer ir::Deserializer to be used as storage for a found
 *     log event
 * @param[out] ir_pos Position in ir_view read to
 * @param[out] log_msg_ptr Log message of the event stored in ir_deserializer
 * @param[out] log_msg_size Size of the log message of log_msg_ptr
 * @param[out] timestamp Timestamp of the log event
 * @return ffi::ir_stream::IRErrorCode forwarded from
 *     ffi::ir_stream::four_byte_encoding::decode_next_message
 */
int ir_deserializer_deserialize_four_byte_log_event(
        ByteView ir_view,
        void* ir_deserializer,
        size_t* ir_pos,
        LogEventView* log_event
);

/**
 * Given
 * @param[in] ir_view
 * @param[in] ir_deserializer
 * @param[in] time_interval
 * @param[in] queries
 * @param[in] queries_size
 * @param[out] ir_pos
 * @param[out] log_event
 * @param[out] matching_query
 * @return ffi::ir_stream::IRErrorCode forwarded from
 *     ffi::ir_stream::four_byte_encoding::decode_next_message
 * @return ffi::ir_stream::IRErrorCode_Unsupported_Version + 1 if no query is
 *     found before time_interval.m_upper // TODO this must be replaced
 */
int ir_deserializer_deserialize_eight_byte_wildcard_match(
        ByteView ir_view,
        void* ir_deserializer,
        TimestampInterval time_interval,
        WildcardQueryView* queries,
        size_t queries_size,
        size_t* ir_pos,
        LogEventView* log_event,
        size_t* matching_query
);

int ir_deserializer_deserialize_four_byte_wildcard_match(
        ByteView ir_view,
        void* ir_deserializer,
        TimestampInterval time_interval,
        WildcardQueryView* queries,
        size_t queries_size,
        size_t* ir_pos,
        LogEventView* log_event,
        size_t* matching_query
);

/**
 * Given a CLP IR buffer (any encoding), attempt to deserialize a premable and
 * extract its information. An ir::Deserializer will be allocated to use as the
 * backing storage for a Go ir.Deserializer (i.e. subsequent calls to
 * ir_deserializer_deserialize_*_log_event). It is left to the Go layer to read
 * the metadata based on the returned type. All pointer parameters must be
 * non-null (non-nil Cgo C.<type> pointer or unsafe.Pointer from Go).
 * @param[in] ir_view Byte buffer/slice containing CLP IR
 * @param[out] ir_pos Position in ir_view read to
 * @param[out] ir_encoding IR encoding type (1: four byte, 0: eight byte)
 * @param[out] metadata_type Type of metadata in preamble (e.g. json)
 * @param[out] metadata_pos Position in ir_view where the metadata begins
 * @param[out] metadata_size Size of the metadata (in bytes)
 * @param[out] ir_deserializer_ptr Address of a new ir::Deserializer
 * @return ffi::ir_stream::IRErrorCode forwarded from either
 *     ffi::ir_stream::get_encoding_type or ffi::ir_stream::decode_preamble
 */
int ir_deserializer_deserialize_preamble(
        ByteView ir_view,
        size_t* ir_pos,
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

// NOLINTEND(modernize-use-trailing-return-type)
// NOLINTEND(modernize-deprecated-headers)
#endif  // FFI_GO_IR_DESERIALIZER_H
