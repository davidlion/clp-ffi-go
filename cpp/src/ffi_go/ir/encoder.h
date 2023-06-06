#ifndef FFI_GO_IR_ENCODER_H
#define FFI_GO_IR_ENCODER_H

// header must support C, making modernize checks inapplicable
// NOLINTBEGIN(modernize-deprecated-headers)
// NOLINTBEGIN(modernize-use-trailing-return-type)
// NOLINTBEGIN(modernize-use-using)

#include <stdint.h>
#include <stdlib.h>

/**
 * @param[in] msg
 * @param[in] msg_size
 * @param[out] logtype
 * @param[out] logtype_size
 * @param[out] vars
 * @param[out] vars_size
 * @param[out] dict_vars
 * @param[out] dict_vars_size
 * @param[out] dict_var_end_offsets
 * @param[out] dict_var_end_offsets_size
 * @param[out] log_msg_ir_ptr
 * @return
 */
int ir_encoder_encode_eight_byte_log_message(
        char const* msg,
        size_t msg_size,
        char** logtype,
        size_t* logtype_size,
        int64_t** vars,
        size_t* vars_size,
        char** dict_vars,
        size_t* dict_vars_size,
        int32_t** dict_var_end_offsets,
        size_t* dict_var_end_offsets_size,
        void* log_msg_ir
);

int ir_encoder_encode_four_byte_log_message(
        char const* msg,
        size_t msg_size,
        char** logtype,
        size_t* logtype_size,
        int32_t** vars,
        size_t* vars_size,
        char** dict_vars,
        size_t* dict_vars_size,
        int32_t** dict_var_end_offsets,
        size_t* dict_var_end_offsets_size,
        void* log_msg_ir
);

/**
 * Create the underlying data structure for a Go ir.IREncoder.
 * @return The new data structure's address
 */
void* ir_encoder_eight_byte_new();

/**
 * @copydoc encoder_eight_byte_new()
 */
void* ir_encoder_four_byte_new();

/**
 * Clean up an underlying data structure of a Go ir.IREncoder.
 * @param[in] decoder A address returned by encoder_eight_byte_new
 */
void ir_encoder_eight_byte_close(void* log_msg_ir);

/**
 * Clean up an underlying data structure of a Go ir.IREncoder.
 * @param[in] decoder A address returned by encoder_four_byte_new
 */
void ir_encoder_four_byte_close(void* log_msg_ir);

// NOLINTEND(modernize-use-using)
// NOLINTEND(modernize-use-trailing-return-type)
// NOLINTEND(modernize-deprecated-headers)
#endif  // FFI_GO_IR_ENCODER_H
