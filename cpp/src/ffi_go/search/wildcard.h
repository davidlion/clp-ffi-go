#ifndef FFI_GO_IR_WILDCARD_MATCH_H
#define FFI_GO_IR_WILDCARD_MATCH_H

// header must support C, making modernize checks inapplicable
// NOLINTBEGIN(modernize-deprecated-headers)
// NOLINTBEGIN(modernize-use-trailing-return-type)
// NOLINTBEGIN(modernize-use-using)

#include <stdlib.h>

#include <ffi_go/defs.h>

/**
 * A view of a wildcard query passed down from Go. The query string must first
 * be cleaned using the CLP function `clean_up_wildcard_search_string`.
 * m_case_sensitive is 1 for a case sensitive query (0 for case insensitive).
 */
typedef struct {
    StringView m_query;
    int m_case_sensitive;
} WildcardQueryView;

/**
 * Given a target string perform CLP wildcard matching using query. The query is
 * first cleaned using CLP's `clean_up_wildcard_search_string`.
 * @param[in] target String to perform matching on
 * @param[in] query Query to use for matching
 * @return 1 if query matches target, 0 otherwise
 */
int wildcard_match(StringView target, WildcardQueryView query);

// NOLINTEND(modernize-use-using)
// NOLINTEND(modernize-use-trailing-return-type)
// NOLINTEND(modernize-deprecated-headers)
#endif  // FFI_GO_IR_WILDCARD_MATCH_H
