#ifndef CMARK_AST_H
#define CMARK_AST_H

#include <stdio.h>
#include "cmark.h"
#include "node.h"
#include "buffer.h"
#include "memory.h"
#include "source_map.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LINK_LABEL_LENGTH 1000

struct cmark_parser {
  struct cmark_mem *mem;
  struct cmark_reference_map *refmap;
  struct cmark_node *root;
  struct cmark_node *current;
  cmark_err_type error_code;
  bufsize_t total_bytes;
  int line_number;
  bufsize_t offset;
  bufsize_t column;
  bufsize_t first_nonspace;
  bufsize_t first_nonspace_column;
  int indent;
  bool blank;
  bool partially_consumed_tab;
  cmark_strbuf curline;
  bufsize_t last_line_length;
  bufsize_t line_offset;
  cmark_strbuf linebuf;
  int options;
  bool last_buffer_ended_with_cr;
  cmark_source_map *source_map;
  cmark_source_extent *last_paragraph_extent;
};

#ifdef __cplusplus
}
#endif

#endif
