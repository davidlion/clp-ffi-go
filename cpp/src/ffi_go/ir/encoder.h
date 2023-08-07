#ifndef FFI_GO_IR_ENCODER_H
#define FFI_GO_IR_ENCODER_H

// header must support C, making modernize checks inapplicable
// NOLINTBEGIN(modernize-deprecated-headers)
// NOLINTBEGIN(modernize-use-trailing-return-type)
// NOLINTBEGIN(modernize-use-using)

#include <stdint.h>
#include <stdlib.h>

/**
 * Given a log message, encode it into a CLP IR object with eight byte encoding.
 * An ir::Encoder must be provided to use as the backing storage for the
 * corresponding Go ir.Encoder. All pointer parameters must be non-null (non-nil
 * Cgo C.<type> pointer or unsafe.Pointer from Go).
 * @param[in] log_msg Log message to encode
 * @param[in] log_msg_size Size of log_msg
 * @param[in] ir_encoder ir::Encoder to be used as storage for the encoded log
 *     message
 * @param[out] logtype_ptr Type of the log message (the log message with
 *     variables extracted and replaced with placeholders)
 * @param[out] logtype_size Size of the logtype of logtype_ptr
 * @param[out] vars_ptr Array of encoded variables
 * @param[out] vars_size Size of the array of vars_ptr
 * @param[out] dict_vars_ptr String containing all dictionary variables
 *     concatenated together
 * @param[out] dict_vars_size Size of the string of dict_vars_ptr
 * @param[out] dict_var_end_offsets_ptr Array of offsets into dict_vars makring
 *     the end of a dictionary variable
 * @param[out] dict_var_end_offsets_size Size of the array of
 *     dict_var_end_offsets_ptr
 * @return ffi::ir_stream::IRErrorCode_Corrupted_IR if ffi::encode_message
 *   returns false
 * @return ffi::ir_stream::IRErrorCode_Success on success
 */
int ir_encoder_encode_eight_byte_log_message(
        char const* log_msg,
        size_t log_msg_size,
        void* ir_encoder,
        char** logtype_ptr,
        size_t* logtype_size,
        int64_t** vars_ptr,
        size_t* vars_size,
        char** dict_vars_ptr,
        size_t* dict_vars_size,
        int32_t** dict_var_end_offsets_ptr,
        size_t* dict_var_end_offsets_size
);

/**
 * Given a log message, encode it into a CLP IR object with four byte encoding.
 * An ir::Encoder must be provided to use as the backing storage for the
 * corresponding Go ir.Encoder. All pointer parameters must be non-null (non-nil
 * Cgo C.<type> pointer or unsafe.Pointer from Go).
 * @param[in] log_msg Log message to encode
 * @param[in] log_msg_size Size of log_msg
 * @param[in] ir_encoder ir::Encoder to be used as storage for the encoded log
 *     message
 * @param[out] logtype_ptr Type of the log message (the log message with
 *     variables extracted and replaced with placeholders)
 * @param[out] logtype_size Size of logtype
 * @param[out] vars_ptr Array of encoded variables
 * @param[out] vars_size Size of vars
 * @param[out] dict_vars_ptr String containing all dictionary variables
 *     concatenated together
 * @param[out] dict_vars_size Size of dict_vars
 * @param[out] dict_var_end_offsets_ptr Array of offsets into dict_vars makring
 *     the end of a dictionary variable
 * @param[out] dict_var_end_offsets_size Size of dict_var_end_offsets
 * @return ffi::ir_stream::IRErrorCode_Corrupted_IR if ffi::encode_message
 *   returns false
 * @return ffi::ir_stream::IRErrorCode_Success on success
 */
int ir_encoder_encode_four_byte_log_message(
        char const* log_msg,
        size_t log_msg_size,
        void* ir_encoder,
        char** logtype_ptr,
        size_t* logtype_size,
        int32_t** vars_ptr,
        size_t* vars_size,
        char** dict_vars_ptr,
        size_t* dict_vars_size,
        int32_t** dict_var_end_offsets_ptr,
        size_t* dict_var_end_offsets_size
);

/**
 * Create a ir::Encoder used as the underlying data storage for a Go ir.Encoder.
 * @return New ir::Encoder's address
 */
void* ir_encoder_eight_byte_new();

/**
 * @copydoc ir_encoder_eight_byte_new()
 */
void* ir_encoder_four_byte_new();

/**
 * Clean up the underlying ir::Encoder of a Go ir.Encoder.
 * @param[in] ir_encoder Address of a ir::Encoder created and returned by
 *   ir_encoder_eight_byte_new
 */
void ir_encoder_eight_byte_close(void* ir_encoder);

/**
 * Clean up the underlying ir::Encoder of a Go ir.Encoder.
 * @param[in] ir_encoder Address of a ir::Encoder created and returned by
 *   ir_encoder_four_byte_new
 */
void ir_encoder_four_byte_close(void* ir_encoder);

// NOLINTEND(modernize-use-using)
// NOLINTEND(modernize-use-trailing-return-type)
// NOLINTEND(modernize-deprecated-headers)
#endif  // FFI_GO_IR_ENCODER_H
