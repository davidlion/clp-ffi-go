#ifndef FFI_GO_IR_WILDCARD_MATCH_H
#define FFI_GO_IR_WILDCARD_MATCH_H

// header must support C, making modernize checks inapplicable
// NOLINTBEGIN(modernize-deprecated-headers)
// NOLINTBEGIN(modernize-use-trailing-return-type)
// NOLINTBEGIN(modernize-use-using)

#include <stdlib.h>

/**
 * Given a target string perform CLP wildcard matching using query. The query is
 * first cleaned using CLP's `clean_up_wildcard_search_string`.
 * @param[in] target String to perform matching on
 * @param[in] target_size Size of target
 * @param[in] query Query to use for matching
 * @param[in] query_size Size of query
 * @param[in] case_sensitive 1 for case sensitive, 0 for insensitive
 * @return 1 if query matches target, 0 otherwise
 */
int wildcard_match(
        char const* target,
        size_t target_size,
        char const* query,
        size_t query_size,
        int case_sensitive
);

// NOLINTEND(modernize-use-using)
// NOLINTEND(modernize-use-trailing-return-type)
// NOLINTEND(modernize-deprecated-headers)
#endif  // FFI_GO_IR_WILDCARD_MATCH_H
