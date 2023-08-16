#include "wildcard.h"

#include <string_view>

#include <clp/components/core/src/string_utils.hpp>

#include <ffi_go/defs.h>

auto wildcard_match(StringView target, WildcardQueryView query) -> int {
    return static_cast<int>(wildcard_match_unsafe(
            std::string_view{target.m_data, target.m_size},
            clean_up_wildcard_search_string(
                    std::string_view{query.m_query.m_data, query.m_query.m_size}
            ),
            static_cast<bool>(query.m_case_sensitive)
    ));
}
