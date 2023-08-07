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
            void* buf,
            size_t buf_size,
            size_t* buf_pos,
            void* ir_deserializer,
            char** log_msg_ptr,
            size_t* log_msg_size,
            epoch_time_ms_t* timestamp_or_delta
    ) -> int {
        Deserializer* deserializer{static_cast<Deserializer*>(ir_deserializer)};
        IrBuffer ir_buf{static_cast<int8_t*>(buf), buf_size};
        ir_buf.set_cursor_pos(*buf_pos);

        IRErrorCode err{};
        if constexpr (std::is_same_v<encoded_variable_t, eight_byte_encoded_variable_t>) {
            err = eight_byte_encoding::decode_next_message(
                    ir_buf,
                    deserializer->m_log_event.m_log_msg,
                    *timestamp_or_delta
            );
        } else if constexpr (std::is_same_v<encoded_variable_t, four_byte_encoded_variable_t>) {
            err = four_byte_encoding::decode_next_message(
                    ir_buf,
                    deserializer->m_log_event.m_log_msg,
                    *timestamp_or_delta
            );
        } else {
            static_assert(cAlwaysFalse<encoded_variable_t>, "Invalid/unhandled encoding type");
        }
        if (IRErrorCode_Success != err && IRErrorCode_Incomplete_IR != err) {
            return static_cast<int>(err);
        }
        if (IRErrorCode_Success == err) {
            *log_msg_ptr = deserializer->m_log_event.m_log_msg.data();
            *log_msg_size = deserializer->m_log_event.m_log_msg.size();
        }
        *buf_pos = ir_buf.get_cursor_pos();
        return static_cast<int>(err);
    }
}  // namespace

extern "C" auto ir_deserializer_deserialize_eight_byte_log_event(
        void* buf,
        size_t buf_size,
        size_t* buf_pos,
        void* ir_deserializer,
        char** log_msg_ptr,
        size_t* log_msg_size,
        epoch_time_ms_t* timestamp
) -> int {
    return deserialize_log_event<eight_byte_encoded_variable_t>(
            buf,
            buf_size,
            buf_pos,
            ir_deserializer,
            log_msg_ptr,
            log_msg_size,
            timestamp
    );
}

extern "C" auto ir_deserializer_deserialize_four_byte_log_event(
        void* buf,
        size_t buf_size,
        size_t* buf_pos,
        void* ir_deserializer,
        char** log_msg_ptr,
        size_t* log_msg,
        epoch_time_ms_t* timestamp_delta
) -> int {
    return deserialize_log_event<four_byte_encoded_variable_t>(
            buf,
            buf_size,
            buf_pos,
            ir_deserializer,
            log_msg_ptr,
            log_msg,
            timestamp_delta
    );
}

extern "C" auto ir_deserializer_deserialize_preamble(
        void* buf,
        size_t buf_size,
        size_t* buf_pos,
        int8_t* ir_encoding,
        int8_t* metadata_type,
        size_t* metadata_pos,
        uint16_t* metadata_size,
        void** ir_deserializer_ptr
) -> int {
    IrBuffer ir_buf{static_cast<int8_t*>(buf), buf_size};
    ir_buf.set_cursor_pos(*buf_pos);

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
