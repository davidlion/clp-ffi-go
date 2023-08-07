#include "encoder.h"

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
    auto encode_log_message(
            char const* log_msg,
            size_t log_msg_size,
            void* ir_encoder,
            char** logtype_ptr,
            size_t* logtype_size,
            encoded_var_t** vars_ptr,
            size_t* vars_size,
            char** dict_vars_ptr,
            size_t* dict_vars_size,
            int32_t** dict_var_end_offsets_ptr,
            size_t* dict_var_end_offsets_size
    ) -> int {
        Encoder<encoded_var_t>* encoder{static_cast<Encoder<encoded_var_t>*>(ir_encoder)};
        LogMessage<encoded_var_t>& ir_log_msg = encoder->m_log_message;
        ir_log_msg.reserve(log_msg_size);

        std::string_view const log_msg_view{log_msg, log_msg_size};
        std::vector<int32_t> dict_var_offsets;
        if (false
            == ffi::encode_message<encoded_var_t>(
                    log_msg_view,
                    ir_log_msg.m_logtype,
                    ir_log_msg.m_vars,
                    dict_var_offsets
            ))
        {
            return static_cast<int>(IRErrorCode_Corrupted_IR);
        }

        // dict_var_offsets contains begin_pos followed by end_pos of each
        // dictionary variable in the message
        int32_t prev_end_off = 0;
        for (size_t i = 0; i < dict_var_offsets.size(); i += 2) {
            int32_t const begin_pos = dict_var_offsets[i];
            int32_t const end_pos = dict_var_offsets[i + 1];
            ir_log_msg.m_dict_vars.insert(
                    ir_log_msg.m_dict_vars.begin() + prev_end_off,
                    log_msg_view.begin() + begin_pos,
                    log_msg_view.begin() + end_pos
            );
            prev_end_off = prev_end_off + (end_pos - begin_pos);
            ir_log_msg.m_dict_var_end_offsets.push_back(prev_end_off);
        }

        *logtype_ptr = ir_log_msg.m_logtype.data();
        *logtype_size = ir_log_msg.m_logtype.size();
        *vars_ptr = ir_log_msg.m_vars.data();
        *vars_size = ir_log_msg.m_vars.size();
        *dict_vars_ptr = ir_log_msg.m_dict_vars.data();
        *dict_vars_size = ir_log_msg.m_dict_vars.size();
        *dict_var_end_offsets_ptr = ir_log_msg.m_dict_var_end_offsets.data();
        *dict_var_end_offsets_size = ir_log_msg.m_dict_var_end_offsets.size();
        return static_cast<int>(IRErrorCode_Success);
    }
}  // namespace

extern "C" auto ir_encoder_encode_eight_byte_log_message(
        char const* log_msg,
        size_t log_msg_size,
        void* ir_encoder,
        char** logtype_ptr,
        size_t* logtype_size,
        ffi::eight_byte_encoded_variable_t** vars_ptr,
        size_t* vars_size,
        char** dict_vars_ptr,
        size_t* dict_vars_size,
        int32_t** dict_var_end_offsets_ptr,
        size_t* dict_var_end_offsets_size
) -> int {
    return encode_log_message<ffi::eight_byte_encoded_variable_t>(
            log_msg,
            log_msg_size,
            ir_encoder,
            logtype_ptr,
            logtype_size,
            vars_ptr,
            vars_size,
            dict_vars_ptr,
            dict_vars_size,
            dict_var_end_offsets_ptr,
            dict_var_end_offsets_size
    );
}

extern "C" auto ir_encoder_encode_four_byte_log_message(
        char const* log_msg,
        size_t log_msg_size,
        void* ir_encoder,
        char** logtype_ptr,
        size_t* logtype_size,
        ffi::four_byte_encoded_variable_t** vars_ptr,
        size_t* vars_size,
        char** dict_vars_ptr,
        size_t* dict_vars_size,
        int32_t** dict_var_end_offsets_ptr,
        size_t* dict_var_end_offsets_size
) -> int {
    return encode_log_message<ffi::four_byte_encoded_variable_t>(
            log_msg,
            log_msg_size,
            ir_encoder,
            logtype_ptr,
            logtype_size,
            vars_ptr,
            vars_size,
            dict_vars_ptr,
            dict_vars_size,
            dict_var_end_offsets_ptr,
            dict_var_end_offsets_size
    );
}

extern "C" auto ir_encoder_eight_byte_new() -> void* {
    return new Encoder<ffi::eight_byte_encoded_variable_t>{};
}

extern "C" auto ir_encoder_four_byte_new() -> void* {
    return new Encoder<ffi::four_byte_encoded_variable_t>{};
}

extern "C" auto ir_encoder_eight_byte_close(void* ir_encoder) -> void {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    delete static_cast<Encoder<ffi::eight_byte_encoded_variable_t>*>(ir_encoder);
}

extern "C" auto ir_encoder_four_byte_close(void* ir_encoder) -> void {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    delete static_cast<Encoder<ffi::four_byte_encoded_variable_t>*>(ir_encoder);
}
}  // namespace ffi_go::ir
