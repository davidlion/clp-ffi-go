#ifndef FFI_GO_IR_LOG_TYPES_HPP
#define FFI_GO_IR_LOG_TYPES_HPP

#include <string>
#include <vector>

#include <clp/components/core/src/Defs.h>

#include <ffi_go/LogTypes.hpp>

namespace ffi_go::ir {

template <typename>
[[maybe_unused]] constexpr bool cAlwaysFalse{false};

/**
 * The backing storage for a Go ir.Deserializer.
 * Mutating its fields will invalidate the the Views stored by an
 * ir.Deserializer (without any warning or way to guard in the Go layer).
 */
struct Deserializer {
    ffi_go::LogEvent m_log_event;
};

/**
 * The backing storage for a Go ir.Serializer.
 * Mutating its fields will invalidate the the Views stored by an ir.Serializer
 * (without any warning or way to guard in the Go layer).
 */
struct Serializer {
    /**
     * Reserve capacity for the logtype and ir buffer.
     * We reserve 1.5x the size of the log message type as a heuristic for the
     * full IR buffer size. The log message type of a log event is not
     * guaranteed to be less than or equal to the size of the actual log
     * message, but in general this is true.
     */
    auto reserve(size_t cap) -> void {
        m_logtype.reserve(cap);
        m_ir_buf.reserve(cap + cap / 2);
    }

    std::string m_logtype;
    std::vector<int8_t> m_ir_buf;
};

template <class encoded_variable_t>
struct LogMessage {
    auto reserve(size_t cap) -> void { m_logtype.reserve(cap); }

    std::string m_logtype;
    std::vector<encoded_variable_t> m_vars;
    std::vector<char> m_dict_vars;
    std::vector<int32_t> m_dict_var_end_offsets;
};
}  // namespace ffi_go::ir
#endif  // FFI_GO_IR_LOG_TYPES_HPP
