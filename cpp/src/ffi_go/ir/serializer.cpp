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
using namespace ffi::ir_stream;

namespace {
    template <class encoded_variable_t>
    auto serialize_log_event(
            char const* message,
            size_t message_size,
            epoch_time_ms_t timestamp_or_delta,
            void* log_event_serializer,
            void** ir_buf_ptr,
            void* ir_buf_size
    ) -> int {
        LogEventSerializer* le_ser{static_cast<LogEventSerializer*>(log_event_serializer)};
        std::string_view const message_view(message, message_size);
        le_ser->m_ir_buf.clear();

        bool success{false};
        if constexpr (std::is_same_v<encoded_variable_t, ffi::eight_byte_encoded_variable_t>) {
            success = eight_byte_encoding::encode_message(
                    timestamp_or_delta,
                    message_view,
                    le_ser->m_logtype,
                    le_ser->m_ir_buf
            );
        } else if constexpr (std::is_same_v<encoded_variable_t, ffi::four_byte_encoded_variable_t>)
        {
            success = four_byte_encoding::encode_message(
                    timestamp_or_delta,
                    message_view,
                    le_ser->m_logtype,
                    le_ser->m_ir_buf
            );
        } else {
            static_assert(cAlwaysFalse<encoded_variable_t>, "Invalid/unhandled encoding type");
        }
        if (false == success) {
            return static_cast<int>(IRErrorCode_Corrupted_IR);
        }

        *ir_buf_ptr = le_ser->m_ir_buf.data();
        *static_cast<std::size_t*>(ir_buf_size) = le_ser->m_ir_buf.size();
        return static_cast<int>(IRErrorCode_Success);
    }

}  // namespace

extern "C" auto ir_serializer_serialize_eight_byte_log_event(
        char const* message,
        size_t message_size,
        epoch_time_ms_t timestamp,
        void* log_event_serializer,
        void** ir_buf_ptr,
        void* ir_buf_size
) -> int {
    return serialize_log_event<ffi::eight_byte_encoded_variable_t>(
            message,
            message_size,
            timestamp,
            log_event_serializer,
            ir_buf_ptr,
            ir_buf_size
    );
}

extern "C" auto ir_serializer_serialize_four_byte_log_event(
        char const* message,
        size_t message_size,
        epoch_time_ms_t timestamp_delta,
        void* log_event_serializer,
        void** ir_buf_ptr,
        void* ir_buf_size
) -> int {
    return serialize_log_event<ffi::four_byte_encoded_variable_t>(
            message,
            message_size,
            timestamp_delta,
            log_event_serializer,
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
        void** log_event_serializer_ptr,
        void** ir_buf_ptr,
        size_t* ir_buf_size
) -> int {
    LogEventSerializer* log_event_serializer{new LogEventSerializer{}};
    *log_event_serializer_ptr = log_event_serializer;
    if (false
        == eight_byte_encoding::encode_preamble(
                std::string_view{ts_pattern, ts_pattern_size},
                std::string_view{ts_pattern_syntax, ts_pattern_syntax_size},
                std::string_view{time_zone_id, time_zone_id_size},
                log_event_serializer->m_ir_buf
        ))
    {
        return static_cast<int>(IRErrorCode_Corrupted_IR);
    }

    *ir_buf_ptr = log_event_serializer->m_ir_buf.data();
    *static_cast<std::size_t*>(ir_buf_size) = log_event_serializer->m_ir_buf.size();
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
        void** log_event_serializer_ptr,
        void** ir_buf_ptr,
        size_t* ir_buf_size
) -> int {
    LogEventSerializer* log_event_serializer{new LogEventSerializer{}};
    *log_event_serializer_ptr = log_event_serializer;
    if (false
        == four_byte_encoding::encode_preamble(
                std::string_view{ts_pattern, ts_pattern_size},
                std::string_view{ts_pattern_syntax, ts_pattern_syntax_size},
                std::string_view{time_zone_id, time_zone_id_size},
                reference_ts,
                log_event_serializer->m_ir_buf
        ))
    {
        return static_cast<int>(IRErrorCode_Corrupted_IR);
    }

    *ir_buf_ptr = log_event_serializer->m_ir_buf.data();
    *static_cast<std::size_t*>(ir_buf_size) = log_event_serializer->m_ir_buf.size();
    return static_cast<int>(IRErrorCode_Success);
}

extern "C" auto ir_serializer_close(void* log_event_serializer) -> void {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    delete static_cast<LogEventSerializer*>(log_event_serializer);
}
}  // namespace ffi_go::ir
