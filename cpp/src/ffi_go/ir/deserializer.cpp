#include "deserializer.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

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
    auto deserialize_log_event(
            BufView buf_view,
            void* ir_deserializer,
            size_t* buf_pos,
            CgoLogEvent* log_event
    ) -> int {
        Deserializer* deserializer{static_cast<Deserializer*>(ir_deserializer)};
        IrBuffer ir_buf{static_cast<int8_t*>(buf_view.m_buf), buf_view.m_buf_size};

        IRErrorCode err{};
        epoch_time_ms_t timestamp{};
        if constexpr (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
            err = eight_byte_encoding::decode_next_message(
                    ir_buf,
                    deserializer->m_log_event.m_log_msg,
                    timestamp
            );
        } else if constexpr (std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>) {
            epoch_time_ms_t timestamp_delta{};
            err = four_byte_encoding::decode_next_message(
                    ir_buf,
                    deserializer->m_log_event.m_log_msg,
                    timestamp_delta
            );
            timestamp = deserializer->m_timestamp + timestamp_delta;
        } else {
            static_assert(cAlwaysFalse<encoded_variable_t>, "Invalid/unhandled encoding type");
        }
        if (IRErrorCode_Success != err) {
            return static_cast<int>(err);
        }
        deserializer->m_timestamp = timestamp;

        *buf_pos = ir_buf.get_cursor_pos();
        log_event->m_log_message = deserializer->m_log_event.m_log_msg.data();
        log_event->m_log_message_size = deserializer->m_log_event.m_log_msg.size();
        log_event->m_timestamp = deserializer->m_timestamp;
        return static_cast<int>(IRErrorCode_Success);
    }

    template <class encoded_variable_t>
    auto deserialize_query_match(
            BufView buf_view,
            void* ir_deserializer,
            size_t* buf_pos,
            char const* query,
            size_t query_size,
            int case_sensitive,
            epoch_time_ms_t min_timestamp,
            epoch_time_ms_t max_timestamp,
            CgoLogEvent* log_event
    ) -> int {
        Deserializer* deserializer{static_cast<Deserializer*>(ir_deserializer)};
        IrBuffer ir_buf{static_cast<int8_t*>(buf_view.m_buf), buf_view.m_buf_size};
        std::string const cleaned_query{
                clean_up_wildcard_search_string(std::string_view{query, query_size})};

        IRErrorCode err{};
        while (true) {
            epoch_time_ms_t timestamp{};
            if constexpr (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
                err = eight_byte_encoding::decode_next_message(
                        ir_buf,
                        deserializer->m_log_event.m_log_msg,
                        timestamp
                );
            } else if constexpr (std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>) {
                epoch_time_ms_t timestamp_delta{};
                err = four_byte_encoding::decode_next_message(
                        ir_buf,
                        deserializer->m_log_event.m_log_msg,
                        timestamp_delta
                );
                timestamp = deserializer->m_timestamp + timestamp_delta;
            } else {
                static_assert(cAlwaysFalse<encoded_variable_t>, "Invalid/unhandled encoding type");
            }

            if (IRErrorCode_Success != err) {
                return static_cast<int>(err);
            }
            deserializer->m_timestamp = timestamp;

            if (min_timestamp <= deserializer->m_timestamp
                && max_timestamp > deserializer->m_timestamp
                && wildcard_match_unsafe(
                        deserializer->m_log_event.m_log_msg,
                        cleaned_query,
                        static_cast<bool>(case_sensitive)
                ))
            {
                *buf_pos = ir_buf.get_cursor_pos();
                log_event->m_log_message = deserializer->m_log_event.m_log_msg.data();
                log_event->m_log_message_size = deserializer->m_log_event.m_log_msg.size();
                log_event->m_timestamp = deserializer->m_timestamp;
                return static_cast<int>(IRErrorCode_Success);
            }
        }
    }
}  // namespace

/* extern "C" auto ir_deserializer_deserialize_eight_byte_query_match( */
/*         void* buf, */
/*         size_t buf_size, */
/*         void* ir_deserializer, */
/*         size_t* buf_pos, */
/*         char** log_msg_ptr, */
/*         size_t* log_msg_size, */
/*         epoch_time_ms_t* timestamp */
/* ) -> int { */
/*     return deserialize_query_match<eight_byte_encoded_variable_t>( */
/*             buf, */
/*             buf_size, */
/*             ir_deserializer, */
/*             buf_pos, */
/*             log_msg_ptr, */
/*             log_msg_size, */
/*             timestamp */
/*     ); */
/* } */

extern "C" auto ir_deserializer_deserialize_eight_byte_log_event(
        BufView buf_view,
        void* ir_deserializer,
        size_t* buf_pos,
        CgoLogEvent* log_event
) -> int {
    return deserialize_log_event<eight_byte_encoded_variable_t>(
            buf_view,
            ir_deserializer,
            buf_pos,
            log_event
    );
}

extern "C" auto ir_deserializer_deserialize_four_byte_log_event(
        BufView buf_view,
        void* ir_deserializer,
        size_t* buf_pos,
        CgoLogEvent* log_event
) -> int {
    return deserialize_log_event<four_byte_encoded_variable_t>(
            buf_view,
            ir_deserializer,
            buf_pos,
            log_event
    );
}

extern "C" auto ir_deserializer_deserialize_preamble(
        BufView buf_view,
        size_t* buf_pos,
        int8_t* ir_encoding,
        int8_t* metadata_type,
        size_t* metadata_pos,
        uint16_t* metadata_size,
        void** ir_deserializer_ptr
) -> int {
    IrBuffer ir_buf{static_cast<int8_t*>(buf_view.m_buf), buf_view.m_buf_size};

    bool four_byte_encoding{};
    if (IRErrorCode const err{get_encoding_type(ir_buf, four_byte_encoding)};
        IRErrorCode_Success != err)
    {
        return static_cast<int>(err);
    }
    *ir_encoding = four_byte_encoding ? 1 : 0;

    if (IRErrorCode const err{
                decode_preamble(ir_buf, *metadata_type, *metadata_pos, *metadata_size)};
        IRErrorCode_Success != err)
    {
        return static_cast<int>(err);
    }

    *buf_pos = ir_buf.get_cursor_pos();
    *ir_deserializer_ptr = new Deserializer();
    return static_cast<int>(IRErrorCode_Success);
}

extern "C" auto ir_deserializer_close(void* ir_deserializer) -> void {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    delete static_cast<Deserializer*>(ir_deserializer);
}
}  // namespace ffi_go::ir
