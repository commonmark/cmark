#ifndef CMARK_H
#define CMARK_H

#include <stdio.h>
#include <cmark_export.h>
#include <cmark_version.h>

#ifdef __cplusplus
extern "C" {
#endif

/** # NAME
 *
 * **cmark** - CommonMark parsing, manipulating, and rendering
 */

/** # DESCRIPTION
 *
 * ## Simple Interface
 */

/** Convert 'text' (assumed to be a UTF-8 encoded string with length
 * 'len') from CommonMark Markdown to HTML, returning a null-terminated,
 * UTF-8-encoded string. It is the caller's responsibility
 * to free the returned buffer. Returns NULL on error.
 */
CMARK_EXPORT
char *cmark_markdown_to_html(const char *text, size_t len, int options);

/** ## Node Structure
 */

typedef enum {
  /* Error status */
  CMARK_NODE_NONE,

  /* Block */
  CMARK_NODE_DOCUMENT,
  CMARK_NODE_BLOCK_QUOTE,
  CMARK_NODE_LIST,
  CMARK_NODE_ITEM,
  CMARK_NODE_CODE_BLOCK,
  CMARK_NODE_HTML_BLOCK,
  CMARK_NODE_CUSTOM_BLOCK,
  CMARK_NODE_PARAGRAPH,
  CMARK_NODE_HEADING,
  CMARK_NODE_THEMATIC_BREAK,
  CMARK_NODE_REFERENCE,

  CMARK_NODE_FIRST_BLOCK = CMARK_NODE_DOCUMENT,
  CMARK_NODE_LAST_BLOCK = CMARK_NODE_REFERENCE,

  /* Inline */
  CMARK_NODE_TEXT,
  CMARK_NODE_SOFTBREAK,
  CMARK_NODE_LINEBREAK,
  CMARK_NODE_CODE,
  CMARK_NODE_HTML_INLINE,
  CMARK_NODE_CUSTOM_INLINE,
  CMARK_NODE_EMPH,
  CMARK_NODE_STRONG,
  CMARK_NODE_LINK,
  CMARK_NODE_IMAGE,

  CMARK_NODE_FIRST_INLINE = CMARK_NODE_TEXT,
  CMARK_NODE_LAST_INLINE = CMARK_NODE_IMAGE,
} cmark_node_type;

typedef enum {
  CMARK_EXTENT_NONE,
  CMARK_EXTENT_OPENER,
  CMARK_EXTENT_CLOSER,
  CMARK_EXTENT_BLANK,
  CMARK_EXTENT_CONTENT,
  CMARK_EXTENT_PUNCTUATION,
  CMARK_EXTENT_LINK_DESTINATION,
  CMARK_EXTENT_LINK_TITLE,
  CMARK_EXTENT_LINK_LABEL,
  CMARK_EXTENT_REFERENCE_DESTINATION,
  CMARK_EXTENT_REFERENCE_LABEL,
  CMARK_EXTENT_REFERENCE_TITLE,
} cmark_extent_type;

/* For backwards compatibility: */
#define CMARK_NODE_HEADER CMARK_NODE_HEADING
#define CMARK_NODE_HRULE CMARK_NODE_THEMATIC_BREAK
#define CMARK_NODE_HTML CMARK_NODE_HTML_BLOCK
#define CMARK_NODE_INLINE_HTML CMARK_NODE_HTML_INLINE

typedef enum {
  CMARK_NO_LIST,
  CMARK_BULLET_LIST,
  CMARK_ORDERED_LIST
} cmark_list_type;

typedef enum {
  CMARK_NO_DELIM,
  CMARK_PERIOD_DELIM,
  CMARK_PAREN_DELIM
} cmark_delim_type;

typedef enum {
  CMARK_ERR_NONE,
  CMARK_ERR_OUT_OF_MEMORY,
  CMARK_ERR_INPUT_TOO_LARGE
} cmark_err_type;

typedef struct cmark_node cmark_node;
typedef struct cmark_parser cmark_parser;
typedef struct cmark_iter cmark_iter;
typedef struct cmark_source_extent cmark_source_extent;

/**
 * ## Custom memory allocator support
 */

/** Defines the memory allocation functions to be used by CMark
 * when parsing and allocating a document tree
 */
typedef struct cmark_mem {
  void *(*calloc)(size_t, size_t);
  void *(*realloc)(void *, size_t);
  void (*free)(void *);
} cmark_mem;

/** Convenience function for bindings.
 */
CMARK_EXPORT
void cmark_default_mem_free(void *ptr);

/**
 * ## Creating and Destroying Nodes
 */

/** Creates a new node of type 'type'.  Note that the node may have
 * other required properties, which it is the caller's responsibility
 * to assign.
 */
CMARK_EXPORT cmark_node *cmark_node_new(cmark_node_type type);

/** Same as `cmark_node_new`, but explicitly listing the memory
 * allocator used to allocate the node.  Note:  be sure to use the same
 * allocator for every node in a tree, or bad things can happen.
 */
CMARK_EXPORT cmark_node *cmark_node_new_with_mem(cmark_node_type type,
                                                 cmark_mem *mem);

/** Frees the memory allocated for a node and any children.
 */
CMARK_EXPORT void cmark_node_free(cmark_node *node);

/**
 * ## Tree Traversal
 */

/** Returns the next node in the sequence after 'node', or NULL if
 * there is none.
 */
CMARK_EXPORT cmark_node *cmark_node_next(cmark_node *node);

/** Returns the previous node in the sequence after 'node', or NULL if
 * there is none.
 */
CMARK_EXPORT cmark_node *cmark_node_previous(cmark_node *node);

/** Returns the parent of 'node', or NULL if there is none.
 */
CMARK_EXPORT cmark_node *cmark_node_parent(cmark_node *node);

/** Returns the first child of 'node', or NULL if 'node' has no children.
 */
CMARK_EXPORT cmark_node *cmark_node_first_child(cmark_node *node);

/** Returns the last child of 'node', or NULL if 'node' has no children.
 */
CMARK_EXPORT cmark_node *cmark_node_last_child(cmark_node *node);

/**
 * ## Iterator
 *
 * An iterator will walk through a tree of nodes, starting from a root
 * node, returning one node at a time, together with information about
 * whether the node is being entered or exited.  The iterator will
 * first descend to a child node, if there is one.  When there is no
 * child, the iterator will go to the next sibling.  When there is no
 * next sibling, the iterator will return to the parent (but with
 * a 'cmark_event_type' of `CMARK_EVENT_EXIT`).  The iterator will
 * return `CMARK_EVENT_DONE` when it reaches the root node again.
 * One natural application is an HTML renderer, where an `ENTER` event
 * outputs an open tag and an `EXIT` event outputs a close tag.
 * An iterator might also be used to transform an AST in some systematic
 * way, for example, turning all level-3 headings into regular paragraphs.
 *
 *     void
 *     usage_example(cmark_node *root) {
 *         cmark_event_type ev_type;
 *         cmark_iter *iter = cmark_iter_new(root);
 *
 *         while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
 *             cmark_node *cur = cmark_iter_get_node(iter);
 *             // Do something with `cur` and `ev_type`
 *         }
 *
 *         cmark_iter_free(iter);
 *     }
 *
 * Iterators will never return `EXIT` events for leaf nodes, which are nodes
 * of type:
 *
 * * CMARK_NODE_HTML_BLOCK
 * * CMARK_NODE_THEMATIC_BREAK
 * * CMARK_NODE_CODE_BLOCK
 * * CMARK_NODE_TEXT
 * * CMARK_NODE_SOFTBREAK
 * * CMARK_NODE_LINEBREAK
 * * CMARK_NODE_CODE
 * * CMARK_NODE_HTML_INLINE
 *
 * Nodes must only be modified after an `EXIT` event, or an `ENTER` event for
 * leaf nodes.
 */

typedef enum {
  CMARK_EVENT_NONE,
  CMARK_EVENT_DONE,
  CMARK_EVENT_ENTER,
  CMARK_EVENT_EXIT
} cmark_event_type;

/** Creates a new iterator starting at 'root'.  The current node and event
 * type are undefined until 'cmark_iter_next' is called for the first time.
 * The memory allocated for the iterator should be released using
 * 'cmark_iter_free' when it is no longer needed.
 */
CMARK_EXPORT
cmark_iter *cmark_iter_new(cmark_node *root);

/** Frees the memory allocated for an iterator.
 */
CMARK_EXPORT
void cmark_iter_free(cmark_iter *iter);

/** Advances to the next node and returns the event type (`CMARK_EVENT_ENTER`,
 * `CMARK_EVENT_EXIT` or `CMARK_EVENT_DONE`).
 */
CMARK_EXPORT
cmark_event_type cmark_iter_next(cmark_iter *iter);

/** Returns the current node.
 */
CMARK_EXPORT
cmark_node *cmark_iter_get_node(cmark_iter *iter);

/** Returns the current event type.
 */
CMARK_EXPORT
cmark_event_type cmark_iter_get_event_type(cmark_iter *iter);

/** Returns the root node.
 */
CMARK_EXPORT
cmark_node *cmark_iter_get_root(cmark_iter *iter);

/** Resets the iterator so that the current node is 'current' and
 * the event type is 'event_type'.  The new current node must be a
 * descendant of the root node or the root node itself.
 */
CMARK_EXPORT
void cmark_iter_reset(cmark_iter *iter, cmark_node *current,
                      cmark_event_type event_type);

/**
 * ## Accessors
 */

/** Returns the user data of 'node'.
 */
CMARK_EXPORT void *cmark_node_get_user_data(cmark_node *node);

/** Sets arbitrary user data for 'node'.  Returns 1 on success,
 * 0 on failure.
 */
CMARK_EXPORT int cmark_node_set_user_data(cmark_node *node, void *user_data);

/** Returns the type of 'node', or `CMARK_NODE_NONE` on error.
 */
CMARK_EXPORT cmark_node_type cmark_node_get_type(cmark_node *node);

/** Like 'cmark_node_get_type', but returns a string representation
    of the type, or `"<unknown>"`.
 */
CMARK_EXPORT
const char *cmark_node_get_type_string(cmark_node *node);

/** Returns the string contents of 'node', or an empty
    string if none is set.
 */
CMARK_EXPORT const char *cmark_node_get_literal(cmark_node *node);

/** Sets the string contents of 'node'.  Returns 1 on success,
 * 0 on failure.
 */
CMARK_EXPORT int cmark_node_set_literal(cmark_node *node, const char *content);

/** Returns the heading level of 'node', or 0 if 'node' is not a heading.
 */
CMARK_EXPORT int cmark_node_get_heading_level(cmark_node *node);

/* For backwards compatibility */
#define cmark_node_get_header_level cmark_node_get_heading_level
#define cmark_node_set_header_level cmark_node_set_heading_level

/** Sets the heading level of 'node', returning 1 on success and 0 on error.
 */
CMARK_EXPORT int cmark_node_set_heading_level(cmark_node *node, int level);

/** Returns the list type of 'node', or `CMARK_NO_LIST` if 'node'
 * is not a list.
 */
CMARK_EXPORT cmark_list_type cmark_node_get_list_type(cmark_node *node);

/** Sets the list type of 'node', returning 1 on success and 0 on error.
 */
CMARK_EXPORT int cmark_node_set_list_type(cmark_node *node,
                                          cmark_list_type type);

/** Returns the list delimiter type of 'node', or `CMARK_NO_DELIM` if 'node'
 * is not a list.
 */
CMARK_EXPORT cmark_delim_type cmark_node_get_list_delim(cmark_node *node);

/** Sets the list delimiter type of 'node', returning 1 on success and 0
 * on error.
 */
CMARK_EXPORT int cmark_node_set_list_delim(cmark_node *node,
                                           cmark_delim_type delim);

/** Returns starting number of 'node', if it is an ordered list, otherwise 0.
 */
CMARK_EXPORT int cmark_node_get_list_start(cmark_node *node);

/** Sets starting number of 'node', if it is an ordered list. Returns 1
 * on success, 0 on failure.
 */
CMARK_EXPORT int cmark_node_set_list_start(cmark_node *node, int start);

/** Returns 1 if 'node' is a tight list, 0 otherwise.
 */
CMARK_EXPORT int cmark_node_get_list_tight(cmark_node *node);

/** Sets the "tightness" of a list.  Returns 1 on success, 0 on failure.
 */
CMARK_EXPORT int cmark_node_set_list_tight(cmark_node *node, int tight);

/** Returns the info string from a fenced code block.
 */
CMARK_EXPORT const char *cmark_node_get_fence_info(cmark_node *node);

/** Sets the info string in a fenced code block, returning 1 on
 * success and 0 on failure.
 */
CMARK_EXPORT int cmark_node_set_fence_info(cmark_node *node, const char *info);

/** Returns the URL of a link, image or reference 'node', or an empty string
    if no URL is set.
 */
CMARK_EXPORT const char *cmark_node_get_url(cmark_node *node);

/** Sets the URL of a link, image or reference 'node'. Returns 1 on success,
 * 0 on failure.
 */
CMARK_EXPORT int cmark_node_set_url(cmark_node *node, const char *url);

/** Returns the title of a link, image or reference 'node', or an empty
    string if no title is set.
 */
CMARK_EXPORT const char *cmark_node_get_title(cmark_node *node);

/** Sets the title of a link, image or reference 'node'. Returns 1 on success,
 * 0 on failure.
 */
CMARK_EXPORT int cmark_node_set_title(cmark_node *node, const char *title);

/** Returns the label of a reference 'node', or an empty
    string if no label is set.
 */
CMARK_EXPORT const char *cmark_node_get_label(cmark_node *node);

/** Sets the label of a reference 'node'. Returns 1 on success,
 * 0 on failure.
 */
CMARK_EXPORT int cmark_node_set_label(cmark_node *node, const char *label);

/** Returns the literal "on enter" text for a custom 'node', or
    an empty string if no on_enter is set.
 */
CMARK_EXPORT const char *cmark_node_get_on_enter(cmark_node *node);

/** Sets the literal text to render "on enter" for a custom 'node'.
    Any children of the node will be rendered after this text.
    Returns 1 on success 0 on failure.
 */
CMARK_EXPORT int cmark_node_set_on_enter(cmark_node *node,
                                         const char *on_enter);

/** Returns the literal "on exit" text for a custom 'node', or
    an empty string if no on_exit is set.
 */
CMARK_EXPORT const char *cmark_node_get_on_exit(cmark_node *node);

/** Sets the literal text to render "on exit" for a custom 'node'.
    Any children of the node will be rendered before this text.
    Returns 1 on success 0 on failure.
 */
CMARK_EXPORT int cmark_node_set_on_exit(cmark_node *node, const char *on_exit);

/** Returns the line on which 'node' begins.
 */
CMARK_EXPORT int cmark_node_get_start_line(cmark_node *node);

/** Returns the column at which 'node' begins.
 */
CMARK_EXPORT int cmark_node_get_start_column(cmark_node *node);

/** Returns the line on which 'node' ends.
 */
CMARK_EXPORT int cmark_node_get_end_line(cmark_node *node);

/** Returns the column at which 'node' ends.
 */
CMARK_EXPORT int cmark_node_get_end_column(cmark_node *node);

/**
 * ## Tree Manipulation
 */

/** Unlinks a 'node', removing it from the tree, but not freeing its
 * memory.  (Use 'cmark_node_free' for that.)
 */
CMARK_EXPORT void cmark_node_unlink(cmark_node *node);

/** Inserts 'sibling' before 'node'.  Returns 1 on success, 0 on failure.
 */
CMARK_EXPORT int cmark_node_insert_before(cmark_node *node,
                                          cmark_node *sibling);

/** Inserts 'sibling' after 'node'. Returns 1 on success, 0 on failure.
 */
CMARK_EXPORT int cmark_node_insert_after(cmark_node *node, cmark_node *sibling);

/** Replaces 'oldnode' with 'newnode' and unlinks 'oldnode' (but does
 * not free its memory).
 * Returns 1 on success, 0 on failure.
 */
CMARK_EXPORT int cmark_node_replace(cmark_node *oldnode, cmark_node *newnode);

/** Adds 'child' to the beginning of the children of 'node'.
 * Returns 1 on success, 0 on failure.
 */
CMARK_EXPORT int cmark_node_prepend_child(cmark_node *node, cmark_node *child);

/** Adds 'child' to the end of the children of 'node'.
 * Returns 1 on success, 0 on failure.
 */
CMARK_EXPORT int cmark_node_append_child(cmark_node *node, cmark_node *child);

/** Consolidates adjacent text nodes.
 */
CMARK_EXPORT void cmark_consolidate_text_nodes(cmark_node *root);

/**
 * ## Parsing
 *
 * Simple interface:
 *
 *     cmark_node *document = cmark_parse_document("Hello *world*", 13,
 *                                                 CMARK_OPT_DEFAULT);
 *
 * Streaming interface:
 *
 *     cmark_parser *parser = cmark_parser_new(CMARK_OPT_DEFAULT);
 *     FILE *fp = fopen("myfile.md", "rb");
 *     while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
 *     	   cmark_parser_feed(parser, buffer, bytes);
 *     	   if (bytes < sizeof(buffer)) {
 *     	       break;
 *     	   }
 *     }
 *     document = cmark_parser_finish(parser);
 *     cmark_parser_free(parser);
 */

/** Creates a new parser object.
 */
CMARK_EXPORT
cmark_parser *cmark_parser_new(int options);

/** Creates a new parser object with the given memory allocator
 */
CMARK_EXPORT
cmark_parser *cmark_parser_new_with_mem(int options, cmark_mem *mem);

/** Frees memory allocated for a parser object.
 */
CMARK_EXPORT
void cmark_parser_free(cmark_parser *parser);

/** Return the error code after a failed operation.
 */
CMARK_EXPORT
cmark_err_type cmark_parser_get_error(cmark_parser *parser);

/** Return the error code after a failed operation.
 */
CMARK_EXPORT
const char *cmark_parser_get_error_message(cmark_parser *parser);

/** Feeds a string of length 'len' to 'parser'.
 */
CMARK_EXPORT
void cmark_parser_feed(cmark_parser *parser, const char *buffer, size_t len);

/** Finish parsing and return a pointer to a tree of nodes or NULL on error.
 */
CMARK_EXPORT
cmark_node *cmark_parser_finish(cmark_parser *parser);

/** Return a pointer to the first extent of the parser's source map
 */
CMARK_EXPORT
cmark_source_extent *cmark_parser_get_first_source_extent(cmark_parser *parser);

/** Parse a CommonMark document in 'buffer' of length 'len'.
 * Returns a pointer to a tree of nodes.  The memory allocated for
 * the node tree should be released using 'cmark_node_free'
 * when it is no longer needed.  Returns NULL on error.
 */
CMARK_EXPORT
cmark_node *cmark_parse_document(const char *buffer, size_t len, int options);

/** Parse a CommonMark document in file 'f', returning a pointer to
 * a tree of nodes.  The memory allocated for the node tree should be
 * released using 'cmark_node_free' when it is no longer needed.
 * Returns NULL on error.
 */
CMARK_EXPORT
cmark_node *cmark_parse_file(FILE *f, int options);

/**
 * ## Source map API
 */

/* Return the index, in bytes, of the start of this extent */
CMARK_EXPORT
size_t cmark_source_extent_get_start(cmark_source_extent *extent);

/* Return the index, in bytes, of the stop of this extent. This
 * index is not included in the extent*/
CMARK_EXPORT
size_t cmark_source_extent_get_stop(cmark_source_extent *extent);

/* Return the extent immediately following 'extent' */
CMARK_EXPORT
cmark_source_extent *cmark_source_extent_get_next(cmark_source_extent *extent);

/* Return the extent immediately preceding 'extent' */
CMARK_EXPORT
cmark_source_extent *cmark_source_extent_get_previous(cmark_source_extent *extent);

/* Return the node 'extent' maps to */
CMARK_EXPORT
cmark_node *cmark_source_extent_get_node(cmark_source_extent *extent);

/* Return the type of 'extent' */
CMARK_EXPORT
cmark_extent_type cmark_source_extent_get_type(cmark_source_extent *extent);

/* Return a string representation of 'extent' */
CMARK_EXPORT
const char *cmark_source_extent_get_type_string(cmark_source_extent *extent);

/**
 * ## Rendering
 */

/** Render a 'node' tree as XML.  It is the caller's responsibility
 * to free the returned buffer.
 */
CMARK_EXPORT
char *cmark_render_xml(cmark_node *root, int options);

/** Render a 'node' tree as an HTML fragment.  It is up to the user
 * to add an appropriate header and footer. It is the caller's
 * responsibility to free the returned buffer.
 */
CMARK_EXPORT
char *cmark_render_html(cmark_node *root, int options);

/** Render a 'node' tree as a groff man page, without the header.
 * It is the caller's responsibility to free the returned buffer.
 */
CMARK_EXPORT
char *cmark_render_man(cmark_node *root, int options, int width);

/** Render a 'node' tree as a commonmark document.
 * It is the caller's responsibility to free the returned buffer.
 */
CMARK_EXPORT
char *cmark_render_commonmark(cmark_node *root, int options, int width);

/** Render a 'node' tree as a LaTeX document.
 * It is the caller's responsibility to free the returned buffer.
 */
CMARK_EXPORT
char *cmark_render_latex(cmark_node *root, int options, int width);

/**
 * ## Options
 */

/** Default options.
 */
#define CMARK_OPT_DEFAULT 0

/**
 * ### Options affecting rendering
 */

/** Include a `data-sourcepos` attribute on all block elements.
 */
#define CMARK_OPT_SOURCEPOS (1 << 1)

/** Render `softbreak` elements as hard line breaks.
 */
#define CMARK_OPT_HARDBREAKS (1 << 2)

/** Suppress raw HTML and unsafe links (`javascript:`, `vbscript:`,
 * `file:`, and `data:`, except for `image/png`, `image/gif`,
 * `image/jpeg`, or `image/webp` mime types).  Raw HTML is replaced
 * by a placeholder HTML comment. Unsafe links are replaced by
 * empty strings.
 */
#define CMARK_OPT_SAFE (1 << 3)

/** Render `softbreak` elements as spaces.
 */
#define CMARK_OPT_NOBREAKS (1 << 4)

/**
 * ### Options affecting parsing
 */

/** Normalize tree by consolidating adjacent text nodes.
 */
#define CMARK_OPT_NORMALIZE (1 << 8)

/** Validate UTF-8 in the input before parsing, replacing illegal
 * sequences with the replacement character U+FFFD.
 */
#define CMARK_OPT_VALIDATE_UTF8 (1 << 9)

/** Convert straight quotes to curly, --- to em dashes, -- to en dashes.
 */
#define CMARK_OPT_SMART (1 << 10)

/**
 * ## Version information
 */

/** The library version as integer for runtime checks. Also available as
 * macro CMARK_VERSION for compile time checks.
 *
 * * Bits 16-23 contain the major version.
 * * Bits 8-15 contain the minor version.
 * * Bits 0-7 contain the patchlevel.
 *
 * In hexadecimal format, the number 0x010203 represents version 1.2.3.
 */
CMARK_EXPORT
int cmark_version(void);

/** The library version string for runtime checks. Also available as
 * macro CMARK_VERSION_STRING for compile time checks.
 */
CMARK_EXPORT
const char *cmark_version_string(void);

/** # AUTHORS
 *
 * John MacFarlane, Vicent Marti,  Kārlis Gaņģis, Nick Wellnhofer.
 */

#ifndef CMARK_NO_SHORT_NAMES
#define NODE_DOCUMENT CMARK_NODE_DOCUMENT
#define NODE_BLOCK_QUOTE CMARK_NODE_BLOCK_QUOTE
#define NODE_LIST CMARK_NODE_LIST
#define NODE_ITEM CMARK_NODE_ITEM
#define NODE_CODE_BLOCK CMARK_NODE_CODE_BLOCK
#define NODE_HTML_BLOCK CMARK_NODE_HTML_BLOCK
#define NODE_CUSTOM_BLOCK CMARK_NODE_CUSTOM_BLOCK
#define NODE_PARAGRAPH CMARK_NODE_PARAGRAPH
#define NODE_HEADING CMARK_NODE_HEADING
#define NODE_HEADER CMARK_NODE_HEADER
#define NODE_THEMATIC_BREAK CMARK_NODE_THEMATIC_BREAK
#define NODE_HRULE CMARK_NODE_HRULE
#define NODE_TEXT CMARK_NODE_TEXT
#define NODE_SOFTBREAK CMARK_NODE_SOFTBREAK
#define NODE_LINEBREAK CMARK_NODE_LINEBREAK
#define NODE_CODE CMARK_NODE_CODE
#define NODE_HTML_INLINE CMARK_NODE_HTML_INLINE
#define NODE_CUSTOM_INLINE CMARK_NODE_CUSTOM_INLINE
#define NODE_EMPH CMARK_NODE_EMPH
#define NODE_STRONG CMARK_NODE_STRONG
#define NODE_LINK CMARK_NODE_LINK
#define NODE_IMAGE CMARK_NODE_IMAGE
#define BULLET_LIST CMARK_BULLET_LIST
#define ORDERED_LIST CMARK_ORDERED_LIST
#define PERIOD_DELIM CMARK_PERIOD_DELIM
#define PAREN_DELIM CMARK_PAREN_DELIM
#endif

#ifdef __cplusplus
}
#endif

#endif
