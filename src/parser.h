#ifndef CMARK_PARSER_H
#define CMARK_PARSER_H

#include <stdio.h>
#include "references.h"
#include "node.h"
#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LINK_LABEL_LENGTH 1000

struct cmark_parser {
  struct cmark_mem *mem;
  /* A hashtable of urls in the current document for cross-references */
  struct cmark_map *refmap;
  /* The root node of the parser, always a CMARK_NODE_DOCUMENT */
  struct cmark_node *root;
  /* The last open block after a line is fully processed */
  struct cmark_node *current;
  /* See the documentation for cmark_parser_get_line_number() in cmark.h */
  int line_number;
  /* See the documentation for cmark_parser_get_offset() in cmark.h */
  bufsize_t offset;
  /* See the documentation for cmark_parser_get_column() in cmark.h */
  bufsize_t column;
  /* See the documentation for cmark_parser_get_first_nonspace() in cmark.h */
  bufsize_t first_nonspace;
  /* See the documentation for cmark_parser_get_first_nonspace_column() in cmark.h */
  bufsize_t first_nonspace_column;
  bufsize_t thematic_break_kill_pos;
  /* See the documentation for cmark_parser_get_indent() in cmark.h */
  int indent;
  /* See the documentation for cmark_parser_is_blank() in cmark.h */
  bool blank;
  /* See the documentation for cmark_parser_has_partially_consumed_tab() in cmark.h */
  bool partially_consumed_tab;
  /* Contains the currently processed line */
  cmark_strbuf curline;
  /* See the documentation for cmark_parser_get_last_line_length() in cmark.h */
  bufsize_t last_line_length;
  /* FIXME: not sure about the difference with curline */
  cmark_strbuf linebuf;
  /* Options set by the user, see the Options section in cmark.h */
  int options;
  bool last_buffer_ended_with_cr;
  size_t total_size;
  cmark_llist *syntax_extensions;
  cmark_llist *inline_syntax_extensions;
  cmark_ispunct_func backslash_ispunct;

  /**
   * The "open" blocks are the blocks visited by the loop in
   * check_open_blocks (blocks.c). I.e. the blocks in this list:
   *
   *   parser->root->last_child->...->last_child
   *
   * open_block_counts is used to keep track of how many of each type of
   * node are currently in the open blocks list. Knowing these counts can
   * sometimes help to end the loop in check_open_blocks early, improving
   * efficiency.
   *
   * The count is stored at this offset: type - CMARK_NODE_TYPE_BLOCK - 1
   * For example, CMARK_NODE_LIST (0x8003) is stored at offset 2.
   */
  size_t open_block_counts[CMARK_NODE_TYPE_BLOCK_LIMIT];
  size_t total_open_blocks;
};

static CMARK_INLINE void incr_open_block_count(cmark_parser *parser, cmark_node_type type) {
  assert(type > CMARK_NODE_TYPE_BLOCK);
  assert(type <= CMARK_NODE_TYPE_BLOCK + CMARK_NODE_TYPE_BLOCK_LIMIT);
  parser->open_block_counts[type - CMARK_NODE_TYPE_BLOCK - 1]++;
  parser->total_open_blocks++;
}

static CMARK_INLINE void decr_open_block_count(cmark_parser *parser, cmark_node_type type) {
  assert(type > CMARK_NODE_TYPE_BLOCK);
  assert(type <= CMARK_NODE_TYPE_BLOCK + CMARK_NODE_TYPE_BLOCK_LIMIT);
  assert(parser->open_block_counts[type - CMARK_NODE_TYPE_BLOCK - 1] > 0);
  parser->open_block_counts[type - CMARK_NODE_TYPE_BLOCK - 1]--;
  assert(parser->total_open_blocks > 0);
  parser->total_open_blocks--;
}

static CMARK_INLINE size_t read_open_block_count(cmark_parser *parser, cmark_node_type type) {
  assert(type > CMARK_NODE_TYPE_BLOCK);
  assert(type <= CMARK_NODE_TYPE_BLOCK + CMARK_NODE_TYPE_BLOCK_LIMIT);
  return parser->open_block_counts[type - CMARK_NODE_TYPE_BLOCK - 1];
}

#ifdef __cplusplus
}
#endif

#endif
