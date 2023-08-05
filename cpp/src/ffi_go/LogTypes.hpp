#ifndef FFI_GO_LOG_TYPES_HPP
#define FFI_GO_LOG_TYPES_HPP

#include <string>

namespace ffi_go {
/**
 * The backing storage for a Go ffi.LogEventView.
 * Mutating a LogEvent instance will invalidate any LogEventViews using it as
 * its backing (without any warning or way to guard in the Go layer).
 */
struct LogEvent {
    auto reserve(size_t cap) -> void { m_message.reserve(cap); }

    std::string m_message;
};

/**
 * The backing storage for a Go ffi.LogMessageView.
 * Mutating a LogMessage instance will invalidate any LogMessageViews using it
 * as its backing (without any warning or way to guard in the Go layer).
 */
struct LogMessage {
    auto reserve(size_t cap) -> void { m_message.reserve(cap); }

    std::string m_message;
};
}  // namespace ffi_go

#endif  // FFI_GO_LOG_TYPES_HPP
