#ifndef CMARK_NODE_H
#define CMARK_NODE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "cmark.h"
#include "buffer.h"

typedef struct {
  int marker_offset;
  int padding;
  int start;
  unsigned char list_type;
  unsigned char delimiter;
  unsigned char bullet_char;
  bool tight;
} cmark_list;

typedef struct {
  unsigned char *info;
  uint8_t fence_length;
  uint8_t fence_offset;
  unsigned char fence_char;
  int8_t fenced;
} cmark_code;

typedef struct {
  uint8_t fence_length;
  uint8_t fence_offset;
} cmark_formula;

typedef struct {
  int internal_offset;
  int8_t level;
  bool setext;
} cmark_heading;

typedef enum {
  CMARK_TABLE_ALIGN_NONE = 0,    // No alignment specified (---)
  CMARK_TABLE_ALIGN_LEFT = 1,    // :---
  CMARK_TABLE_ALIGN_CENTER = 2,  // :---:
  CMARK_TABLE_ALIGN_RIGHT = 3    // ---:
} cmark_table_align;

typedef enum {
  CMARK_TABLE_ROW_TYPE_HEADER,
  CMARK_TABLE_ROW_TYPE_DELIMITER,
  CMARK_TABLE_ROW_TYPE_DATA
} cmark_table_row_type;

typedef struct {
  int columns_cnt;
  cmark_table_align *alignments;
} cmark_table;

typedef struct {
  cmark_table_row_type type;
} cmark_table_row;

typedef struct {
  int idx;
  bool is_delimiter;
} cmark_table_cell;

typedef struct {
  unsigned char *url;
  unsigned char *title;
  int width;
  int height;
} cmark_link;

typedef struct {
  unsigned char *on_enter;
  unsigned char *on_exit;
} cmark_custom;

enum cmark_node__internal_flags {
  CMARK_NODE__OPEN = (1 << 0),
  CMARK_NODE__LAST_LINE_BLANK = (1 << 1),
  CMARK_NODE__LAST_LINE_CHECKED = (1 << 2),
  CMARK_NODE__LIST_LAST_LINE_BLANK = (1 << 3),
};

struct cmark_node {
  cmark_mem *mem;

  struct cmark_node *next;
  struct cmark_node *prev;
  struct cmark_node *parent;
  struct cmark_node *first_child;
  struct cmark_node *last_child;

  void *user_data;

  unsigned char *data;
  bufsize_t len;

  int start_line;
  int start_column;
  int end_line;
  int end_column;
  uint16_t type;
  uint16_t flags;

  union {
    cmark_list list;
    cmark_code code;
    cmark_heading heading;
    cmark_link link;
    cmark_custom custom;
    cmark_formula formula;
    cmark_table table;
    cmark_table_row table_row;
    cmark_table_cell table_cell;
    int html_block_type;
  } as;
};

CMARK_EXPORT int cmark_node_check(cmark_node *node, FILE *out);

#ifdef __cplusplus
}
#endif

#endif
