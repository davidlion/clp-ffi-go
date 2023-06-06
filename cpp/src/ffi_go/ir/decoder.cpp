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
    template <class encoded_variable_t>
    auto decode_log_message(
            char const* logtype,
            size_t logtype_size,
            encoded_variable_t* vars,
            size_t vars_size,
            char const* dict_vars,
            size_t dict_vars_size,
            int32_t const* dict_var_end_offsets,
            size_t dict_var_end_offsets_size,
            void* log_msg,
            char** msg_data,
            size_t* msg_size
    ) -> int {
        LogMessage* lmsg{static_cast<LogMessage*>(log_msg)};
        lmsg->reserve(logtype_size + dict_vars_size);

        IRErrorCode err{IRErrorCode_Success};
        try {
            lmsg->m_message = ffi::decode_message<encoded_variable_t>(
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

        *msg_data = lmsg->m_message.data();
        *msg_size = lmsg->m_message.size();
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
        void* log_msg,
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
            log_msg,
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
        void* log_msg,
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
            log_msg,
            msg,
            msg_size
    );
}

extern "C" auto ir_decoder_new() -> void* {
    return new LogMessage{};
}

extern "C" auto ir_decoder_close(void* decoder) -> void {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    delete static_cast<LogMessage*>(decoder);
}
}  // namespace ffi_go::ir
