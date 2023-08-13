#include "wildcard_match.h"

#include <string_view>

#include <clp/components/core/src/string_utils.hpp>

auto wildcard_match(
        char const* target,
        size_t target_size,
        char const* query,
        size_t query_size,
        int case_sensitive
) -> int {
    std::string_view const target_view{target, target_size};
    std::string_view const query_view{query, query_size};
    return static_cast<int>(wildcard_match_unsafe(
            target_view,
            clean_up_wildcard_search_string(query_view),
            static_cast<bool>(case_sensitive)
    ));
}
