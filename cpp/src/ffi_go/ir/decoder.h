#ifndef FFI_GO_IR_DECODER_H
#define FFI_GO_IR_DECODER_H

// header must support C, making modernize checks inapplicable
// NOLINTBEGIN(modernize-deprecated-headers)
// NOLINTBEGIN(modernize-use-trailing-return-type)
// NOLINTBEGIN(modernize-use-using)

#include <stdint.h>
#include <stdlib.h>

/**
 * @param[in] logtype
 * @param[in] logtype_size
 * @param[in] vars
 * @param[in] vars_size
 * @param[in] dict_vars
 * @param[in] dict_vars_size
 * @param[in] dict_var_end_offsets
 * @param[in] dict_var_end_offsets_size
 * @param[in,out] log_msg
 * @param[out] log_msg
 * @param[out] log_msg_size
 * @return
 */
int ir_decoder_decode_eight_byte_log_message(
        char const* logtype,
        size_t logtype_size,
        int64_t* vars,
        size_t vars_size,
        char const* dict_vars,
        size_t dict_vars_size,
        int32_t const* dict_var_end_offsets,
        size_t dict_var_end_offsets_size,
        void* log_msg,
        char** msg,
        size_t* msg_size
);

int ir_decoder_decode_four_byte_log_message(
        char const* logtype,
        size_t logtype_size,
        int32_t* vars,
        size_t vars_size,
        char const* dict_vars,
        size_t dict_vars_size,
        int32_t const* dict_var_end_offsets,
        size_t dict_var_end_offsets_size,
        void* log_msg,
        char** msg,
        size_t* msg_size
);

/**
 * Create the underlying data structure for a Go ir.IRDecoder.
 * @return The new data structure's address
 */
void* ir_decoder_new();

/**
 * Clean up an underlying data structure of a Go ir.IRDecoder.
 * @param[in] decoder A address returned by decoder_new
 */
void ir_decoder_close(void* decoder);

// NOLINTEND(modernize-use-using)
// NOLINTEND(modernize-use-trailing-return-type)
// NOLINTEND(modernize-deprecated-headers)
#endif  // FFI_GO_IR_DECODER_H