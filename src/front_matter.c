#include "front_matter.h"
#include "cmark.h"

#include <string.h>

// ---------------------------------------------------------------------------
// Delimiter and info string parsing
// ---------------------------------------------------------------------------

// Return true if `input` is an opening front matter delimiter: "---" followed
// by an optional info string and a newline.  No leading whitespace before
// "---" is permitted.
//
// Note: some tools (e.g. Jekyll) also accept "..." as a closing delimiter,
// derived from the YAML document-end marker.  We intentionally do not support
// it here because this implementation is format-agnostic — the content between
// the delimiters may be YAML, TOML, JSON, or anything else.  "..." has no
// meaning outside of YAML, so "---" is the only unambiguous delimiter.
static bool is_opening_delimiter(cmark_chunk *input) {
  const unsigned char *p = input->data;
  return input->len >= 3 && p[0] == '-' && p[1] == '-' && p[2] == '-';
}

// Return true if `input` is a closing front matter delimiter: exactly "---"
// with optional trailing whitespace then a newline.  An info string is not
// permitted on the closing delimiter.
static bool is_closing_delimiter(cmark_chunk *input) {
  const unsigned char *p = input->data;
  int len = input->len;

  if (len < 3 || !(p[0] == '-' && p[1] == '-' && p[2] == '-'))
    return false;

  for (int i = 3; i < len; i++) {
    if (p[i] == '\n' || p[i] == '\r')
      return true;
    if (p[i] != ' ' && p[i] != '\t')
      return false;
  }
  return true;
}

// Extract the optional info string from an opening delimiter line, e.g.
// "--- yaml\n" yields "yaml".  Returns a zero-length chunk if absent.
static cmark_chunk parse_info(cmark_chunk *input) {
  const unsigned char *p = input->data + 3;
  int len = input->len - 3;

  while (len > 0 && (*p == ' ' || *p == '\t')) { p++; len--; }
  while (len > 0 && (p[len-1] == '\n' || p[len-1] == '\r' ||
                     p[len-1] == ' '  || p[len-1] == '\t'))
    len--;

  return (cmark_chunk){ .data = (unsigned char *)p, .len = (bufsize_t)len };
}

// ---------------------------------------------------------------------------
// Node creation
// ---------------------------------------------------------------------------

static void create_front_matter_node(cmark_parser *parser) {
  cmark_node *node =
      cmark_node_new_with_mem(CMARK_NODE_FRONT_MATTER, parser->mem);

  // Store identically to a code block: info string + literal content.
  cmark_node_set_fence_info(node,
      parser->front_matter_info.size > 0
          ? (const char *)parser->front_matter_info.ptr
          : "");

  cmark_node_set_literal(node,
      parser->front_matter_buf.size > 0
          ? (const char *)parser->front_matter_buf.ptr
          : "");

  node->start_line = 1;
  node->start_column = 1;
  node->end_line = parser->line_number;
  node->end_column = 3;

  cmark_node *first = cmark_node_first_child(parser->root);
  if (first)
    cmark_node_insert_before(first, node);
  else
    cmark_node_append_child(parser->root, node);

  parser->front_matter_scanning = false;
  cmark_strbuf_clear(&parser->front_matter_buf);
  cmark_strbuf_clear(&parser->front_matter_info);
}

// ---------------------------------------------------------------------------
// State machine — called from S_process_line in blocks.c
// ---------------------------------------------------------------------------

bool cmark_front_matter_process_line(cmark_parser *parser, cmark_chunk *input) {
  // NULL signals end-of-document: the whole document is the front matter.
  if (input == NULL) {
    create_front_matter_node(parser);
    return true;
  }

  // Adjust for any offset already consumed (e.g. a UTF-8 BOM on line 1).
  cmark_chunk adjusted = {
    .data = input->data + parser->offset,
    .len  = input->len  - parser->offset,
  };
  input = &adjusted;

  if (parser->line_number == 1) {
    if (is_opening_delimiter(input)) {
      parser->front_matter_scanning = true;
      // Capture optional info string (e.g. "yaml" from "--- yaml\n").
      cmark_chunk info = parse_info(input);
      if (info.len > 0)
        cmark_strbuf_put(&parser->front_matter_info, info.data, info.len);
    }
    return parser->front_matter_scanning;
  }

  if (!parser->front_matter_scanning)
    return false;

  if (is_closing_delimiter(input)) {
    create_front_matter_node(parser);
    return true;
  }

  // Accumulate this content line.
  cmark_strbuf_put(&parser->front_matter_buf, input->data, input->len);
  return true;
}
