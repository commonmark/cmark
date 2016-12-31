#ifndef CMARK_SOURCE_MAP_H
#define CMARK_SOURCE_MAP_H

#include "cmark.h"
#include "config.h"
#include "buffer.h"

typedef struct _cmark_source_map
{
  cmark_source_extent *head;
  cmark_source_extent *tail;
  cmark_source_extent *cursor;
  cmark_source_extent *next_cursor;
  bufsize_t cursor_offset;
  cmark_mem *mem;
} cmark_source_map;

struct cmark_source_extent
{
  bufsize_t start;
  bufsize_t stop;
  struct cmark_source_extent *next;
  struct cmark_source_extent *prev;
  cmark_node *node;
  cmark_extent_type type;
};

cmark_source_map    * source_map_new          (cmark_mem *mem);

void                  source_map_free         (cmark_source_map *self);

bool                  source_map_check        (cmark_source_map *self,
                                               bufsize_t total_length);

void                  source_map_pretty_print (cmark_source_map *self);

cmark_source_extent * source_map_append_extent(cmark_source_map *self,
                                               bufsize_t start,
                                               bufsize_t stop,
                                               cmark_node *node,
                                               cmark_extent_type type);

cmark_source_extent * source_map_insert_extent(cmark_source_map *self,
                                               cmark_source_extent *previous,
                                               bufsize_t start,
                                               bufsize_t stop,
                                               cmark_node *node,
                                               cmark_extent_type type);

cmark_source_extent * source_map_free_extent  (cmark_source_map *self,
                                               cmark_source_extent *extent);

cmark_source_extent * source_map_stitch_extent(cmark_source_map *self,
                                               cmark_source_extent *extent,
                                               cmark_node *node,
                                               bufsize_t total_length);

cmark_source_extent * source_map_splice_extent(cmark_source_map *self,
                                               bufsize_t start,
                                               bufsize_t stop,
                                               cmark_node *node,
                                               cmark_extent_type type);

bool                  source_map_start_cursor (cmark_source_map *self,
                                               cmark_source_extent *cursor);

#endif
