#ifndef CMARK_DELIM_H
#define CMARK_DELIM_H

#include "cmark.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file cmark_delim.h
 * @brief Functions for retrieving delimiter information from cmark nodes.
 *
 * These functions allow downstream code to determine the original markdown
 * delimiter characters and lengths for inline nodes (EMPH, STRONG, CODE).
 * This enables use cases like rendering markdown with delimiters included
 * inside the styled elements, e.g., `**example**` -> `<strong>**example**</strong>`.
 *
 * The functions compute delimiter info on-demand from the original source text
 * and node position information, requiring no modifications to cmark's parsing.
 *
 * USAGE:
 * 1. Parse with cmark_parse_document_for_delimiters() - recommended
 * 2. Or parse with CMARK_OPT_SOURCEPOS and pass those options to the query functions
 *
 * The delimiter query functions require CMARK_OPT_SOURCEPOS and will return 0 if
 * this option is not provided.
 */

/**
 * Parse a CommonMark document with source position tracking enabled.
 *
 * This is a convenience wrapper around cmark_parse_document() that ensures
 * accurate source positions are available for delimiter info queries.
 * Use this function when you plan to call cmark_node_get_delimiter_info()
 * or related functions on the resulting document.
 *
 * @param buffer  The markdown source text
 * @param len     Length of buffer
 * @param options Parsing options (CMARK_OPT_SOURCEPOS is added automatically)
 * @return Root node of parsed document
 */
CMARK_EXPORT cmark_node *cmark_parse_document_for_delimiters(
    const char *buffer,
    size_t len,
    int options);

/**
 * Get delimiter info for an inline node by examining the source text.
 *
 * For EMPH nodes, returns the delimiter character ('*' or '_') and length 1.
 * For STRONG nodes, returns the delimiter character ('*' or '_') and length 2.
 * For CODE nodes, returns '`' and the number of backticks used.
 *
 * @param node       The cmark node (EMPH, STRONG, or CODE)
 * @param source     The original markdown source text passed to the parser
 * @param source_len Length of source text
 * @param options    The options used when parsing (must include CMARK_OPT_SOURCEPOS)
 * @param delim_char Output: the delimiter character ('*', '_', or '`'), may be NULL
 * @param delim_len  Output: the delimiter length (1 for emph, 2 for strong, N for code), may be NULL
 * @return 1 on success, 0 if node is NULL, source is NULL, options missing CMARK_OPT_SOURCEPOS,
 *         or node type doesn't have delimiters
 */
CMARK_EXPORT int cmark_node_get_delimiter_info(
    cmark_node *node,
    const char *source,
    size_t source_len,
    int options,
    int *delim_char,
    int *delim_len);

/**
 * Convenience function to get just the delimiter character.
 *
 * @param node       The cmark node (EMPH, STRONG, or CODE)
 * @param source     The original markdown source text passed to the parser
 * @param source_len Length of source text
 * @param options    The options used when parsing (must include CMARK_OPT_SOURCEPOS)
 * @return The delimiter char ('*', '_', or '`'), or 0 if not applicable or invalid options
 */
CMARK_EXPORT int cmark_node_get_delim_char(
    cmark_node *node,
    const char *source,
    size_t source_len,
    int options);

/**
 * Get the delimiter length for a node without needing the source text.
 *
 * For EMPH, always returns 1.
 * For STRONG, always returns 2.
 * For CODE, returns 0 (use cmark_node_get_delimiter_info to get actual backtick count).
 *
 * @param node The cmark node
 * @return The delimiter length, or 0 if not applicable or if source is needed
 */
CMARK_EXPORT int cmark_node_get_delim_length(cmark_node *node);

#ifdef __cplusplus
}
#endif

#endif /* CMARK_DELIM_H */
