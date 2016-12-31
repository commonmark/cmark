#include <assert.h>

#include "source_map.h"

cmark_source_map *
source_map_new(cmark_mem *mem)
{
  cmark_source_map *res = (cmark_source_map *) mem->calloc(1, sizeof(cmark_source_map));
  res->mem = mem;
  return res;
}

void
source_map_free(cmark_source_map *self)
{
  cmark_source_extent *tmp;
  for (tmp = self->head; tmp; tmp = source_map_free_extent(self, tmp));
  self->mem->free(self);
}

cmark_source_extent *
source_map_append_extent(cmark_source_map *self, bufsize_t start, bufsize_t stop, cmark_node *node, cmark_extent_type type)
{
  assert (start <= stop);
  assert (!self->tail || self->tail->stop <= start);

  cmark_source_extent *res = (cmark_source_extent *) self->mem->calloc(1, sizeof(cmark_source_extent));

  res->start = start;
  res->stop = stop;
  res->node = node;
  res->type = type;

  res->next = NULL;
  res->prev = self->tail;

  if (!self->head)
    self->head = res;
  else
    self->tail->next = res;

  self->tail = res;

  return res;
}

cmark_source_extent *
source_map_insert_extent(cmark_source_map *self, cmark_source_extent *previous,
                         bufsize_t start, bufsize_t stop, cmark_node *node, cmark_extent_type type)
{
  if (start == stop)
    return previous;

  cmark_source_extent *extent = (cmark_source_extent *) self->mem->calloc(1, sizeof(cmark_source_extent));

  extent->start = start;
  extent->stop = stop;
  extent->node = node;
  extent->type = type;
  extent->next = previous->next;
  extent->prev = previous;
  previous->next = extent;

  if (extent->next)
    extent->next->prev = extent;
  else
    self->tail = extent;

  return extent;
}

cmark_source_extent *
source_map_free_extent(cmark_source_map *self, cmark_source_extent *extent)
{
  cmark_source_extent *next = extent->next;

  if (extent->prev)
    extent->prev->next = next;

  if (extent->next)
    extent->next->prev = extent->prev;

  if (extent == self->tail)
    self->tail = extent->prev;

  if (extent == self->head)
    self->head = extent->next;

  if (extent == self->cursor) {
    self->cursor = extent->prev;
  }

  if (extent == self->next_cursor) {
    self->next_cursor = extent->next;
  }

  self->mem->free(extent);

  return next;
}

cmark_source_extent *
source_map_stitch_extent(cmark_source_map *self, cmark_source_extent *extent,
                         cmark_node *node, bufsize_t total_length)
{
  cmark_source_extent *next_extent = extent->next;
  cmark_source_extent *res;

  while (next_extent && extent->start == extent->stop) {
    extent = source_map_free_extent(self, extent);
    extent = next_extent;
    next_extent = extent->next;
  }

  if (next_extent) {
    res = source_map_insert_extent(self,
                                   extent,
                                   extent->stop,
                                   extent->next->start,
                                   node,
                                   CMARK_EXTENT_BLANK);
  } else {
    res = source_map_insert_extent(self,
                                   extent,
                                   extent->stop,
                                   total_length,
                                   node,
                                   CMARK_EXTENT_BLANK);
  }

  if (extent->start == extent->stop)
    source_map_free_extent(self, extent);

  return res;
}

cmark_source_extent *
source_map_splice_extent(cmark_source_map *self, bufsize_t start, bufsize_t stop,
                         cmark_node *node, cmark_extent_type type)
{
  if (!self->next_cursor) {
    self->cursor = source_map_insert_extent(self,
                                            self->cursor,
                                            start + self->cursor_offset,
                                            stop + self->cursor_offset, node, type);

    return self->cursor;
  } else if (start + self->cursor_offset < self->next_cursor->start &&
             stop + self->cursor_offset <= self->next_cursor->start) {
    self->cursor = source_map_insert_extent(self,
                                            self->cursor,
                                            start + self->cursor_offset,
                                            stop + self->cursor_offset, node, type);

    return self->cursor;
  } else if (start + self->cursor_offset < self->next_cursor->start) {
    bufsize_t new_start = self->next_cursor->start - self->cursor_offset;

    self->cursor = source_map_insert_extent(self,
                                            self->cursor,
                                            start + self->cursor_offset,
                                            self->next_cursor->start,
                                            node, type);

    if (new_start == stop)
      return self->cursor;

    start = new_start;
  }

  while (self->next_cursor && start + self->cursor_offset >= self->next_cursor->start) {
    self->cursor_offset += self->next_cursor->stop - self->next_cursor->start;
    self->cursor = self->cursor->next;
    self->next_cursor = self->cursor->next;
  }

  return source_map_splice_extent(self, start, stop, node, type);
}

bool
source_map_start_cursor(cmark_source_map *self, cmark_source_extent *cursor)
{
  self->cursor = cursor ? cursor : self->head;

  if (!self->cursor)
    return false;

  self->next_cursor = self->cursor->next;
  self->cursor_offset = self->cursor->stop;

  return true;
}

void
source_map_pretty_print(cmark_source_map *self) {
  cmark_source_extent *tmp;

  for (tmp = self->head; tmp; tmp = tmp->next) {
    printf ("%d:%d - %s, %s (%p)\n", tmp->start, tmp->stop,
						cmark_node_get_type_string(tmp->node),
            cmark_source_extent_get_type_string(tmp),
            (void *) tmp->node);
  }
}

bool
source_map_check(cmark_source_map *self, bufsize_t total_length)
{
  bufsize_t last_stop = 0;
  cmark_source_extent *tmp;

  for (tmp = self->head; tmp; tmp = tmp->next) {
    if (tmp->start != last_stop) {
      return false;
    } if (tmp->start == tmp->stop)
      return false;
    last_stop = tmp->stop;
  }

  if (last_stop != total_length)
    return false;

  return true;
}


size_t
cmark_source_extent_get_start(cmark_source_extent *extent)
{
  return extent->start;
}

size_t
cmark_source_extent_get_stop(cmark_source_extent *extent)
{
  return extent->stop;
}

cmark_node *
cmark_source_extent_get_node(cmark_source_extent *extent)
{
  return extent->node;
}

cmark_source_extent *
cmark_source_extent_get_next(cmark_source_extent *extent)
{
  return extent->next;
}

cmark_source_extent *
cmark_source_extent_get_previous(cmark_source_extent *extent)
{
  return extent->prev;
}

cmark_extent_type
cmark_source_extent_get_type(cmark_source_extent *extent)
{
  return extent->type;
}

const char *
cmark_source_extent_get_type_string(cmark_source_extent *extent)
{
  switch (extent->type) {
    case CMARK_EXTENT_NONE:
      return "unknown";
    case CMARK_EXTENT_OPENER:
      return "opener";
    case CMARK_EXTENT_CLOSER:
      return "closer";
    case CMARK_EXTENT_BLANK:
      return "blank";
    case CMARK_EXTENT_CONTENT:
      return "content";
    case CMARK_EXTENT_PUNCTUATION:
      return "punctuation";
    case CMARK_EXTENT_LINK_DESTINATION:
      return "link_destination";
    case CMARK_EXTENT_LINK_TITLE:
      return "link_title";
    case CMARK_EXTENT_LINK_LABEL:
      return "link_label";
    case CMARK_EXTENT_REFERENCE_DESTINATION:
      return "reference_destination";
    case CMARK_EXTENT_REFERENCE_LABEL:
      return "reference_label";
    case CMARK_EXTENT_REFERENCE_TITLE:
      return "reference_title";
  }
  return "unknown";
}
