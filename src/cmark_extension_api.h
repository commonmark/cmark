#ifndef CMARK_EXTENSION_API_H
#define CMARK_EXTENSION_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <cmark.h>

/** Return the index of the line currently being parsed, starting with 1.
 */
CMARK_EXPORT
int cmark_parser_get_line_number(cmark_parser *parser);

/** Return the offset in bytes in the line being processed.
 *
 * Example:
 *
 * ### foo
 *
 * Here, offset will first be 0, then 5 (the index of the 'f' character).
 */
CMARK_EXPORT
int cmark_parser_get_offset(cmark_parser *parser);

/**
 * Return the offset in 'columns' in the line being processed.
 *
 * This value may differ from the value returned by
 * cmark_parser_get_offset() in that it accounts for tabs,
 * and as such should not be used as an index in the current line's
 * buffer.
 *
 * Example:
 *
 * cmark_parser_advance_offset() can be called to advance the
 * offset by a number of columns, instead of a number of bytes.
 *
 * In that case, if offset falls "in the middle" of a tab
 * character, 'column' and offset will differ.
 *
 * ```
 * foo                 \t bar
 * ^                   ^^
 * offset (0)          20
 * ```
 *
 * If cmark_parser_advance_offset is called here with 'columns'
 * set to 'true' and 'offset' set to 22, cmark_parser_get_offset()
 * will return 20, whereas cmark_parser_get_column() will return
 * 22.
 *
 * Additionally, as tabs expand to the next multiple of 4 column,
 * cmark_parser_has_partially_consumed_tab() will now return
 * 'true'.
 */
CMARK_EXPORT
int cmark_parser_get_column(cmark_parser *parser);

/** Return the absolute index in bytes of the first nonspace
 * character coming after the offset as returned by
 * cmark_parser_get_offset() in the line currently being processed.
 *
 * Example:
 *
 * ```
 *   foo        bar            baz  \n
 * ^               ^           ^
 * 0            offset (16) first_nonspace (28)
 * ```
 */
CMARK_EXPORT
int cmark_parser_get_first_nonspace(cmark_parser *parser);

/** Return the absolute index of the first nonspace column coming after 'offset'
 * in the line currently being processed, counting tabs as multiple
 * columns as appropriate.
 *
 * See the documentation for cmark_parser_get_first_nonspace() and
 * cmark_parser_get_column() for more information.
 */
CMARK_EXPORT
int cmark_parser_get_first_nonspace_column(cmark_parser *parser);

/** Return the difference between the values returned by
 * cmark_parser_get_first_nonspace_column() and
 * cmark_parser_get_column().
 *
 * This is not a byte offset, as it can count one tab as multiple
 * characters.
 */
CMARK_EXPORT
int cmark_parser_get_indent(cmark_parser *parser);

/** Return 'true' if the line currently being processed has been entirely
 * consumed, 'false' otherwise.
 *
 * Example:
 *
 * ```
 *   foo        bar            baz  \n
 * ^
 * offset
 * ```
 *
 * This function will return 'false' here.
 *
 * ```
 *   foo        bar            baz  \n
 *                 ^
 *              offset
 * ```
 * This function will still return 'false'.
 *
 * ```
 *   foo        bar            baz  \n
 *                                ^
 *                             offset
 * ```
 *
 * At this point, this function will now return 'true'.
 */
CMARK_EXPORT
int cmark_parser_is_blank(cmark_parser *parser);

/** Return 'true' if the value returned by cmark_parser_get_offset()
 * is 'inside' an expanded tab.
 *
 * See the documentation for cmark_parser_get_column() for more
 * information.
 */
CMARK_EXPORT
int cmark_parser_has_partially_consumed_tab(cmark_parser *parser);

/** Return the length in bytes of the previously processed line, excluding potential
 * newline (\n) and carriage return (\r) trailing characters.
 */
CMARK_EXPORT
int cmark_parser_get_last_line_length(cmark_parser *parser);

#ifdef __cplusplus
}
#endif

#endif
