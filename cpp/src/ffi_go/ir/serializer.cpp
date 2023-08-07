#include "serializer.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <clp/components/core/src/ffi/encoding_methods.hpp>
#include <clp/components/core/src/ffi/ir_stream/decoding_methods.hpp>
#include <clp/components/core/src/ffi/ir_stream/encoding_methods.hpp>

#include <ffi_go/ir/LogTypes.hpp>
#include <ffi_go/LogTypes.hpp>

namespace ffi_go::ir {
using namespace ffi;
using namespace ffi::ir_stream;

namespace {
    template <class encoded_variable_t>
    auto serialize_log_event(
            char const* log_msg,
            size_t log_msg_size,
            epoch_time_ms_t timestamp_or_delta,
            void* ir_serializer,
            void** ir_buf_ptr,
            void* ir_buf_size
    ) -> int {
        Serializer* serializer{static_cast<Serializer*>(ir_serializer)};
        std::string_view const log_msg_view(log_msg, log_msg_size);
        serializer->m_ir_buf.clear();

        bool success{false};
        if constexpr (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
            success = eight_byte_encoding::encode_message(
                    timestamp_or_delta,
                    log_msg_view,
                    serializer->m_logtype,
                    serializer->m_ir_buf
            );
        } else if constexpr (std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>) {
            success = four_byte_encoding::encode_message(
                    timestamp_or_delta,
                    log_msg_view,
                    serializer->m_logtype,
                    serializer->m_ir_buf
            );
        } else {
            static_assert(cAlwaysFalse<encoded_variable_t>, "Invalid/unhandled encoding type");
        }
        if (false == success) {
            return static_cast<int>(IRErrorCode_Corrupted_IR);
        }

        *ir_buf_ptr = serializer->m_ir_buf.data();
        *static_cast<std::size_t*>(ir_buf_size) = serializer->m_ir_buf.size();
        return static_cast<int>(IRErrorCode_Success);
    }
}  // namespace

extern "C" auto ir_serializer_serialize_eight_byte_log_event(
        char const* log_msg,
        size_t log_msg_size,
        epoch_time_ms_t timestamp,
        void* ir_serializer,
        void** ir_buf_ptr,
        void* ir_buf_size
) -> int {
    return serialize_log_event<eight_byte_encoded_variable_t>(
            log_msg,
            log_msg_size,
            timestamp,
            ir_serializer,
            ir_buf_ptr,
            ir_buf_size
    );
}

extern "C" auto ir_serializer_serialize_four_byte_log_event(
        char const* log_msg,
        size_t log_msg_size,
        epoch_time_ms_t timestamp_delta,
        void* ir_serializer,
        void** ir_buf_ptr,
        void* ir_buf_size
) -> int {
    return serialize_log_event<four_byte_encoded_variable_t>(
            log_msg,
            log_msg_size,
            timestamp_delta,
            ir_serializer,
            ir_buf_ptr,
            ir_buf_size
    );
}

extern "C" auto ir_serializer_serialize_eight_byte_preamble(
        char const* ts_pattern,
        size_t ts_pattern_size,
        char const* ts_pattern_syntax,
        size_t ts_pattern_syntax_size,
        char const* time_zone_id,
        size_t time_zone_id_size,
        void** ir_serializer_ptr,
        void** ir_buf_ptr,
        size_t* ir_buf_size
) -> int {
    Serializer* ir_serializer{new Serializer{}};
    *ir_serializer_ptr = ir_serializer;
    if (false
        == eight_byte_encoding::encode_preamble(
                std::string_view{ts_pattern, ts_pattern_size},
                std::string_view{ts_pattern_syntax, ts_pattern_syntax_size},
                std::string_view{time_zone_id, time_zone_id_size},
                ir_serializer->m_ir_buf
        ))
    {
        return static_cast<int>(IRErrorCode_Corrupted_IR);
    }

    *ir_buf_ptr = ir_serializer->m_ir_buf.data();
    *static_cast<std::size_t*>(ir_buf_size) = ir_serializer->m_ir_buf.size();
    return static_cast<int>(IRErrorCode_Success);
}

extern "C" auto ir_serializer_serialize_four_byte_preamble(
        char const* ts_pattern,
        size_t ts_pattern_size,
        char const* ts_pattern_syntax,
        size_t ts_pattern_syntax_size,
        char const* time_zone_id,
        size_t time_zone_id_size,
        epoch_time_ms_t reference_ts,
        void** ir_serializer_ptr,
        void** ir_buf_ptr,
        size_t* ir_buf_size
) -> int {
    Serializer* serializer{new Serializer{}};
    *ir_serializer_ptr = serializer;
    if (false
        == four_byte_encoding::encode_preamble(
                std::string_view{ts_pattern, ts_pattern_size},
                std::string_view{ts_pattern_syntax, ts_pattern_syntax_size},
                std::string_view{time_zone_id, time_zone_id_size},
                reference_ts,
                serializer->m_ir_buf
        ))
    {
        return static_cast<int>(IRErrorCode_Corrupted_IR);
    }

    *ir_buf_ptr = serializer->m_ir_buf.data();
    *static_cast<std::size_t*>(ir_buf_size) = serializer->m_ir_buf.size();
    return static_cast<int>(IRErrorCode_Success);
}

extern "C" auto ir_serializer_close(void* ir_serializer) -> void {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    delete static_cast<Serializer*>(ir_serializer);
}
}  // namespace ffi_go::ir
