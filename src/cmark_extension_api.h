#ifndef CMARK_EXTENSION_API_H
#define CMARK_EXTENSION_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <cmark.h>

/**
 * ## Extension Support
 *
 * While the "core" of libcmark is strictly compliant with the
 * specification, an API is provided for extension writers to
 * hook into the parsing process.
 *
 * It should be noted that the cmark_node API already offers
 * room for customization, with methods offered to traverse and
 * modify the AST, and even define custom blocks.
 * When the desired customization is achievable in an error-proof
 * way using that API, it should be the preferred method.
 *
 * The following API requires a more in-depth understanding
 * of libcmark's parsing strategy, which is exposed
 * [here](http://spec.commonmark.org/0.24/#appendix-a-parsing-strategy).
 *
 * It should be used when "a posteriori" modification of the AST
 * proves to be too difficult / impossible to implement correctly.
 *
 * It can also serve as an intermediary step before extending
 * the specification, as an extension implemented using this API
 * will be trivially integrated in the core if it proves to be
 * desirable.
 */

typedef struct cmark_plugin cmark_plugin;

/** A syntax extension that can be attached to a cmark_parser
 * with cmark_parser_attach_syntax_extension().
 *
 * Extension writers should assign functions matching
 * the signature of the following 'virtual methods' to
 * implement new functionality.
 *
 * Their calling order and expected behaviour match the procedure outlined
 * at <http://spec.commonmark.org/0.24/#phase-1-block-structure>:
 *
 * During step 1, cmark will call the function provided through
 * 'cmark_syntax_extension_set_match_block_func' when it
 * iterates over an open block created by this extension,
 * to determine  whether it could contain the new line.
 * If no function was provided, cmark will close the block.
 *
 * During step 2, if and only if the new line doesn't match any
 * of the standard syntax rules, cmark will call the function
 * provided through 'cmark_syntax_extension_set_open_block_func'
 * to let the extension determine whether that new line matches
 * one of its syntax rules.
 * It is the responsibility of the parser to create and add the
 * new block with cmark_parser_make_block and cmark_parser_add_child.
 * If no function was provided is NULL, the extension will have
 * no effect at all on the final block structure of the AST.
 */
typedef struct cmark_syntax_extension cmark_syntax_extension;

/**
 * ### Plugin API.
 *
 * Extensions should be distributed as dynamic libraries,
 * with a single exported function named after the distributed
 * filename.
 *
 * When discovering extensions (see cmark_init), cmark will
 * try to load a symbol named "init_{{filename}}" in all the
 * dynamic libraries it encounters.
 *
 * For example, given a dynamic library named myextension.so
 * (or myextension.dll), cmark will try to load the symbol
 * named "init_myextension". This means that the filename
 * must lend itself to forming a valid C identifier, with
 * the notable exception of dashes, which will be translated
 * to underscores, which means cmark will look for a function
 * named "init_my_extension" if it encounters a dynamic library
 * named "my-extension.so".
 *
 * See the 'cmark_plugin_init_func' typedef for the exact prototype
 * this function should follow.
 *
 * For now the extensibility of cmark is not complete, as
 * it only offers API to hook into the block parsing phase
 * (<http://spec.commonmark.org/0.24/#phase-1-block-structure>).
 *
 * See 'cmark_plugin_register_syntax_extension' for more information.
 */

/** The prototype plugins' init function should follow.
 */
typedef int (*cmark_plugin_init_func)(cmark_plugin *plugin);

/** Register a syntax 'extension' with the 'plugin', it will be made
 * available as an extension and, if attached to a cmark_parser
 * with 'cmark_parser_attach_syntax_extension', it will contribute
 * to the block parsing process.
 *
 * See the documentation for 'cmark_syntax_extension' for information
 * on how to implement one.
 *
 * This function will typically be called from the init function
 * of external modules.
 *
 * This takes ownership of 'extension', one should not call
 * 'cmark_syntax_extension_free' on a registered extension.
 */
CMARK_EXPORT
int cmark_plugin_register_syntax_extension(cmark_plugin *plugin,
                                            cmark_syntax_extension *extension);

/** This will search for the syntax extension named 'name' among the
 *  registered syntax extensions.
 *
 *  It can then be attached to a cmark_parser
 *  with the cmark_parser_attach_syntax_extension method.
 */
CMARK_EXPORT
cmark_syntax_extension *cmark_find_syntax_extension(const char *name);

/** Should create and add a new open block to 'parent_container' if
 * 'input' matches a syntax rule for that block type. It is allowed
 * to modify the type of 'parent_container'.
 *
 * Should return the newly created block if there is one, or
 * 'parent_container' if its type was modified, or NULL.
 */
typedef cmark_node * (*cmark_open_block_func) (cmark_syntax_extension *extension,
                                       int indented,
                                       cmark_parser *parser,
                                       cmark_node *parent_container,
                                       unsigned char *input,
                                       int len);

/** Should return 'true' if 'input' can be contained in 'container',
 *  'false' otherwise.
 */
typedef int (*cmark_match_block_func)        (cmark_syntax_extension *extension,
                                       cmark_parser *parser,
                                       unsigned char *input,
                                       int len,
                                       cmark_node *container);

/** Free a cmark_syntax_extension.
 */
CMARK_EXPORT
void cmark_syntax_extension_free               (cmark_syntax_extension *extension);

/** Return a newly-constructed cmark_syntax_extension, named 'name'.
 */
CMARK_EXPORT
cmark_syntax_extension *cmark_syntax_extension_new (const char *name);

/** See the documentation for 'cmark_syntax_extension'
 */
CMARK_EXPORT
void cmark_syntax_extension_set_open_block_func(cmark_syntax_extension *extension,
                                                cmark_open_block_func func);

/** See the documentation for 'cmark_syntax_extension'
 */
CMARK_EXPORT
void cmark_syntax_extension_set_match_block_func(cmark_syntax_extension *extension,
                                                 cmark_match_block_func func);

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

/** Add a child to 'parent' during the parsing process.
 *
 * If 'parent' isn't the kind of node that can accept this child,
 * this function will back up till it hits a node that can, closing
 * blocks as appropriate.
 */
CMARK_EXPORT
cmark_node*cmark_parser_add_child(cmark_parser *parser,
                                  cmark_node *parent,
                                  cmark_node_type block_type,
                                  int start_column);

/** Advance the 'offset' of the parser in the current line.
 *
 * See the documentation of cmark_parser_get_offset() and
 * cmark_parser_get_column() for more information.
 */
CMARK_EXPORT
void cmark_parser_advance_offset(cmark_parser *parser,
                                 const char *input,
                                 int count,
                                 int columns);

/** Attach the syntax 'extension' to the 'parser', to provide extra syntax
 *  rules.
 *  See the documentation for cmark_syntax_extension for more information.
 *
 *  Returns 'true' if the 'extension' was successfully attached,
 *  'false' otherwise.
 */
CMARK_EXPORT
int cmark_parser_attach_syntax_extension(cmark_parser *parser, cmark_syntax_extension *extension);

/** Change the type of 'node'.
 *
 * Return 0 if the type could be changed, 1 otherwise.
 */
CMARK_EXPORT int cmark_node_set_type(cmark_node *node, cmark_node_type type);

/** Return the string content for all types of 'node'.
 *  The pointer stays valid as long as 'node' isn't freed.
 */
CMARK_EXPORT const char *cmark_node_get_string_content(cmark_node *node);

/** Set the string 'content' for all types of 'node'.
 *  Copies 'content'.
 */
CMARK_EXPORT int cmark_node_set_string_content(cmark_node *node, const char *content);

/** Get the syntax extension responsible for the creation of 'node'.
 *  Return NULL if 'node' was created because it matched standard syntax rules.
 */
CMARK_EXPORT cmark_syntax_extension *cmark_node_get_syntax_extension(cmark_node *node);

/** Set the syntax extension responsible for creating 'node'.
 */
CMARK_EXPORT int cmark_node_set_syntax_extension(cmark_node *node,
                                                  cmark_syntax_extension *extension);

#ifdef __cplusplus
}
#endif

#endif
