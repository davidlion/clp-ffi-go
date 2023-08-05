#include "decoder.h"

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <clp/components/core/src/ffi/encoding_methods.hpp>
#include <clp/components/core/src/ffi/ir_stream/decoding_methods.hpp>

#include <ffi_go/ir/LogTypes.hpp>
#include <ffi_go/LogTypes.hpp>

namespace ffi_go::ir {
using namespace ffi::ir_stream;

namespace {
    template <class encoded_var_t>
    auto decode_log_message(
            char const* logtype,
            size_t logtype_size,
            encoded_var_t* vars,
            size_t vars_size,
            char const* dict_vars,
            size_t dict_vars_size,
            int32_t const* dict_var_end_offsets,
            size_t dict_var_end_offsets_size,
            void* ir_decoder,
            char** msg_data,
            size_t* msg_size
    ) -> int {
        Decoder* decoder{static_cast<Decoder*>(ir_decoder)};
        ffi_go::LogMessage& log_msg = decoder->m_log_message;
        log_msg.reserve(logtype_size + dict_vars_size);

        IRErrorCode err{IRErrorCode_Success};
        try {
            log_msg = ffi::decode_message<encoded_var_t>(
                    std::string_view(logtype, logtype_size),
                    vars,
                    vars_size,
                    std::string_view(dict_vars, dict_vars_size),
                    dict_var_end_offsets,
                    dict_var_end_offsets_size
            );
        } catch (ffi::EncodingException& e) {
            err = IRErrorCode_Decode_Error;
        }

        *msg_data = log_msg.data();
        *msg_size = log_msg.size();
        return static_cast<int>(err);
    }
}  // namespace

extern "C" auto ir_decoder_decode_eight_byte_log_message(
        char const* logtype,
        size_t logtype_size,
        ffi::eight_byte_encoded_variable_t* vars,
        size_t vars_size,
        char const* dict_vars,
        size_t dict_vars_size,
        int32_t const* dict_var_end_offsets,
        size_t dict_var_end_offsets_size,
        void* ir_decoder,
        char** msg,
        size_t* msg_size
) -> int {
    return decode_log_message<ffi::eight_byte_encoded_variable_t>(
            logtype,
            logtype_size,
            vars,
            vars_size,
            dict_vars,
            dict_vars_size,
            dict_var_end_offsets,
            dict_var_end_offsets_size,
            ir_decoder,
            msg,
            msg_size
    );
}

extern "C" auto ir_decoder_decode_four_byte_log_message(
        char const* logtype,
        size_t logtype_size,
        ffi::four_byte_encoded_variable_t* vars,
        size_t vars_size,
        char const* dict_vars,
        size_t dict_vars_size,
        int32_t const* dict_var_end_offsets,
        size_t dict_var_end_offsets_size,
        void* ir_decoder,
        char** msg,
        size_t* msg_size
) -> int {
    return decode_log_message<ffi::four_byte_encoded_variable_t>(
            logtype,
            logtype_size,
            vars,
            vars_size,
            dict_vars,
            dict_vars_size,
            dict_var_end_offsets,
            dict_var_end_offsets_size,
            ir_decoder,
            msg,
            msg_size
    );
}

extern "C" auto ir_decoder_new() -> void* {
    return new Decoder{};
}

extern "C" auto ir_decoder_close(void* ir_decoder) -> void {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    delete static_cast<Decoder*>(ir_decoder);
}
}  // namespace ffi_go::ir
