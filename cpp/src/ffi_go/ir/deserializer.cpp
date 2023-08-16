#include "deserializer.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>

#include <clp/components/core/src/ffi/encoding_methods.hpp>
#include <clp/components/core/src/ffi/ir_stream/decoding_methods.hpp>
#include <clp/components/core/src/ffi/ir_stream/encoding_methods.hpp>

#include <ffi_go/defs.h>
#include <ffi_go/ir/LogTypes.hpp>
#include <ffi_go/LogTypes.hpp>
#include <ffi_go/search/wildcard.h>

namespace ffi_go::ir {
using namespace ffi;
using namespace ffi::ir_stream;

namespace {
    template <class encoded_variable_t>
    auto deserialize_log_event(
            ByteView ir_view,
            void* ir_deserializer,
            size_t* ir_pos,
            LogEventView* log_event
    ) -> int {
        Deserializer* deserializer{static_cast<Deserializer*>(ir_deserializer)};
        IrBuffer ir_buf{static_cast<int8_t*>(ir_view.m_data), ir_view.m_size};

        IRErrorCode err{};
        epoch_time_ms_t timestamp{};
        if constexpr (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
            err = eight_byte_encoding::decode_next_message(
                    ir_buf,
                    deserializer->m_log_event.m_log_message,
                    timestamp
            );
        } else if constexpr (std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>) {
            epoch_time_ms_t timestamp_delta{};
            err = four_byte_encoding::decode_next_message(
                    ir_buf,
                    deserializer->m_log_event.m_log_message,
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

        *ir_pos = ir_buf.get_cursor_pos();
        log_event->m_log_message.m_data = deserializer->m_log_event.m_log_message.data();
        log_event->m_log_message.m_size = deserializer->m_log_event.m_log_message.size();
        log_event->m_timestamp = deserializer->m_timestamp;
        return static_cast<int>(IRErrorCode_Success);
    }

    template <class encoded_variable_t>
    auto deserialize_wildcard_match(
            ByteView ir_view,
            void* ir_deserializer,
            TimestampInterval time_interval,
            WildcardQueryView* queries,
            size_t queries_size,
            size_t* ir_pos,
            LogEventView* log_event,
            size_t* matching_query
    ) -> int {
        Deserializer* deserializer{static_cast<Deserializer*>(ir_deserializer)};
        IrBuffer ir_buf{static_cast<int8_t*>(ir_view.m_data), ir_view.m_size};

        IRErrorCode err{};
        while (true) {
            epoch_time_ms_t timestamp{};
            if constexpr (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
                err = eight_byte_encoding::decode_next_message(
                        ir_buf,
                        deserializer->m_log_event.m_log_message,
                        timestamp
                );
            } else if constexpr (std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>) {
                epoch_time_ms_t timestamp_delta{};
                err = four_byte_encoding::decode_next_message(
                        ir_buf,
                        deserializer->m_log_event.m_log_message,
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

            if (time_interval.m_upper <= deserializer->m_timestamp) {
                // TODO this is an extremely fragile hack until the CLP ffi ir
                // code is refactored and IRErrorCode is completed
                return static_cast<int>(IRErrorCode_Unsupported_Version + 1);
            }
            if (time_interval.m_lower > deserializer->m_timestamp) {
                continue;
            }
            std::span<WildcardQueryView> const qv_span{queries, queries_size};
            auto const found_query = std::find_if(
                    qv_span.begin(),
                    qv_span.end(),
                    [&](WildcardQueryView qv) -> bool {
                        return wildcard_match_unsafe(
                                deserializer->m_log_event.m_log_message,
                                clean_up_wildcard_search_string(
                                        std::string_view{qv.m_query.m_data, qv.m_query.m_size}
                                ),
                                static_cast<bool>(qv.m_case_sensitive)
                        );
                    }
            );
            if (qv_span.end() != found_query) {
                *ir_pos = ir_buf.get_cursor_pos();
                log_event->m_log_message.m_data = deserializer->m_log_event.m_log_message.data();
                log_event->m_log_message.m_size = deserializer->m_log_event.m_log_message.size();
                log_event->m_timestamp = deserializer->m_timestamp;
                *matching_query = found_query - qv_span.begin();
                return static_cast<int>(IRErrorCode_Success);
            }
        }
    }
}  // namespace

extern "C" auto ir_deserializer_deserialize_eight_byte_wildcard_match(
        ByteView ir_view,
        void* ir_deserializer,
        TimestampInterval time_interval,
        WildcardQueryView* queries,
        size_t queries_size,
        size_t* ir_pos,
        LogEventView* log_event,
        size_t* matching_query
) -> int {
    return deserialize_wildcard_match<eight_byte_encoded_variable_t>(
            ir_view,
            ir_deserializer,
            time_interval,
            queries,
            queries_size,
            ir_pos,
            log_event,
            matching_query
    );
}

extern "C" auto ir_deserializer_deserialize_four_byte_wildcard_match(
        ByteView ir_view,
        void* ir_deserializer,
        TimestampInterval time_interval,
        WildcardQueryView* queries,
        size_t queries_size,
        size_t* ir_pos,
        LogEventView* log_event,
        size_t* matching_query
) -> int {
    return deserialize_wildcard_match<four_byte_encoded_variable_t>(
            ir_view,
            ir_deserializer,
            time_interval,
            queries,
            queries_size,
            ir_pos,
            log_event,
            matching_query
    );
}

extern "C" auto ir_deserializer_deserialize_eight_byte_log_event(
        ByteView ir_view,
        void* ir_deserializer,
        size_t* ir_pos,
        LogEventView* log_event
) -> int {
    return deserialize_log_event<eight_byte_encoded_variable_t>(
            ir_view,
            ir_deserializer,
            ir_pos,
            log_event
    );
}

extern "C" auto ir_deserializer_deserialize_four_byte_log_event(
        ByteView ir_view,
        void* ir_deserializer,
        size_t* ir_pos,
        LogEventView* log_event
) -> int {
    return deserialize_log_event<four_byte_encoded_variable_t>(
            ir_view,
            ir_deserializer,
            ir_pos,
            log_event
    );
}

extern "C" auto ir_deserializer_deserialize_preamble(
        ByteView ir_view,
        size_t* ir_pos,
        int8_t* ir_encoding,
        int8_t* metadata_type,
        size_t* metadata_pos,
        uint16_t* metadata_size,
        void** ir_deserializer_ptr
) -> int {
    IrBuffer ir_buf{static_cast<int8_t*>(ir_view.m_data), ir_view.m_size};

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

    *ir_pos = ir_buf.get_cursor_pos();
    *ir_deserializer_ptr = new Deserializer();
    return static_cast<int>(IRErrorCode_Success);
}

extern "C" auto ir_deserializer_close(void* ir_deserializer) -> void {
    // NOLINTNEXTLINE(cppcoreguidelines-owning-memory)
    delete static_cast<Deserializer*>(ir_deserializer);
}
}  // namespace ffi_go::ir
