#include "cmark_delim.h"
#include "cmark.h"
#include <stddef.h>

cmark_node *cmark_parse_document_for_delimiters(
    const char *buffer,
    size_t len,
    int options)
{
    return cmark_parse_document(buffer, len, options | CMARK_OPT_SOURCEPOS);
}

/**
 * Helper: Convert 1-indexed line and column to 0-indexed position in source.
 *
 * Handles all line ending styles: LF (\n), CR (\r), and CRLF (\r\n).
 *
 * @param line   1-indexed line number
 * @param col    1-indexed column number
 * @param source The source text
 * @param len    Length of source text
 * @return 0-indexed position in source, or len if out of bounds
 */
static size_t col_to_idx(int line, int col, const char *source, size_t len) {
    size_t idx = 0;
    int current_line = 1;

    // Skip to the correct line, handling \n, \r, and \r\n
    while (idx < len && current_line < line) {
        char c = source[idx];
        if (c == '\n') {
            current_line++;
            idx++;
        } else if (c == '\r') {
            current_line++;
            idx++;
            // Skip \n if this is \r\n (CRLF)
            if (idx < len && source[idx] == '\n') {
                idx++;
            }
        } else {
            idx++;
        }
    }

    // Add column offset (col is 1-indexed, so subtract 1)
    if (col > 0) {
        idx += (size_t)(col - 1);
    }

    return (idx < len) ? idx : len;
}

int cmark_node_get_delimiter_info(
    cmark_node *node,
    const char *source,
    size_t source_len,
    int options,
    int *delim_char,
    int *delim_len)
{
    if (node == NULL || source == NULL) {
        return 0;
    }

    // Require CMARK_OPT_SOURCEPOS for accurate position tracking
    if (!(options & CMARK_OPT_SOURCEPOS)) {
        return 0;
    }

    cmark_node_type type = cmark_node_get_type(node);
    int start_col = cmark_node_get_start_column(node);
    int start_line = cmark_node_get_start_line(node);

    switch (type) {
    case CMARK_NODE_EMPH:
        // Emph: start_column points TO the opening delimiter
        if (delim_len) {
            *delim_len = 1;
        }
        if (delim_char) {
            if (start_col >= 1) {
                size_t idx = col_to_idx(start_line, start_col, source, source_len);
                *delim_char = (idx < source_len) ? (unsigned char)source[idx] : 0;
            } else {
                *delim_char = 0;
            }
        }
        return 1;

    case CMARK_NODE_STRONG:
        // Strong: start_column points TO the first opening delimiter
        if (delim_len) {
            *delim_len = 2;
        }
        if (delim_char) {
            if (start_col >= 1) {
                size_t idx = col_to_idx(start_line, start_col, source, source_len);
                *delim_char = (idx < source_len) ? (unsigned char)source[idx] : 0;
            } else {
                *delim_char = 0;
            }
        }
        return 1;

    case CMARK_NODE_CODE:
        // Code uses backticks
        if (delim_char) {
            *delim_char = '`';
        }
        if (delim_len) {
            if (start_col > 1) {
                // Count backticks backwards from content start
                size_t idx = col_to_idx(start_line, start_col - 1, source, source_len);
                int count = 0;

                // Count consecutive backticks going backwards
                while (idx < source_len && source[idx] == '`') {
                    count++;
                    if (idx == 0) {
                        break;
                    }
                    idx--;
                }

                *delim_len = (count > 0) ? count : 1;
            } else {
                *delim_len = 1;
            }
        }
        return 1;

    default:
        return 0;
    }
}

int cmark_node_get_delim_char(
    cmark_node *node,
    const char *source,
    size_t source_len,
    int options)
{
    int delim_char = 0;
    cmark_node_get_delimiter_info(node, source, source_len, options, &delim_char, NULL);
    return delim_char;
}

int cmark_node_get_delim_length(cmark_node *node)
{
    if (node == NULL) {
        return 0;
    }

    switch (cmark_node_get_type(node)) {
    case CMARK_NODE_EMPH:
        return 1;
    case CMARK_NODE_STRONG:
        return 2;
    case CMARK_NODE_CODE:
        // For code, we need the source to count backticks
        // Return 0 to indicate caller should use the full function
        return 0;
    default:
        return 0;
    }
}
