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

  /* Front matter scanning state (CMARK_OPT_FRONT_MATTER).
   *
   * cmark_front_matter_process_line() is called from S_process_line() in
   * blocks.c immediately after parser->line_number is incremented, so the
   * first line of the document arrives with line_number == 1.  The function
   * relies on this: it uses line_number == 1 as the trigger to decide
   * whether the document opens with a front matter block.
   *
   * front_matter_scanning is set to true when a valid opening "---" is seen
   * on line 1 and remains true until the matching closing "---" is found or
   * the document ends.  While scanning, each content line is accumulated in
   * front_matter_buf.  Both fields are freed explicitly in
   * cmark_parser_finish() and cmark_parser_free().
   */
  bool front_matter_scanning;
  cmark_strbuf front_matter_buf;  /* accumulated content lines */
  cmark_strbuf front_matter_info; /* optional format hint from opening "--- <info>" */
};

#ifdef __cplusplus
}
#endif

#endif
