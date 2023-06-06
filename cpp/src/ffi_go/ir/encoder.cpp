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
    template <class encoded_variable_t>
    auto encode_log_message(
            char const* msg,
            size_t msg_size,
            char** logtype,
            size_t* logtype_size,
            encoded_variable_t** vars,
            size_t* vars_size,
            char** dict_vars,
            size_t* dict_vars_size,
            int32_t** dict_var_end_offsets,
            size_t* dict_var_end_offsets_size,
            void* log_msg_ir
    ) -> int {
        LogMessageIR<encoded_variable_t>* lmsg_ir{
                static_cast<LogMessageIR<encoded_variable_t>*>(log_msg_ir)};
        lmsg_ir->reserve(msg_size);

        std::string_view const msg_view{msg, msg_size};
        std::vector<int32_t> dict_var_offsets;
        if (false
            == ffi::encode_message(msg_view, lmsg_ir->m_logtype, lmsg_ir->m_vars, dict_var_offsets))
        {
            return -1;
        }

        // dict_var_offsets contains begin_pos followed by end_pos of each
        // dictionary variable in the message
        int32_t prev_end_off = 0;
        for (size_t i = 0; i < dict_var_offsets.size(); i += 2) {
            int32_t const begin_pos = dict_var_offsets[i];
            int32_t const end_pos = dict_var_offsets[i + 1];
            lmsg_ir->m_dict_vars.insert(
                    lmsg_ir->m_dict_vars.begin() + prev_end_off,
                    msg_view.begin() + begin_pos,
                    msg_view.begin() + end_pos
            );
            prev_end_off = prev_end_off + (end_pos - begin_pos);
            lmsg_ir->m_dict_var_end_offsets.push_back(prev_end_off);
        }

        *logtype = lmsg_ir->m_logtype.data();
        *logtype_size = lmsg_ir->m_logtype.size();
        *vars = lmsg_ir->m_vars.data();
        *vars_size = lmsg_ir->m_vars.size();
        *dict_vars = lmsg_ir->m_dict_vars.data();
        *dict_vars_size = lmsg_ir->m_dict_vars.size();
        *dict_var_end_offsets = lmsg_ir->m_dict_var_end_offsets.data();
        *dict_var_end_offsets_size = lmsg_ir->m_dict_var_end_offsets.size();
        return 0;
    }
}  // namespace

extern "C" auto ir_encoder_encode_eight_byte_log_message(
        char const* msg,
        size_t msg_size,
        char** logtype,
        size_t* logtype_size,
        ffi::eight_byte_encoded_variable_t** vars,
        size_t* vars_size,
        char** dict_vars,
        size_t* dict_vars_size,
        int32_t** dict_var_end_offsets,
        size_t* dict_var_end_offsets_size,
        void* log_msg_ir
) -> int {
    return encode_log_message<ffi::eight_byte_encoded_variable_t>(
            msg,
            msg_size,
            logtype,
            logtype_size,
            vars,
            vars_size,
            dict_vars,
            dict_vars_size,
            dict_var_end_offsets,
            dict_var_end_offsets_size,
            log_msg_ir
    );
}

extern "C" auto ir_encoder_encode_four_byte_log_message(
        char const* msg,
        size_t msg_size,
        char** logtype,
        size_t* logtype_size,
        ffi::four_byte_encoded_variable_t** vars,
        size_t* vars_size,
        char** dict_vars,
        size_t* dict_vars_size,
        int32_t** dict_var_end_offsets,
        size_t* dict_var_end_offsets_size,
        void* log_msg_ir
) -> int {
    return encode_log_message<ffi::four_byte_encoded_variable_t>(
            msg,
            msg_size,
            logtype,
            logtype_size,
            vars,
            vars_size,
            dict_vars,
            dict_vars_size,
            dict_var_end_offsets,
            dict_var_end_offsets_size,
            log_msg_ir
    );
}

extern "C" auto ir_encoder_eight_byte_new() -> void* {
    return new LogMessageIR<ffi::eight_byte_encoded_variable_t>{};
}

extern "C" auto ir_encoder_four_byte_new() -> void* {
    return new LogMessageIR<ffi::four_byte_encoded_variable_t>{};
}

extern "C" auto ir_encoder_eight_byte_close(void* log_msg_ir) -> void {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    delete static_cast<LogMessageIR<ffi::eight_byte_encoded_variable_t>*>(log_msg_ir);
}

extern "C" auto ir_encoder_four_byte_close(void* log_msg_ir) -> void {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    delete static_cast<LogMessageIR<ffi::four_byte_encoded_variable_t>*>(log_msg_ir);
}
}  // namespace ffi_go::ir
