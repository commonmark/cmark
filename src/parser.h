#ifndef CMARK_AST_H
#define CMARK_AST_H

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
  struct cmark_reference_map *refmap;
  struct cmark_node *root;
  struct cmark_node *current;
  int line_number;
  bufsize_t offset;
  bufsize_t column;
  bufsize_t first_nonspace;
  bufsize_t first_nonspace_column;
  bufsize_t thematic_break_kill_pos;
  int indent;
  bool blank;
  bool partially_consumed_tab;
  cmark_strbuf curline;
  bufsize_t last_line_length;
  cmark_strbuf linebuf;
  cmark_strbuf content;
  int options;
  bool last_buffer_ended_with_cr;
  unsigned int total_size;

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
   * The count is stored at this offset: type - CMARK_NODE_FIRST_BLOCK
   * For example, CMARK_NODE_LIST (0x3) is stored at offset 2.
   */
  size_t open_block_counts[CMARK_NODE_TYPE_BLOCK_LIMIT];
  size_t total_open_blocks;
};

static CMARK_INLINE void incr_open_block_count(cmark_parser *parser, cmark_node_type type) {
  assert(type >= CMARK_NODE_FIRST_BLOCK);
  assert(type < CMARK_NODE_FIRST_BLOCK + CMARK_NODE_TYPE_BLOCK_LIMIT);
  parser->open_block_counts[type - CMARK_NODE_FIRST_BLOCK]++;
  parser->total_open_blocks++;
}

static CMARK_INLINE void decr_open_block_count(cmark_parser *parser, cmark_node_type type) {
  assert(type >= CMARK_NODE_FIRST_BLOCK);
  assert(type < CMARK_NODE_FIRST_BLOCK + CMARK_NODE_TYPE_BLOCK_LIMIT);
  assert(parser->open_block_counts[type - CMARK_NODE_FIRST_BLOCK] > 0);
  parser->open_block_counts[type - CMARK_NODE_FIRST_BLOCK]--;
  assert(parser->total_open_blocks > 0);
  parser->total_open_blocks--;
}

static CMARK_INLINE size_t read_open_block_count(cmark_parser *parser, cmark_node_type type) {
  assert(type >= CMARK_NODE_FIRST_BLOCK);
  assert(type < CMARK_NODE_FIRST_BLOCK + CMARK_NODE_TYPE_BLOCK_LIMIT);
  return parser->open_block_counts[type - CMARK_NODE_FIRST_BLOCK];
}

#ifdef __cplusplus
}
#endif

#endif
