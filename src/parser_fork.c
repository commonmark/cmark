#include <string.h>
#include "parser.h"
#include "node.h"
#include "buffer.h"
#include "chunk.h"
#include "cmark.h"
#include "references.h"
#include "map.h"
#include "syntax_extension.h"

static cmark_node *copy_node(cmark_mem *mem, cmark_node *src)
{
  if (!src)
    return NULL;

  cmark_node *dst = (cmark_node *)mem->calloc(1, sizeof(*dst));
  if (!dst)
    return NULL;

  dst->type = src->type;
  dst->flags = src->flags;
  dst->start_line = src->start_line;
  dst->start_column = src->start_column;
  dst->end_line = src->end_line;
  dst->end_column = src->end_column;
  dst->internal_offset = src->internal_offset;
  dst->backtick_count = src->backtick_count;
  // dont copy extension/ancestor_extension as they may point to original parser state
  dst->extension = src->extension; // Copy extension reference for proper node handling
  dst->ancestor_extension = NULL;
  // dont copy user_data/user_data_free_func as they may contain callbacks to original parser
  dst->user_data = NULL;
  dst->user_data_free_func = NULL;
  dst->footnote = src->footnote;
  // dont copy parent_footnote_def as it will be set during tree construction
  dst->parent_footnote_def = NULL;

  cmark_strbuf_init(mem, &dst->content, 0);
  if (src->content.ptr && src->content.size > 0)
  {
    cmark_strbuf_put(&dst->content, src->content.ptr, src->content.size);
  }

  switch (src->type)
  {
  case CMARK_NODE_CODE_BLOCK:
    dst->as.code = src->as.code;
    if (src->as.code.info.data)
    {
      dst->as.code.info = cmark_chunk_literal(cmark_chunk_to_cstr(mem, &src->as.code.info));
    }
    if (src->as.code.literal.data)
    {
      dst->as.code.literal = cmark_chunk_literal(cmark_chunk_to_cstr(mem, &src->as.code.literal));
    }
    break;

  case CMARK_NODE_HEADING:
    dst->as.heading = src->as.heading;
    break;

  case CMARK_NODE_LIST:
    dst->as.list = src->as.list;
    break;

  case CMARK_NODE_LINK:
  case CMARK_NODE_IMAGE:
    dst->as.link = src->as.link;
    if (src->as.link.url.data)
    {
      dst->as.link.url = cmark_chunk_literal(cmark_chunk_to_cstr(mem, &src->as.link.url));
    }
    if (src->as.link.title.data)
    {
      dst->as.link.title = cmark_chunk_literal(cmark_chunk_to_cstr(mem, &src->as.link.title));
    }
    break;

  case CMARK_NODE_CUSTOM_BLOCK:
  case CMARK_NODE_CUSTOM_INLINE:
    dst->as.custom = src->as.custom;
    if (src->as.custom.on_enter.data)
    {
      dst->as.custom.on_enter = cmark_chunk_literal(cmark_chunk_to_cstr(mem, &src->as.custom.on_enter));
    }
    if (src->as.custom.on_exit.data)
    {
      dst->as.custom.on_exit = cmark_chunk_literal(cmark_chunk_to_cstr(mem, &src->as.custom.on_exit));
    }
    break;

  case CMARK_NODE_HTML_BLOCK:
    dst->as.html_block_type = src->as.html_block_type;
    break;

  case CMARK_NODE_TEXT:
  case CMARK_NODE_HTML_INLINE:
  case CMARK_NODE_CODE:
  case CMARK_NODE_FOOTNOTE_REFERENCE:
  case CMARK_NODE_FOOTNOTE_DEFINITION:
    dst->as.literal = src->as.literal;
    if (src->as.literal.data)
    {
      dst->as.literal = cmark_chunk_literal(cmark_chunk_to_cstr(mem, &src->as.literal));
    }
    break;

  case CMARK_NODE_ATTRIBUTE:
    dst->as.attribute = src->as.attribute;
    if (src->as.attribute.attributes.data)
    {
      dst->as.attribute.attributes = cmark_chunk_literal(cmark_chunk_to_cstr(mem, &src->as.attribute.attributes));
    }
    break;

  default:
    dst->as = src->as;
    break;
  }

  dst->parent = NULL;
  dst->first_child = NULL;
  dst->last_child = NULL;
  dst->prev = NULL;
  dst->next = NULL;

  return dst;
}

struct node_mapping
{
  cmark_node *src;
  cmark_node *dst;
  struct node_mapping *next;
};

static int add_node_mapping(cmark_mem *mem, struct node_mapping **mappings,
                            cmark_node *src, cmark_node *dst)
{
  struct node_mapping *mapping = (struct node_mapping *)mem->calloc(1, sizeof(struct node_mapping));
  if (!mapping)
    return 0;

  mapping->src = src;
  mapping->dst = dst;
  mapping->next = *mappings;
  *mappings = mapping;
  return 1;
}

static cmark_node *find_mapped_node(struct node_mapping *mappings, cmark_node *src)
{
  struct node_mapping *current = mappings;
  while (current)
  {
    if (current->src == src)
    {
      return current->dst;
    }
    current = current->next;
  }
  return NULL;
}

static void free_node_mappings(cmark_mem *mem, struct node_mapping *mappings)
{
  while (mappings)
  {
    struct node_mapping *next = mappings->next;
    mem->free(mappings);
    mappings = next;
  }
}

static cmark_node *copy_node_tree_with_mapping(cmark_mem *mem, cmark_node *src,
                                               struct node_mapping **mappings)
{
  if (!src)
    return NULL;

  cmark_node *dst = copy_node(mem, src);
  if (!dst)
    return NULL;

  add_node_mapping(mem, mappings, src, dst);

  cmark_node *child = src->first_child;
  while (child)
  {
    cmark_node *child_copy = copy_node_tree_with_mapping(mem, child, mappings);
    if (child_copy)
    {
      cmark_node_append_child(dst, child_copy);
    }
    child = child->next;
  }

  return dst;
}

static cmark_node *copy_node_tree(cmark_mem *mem, cmark_node *src)
{
  struct node_mapping *mappings = NULL;
  cmark_node *result = copy_node_tree_with_mapping(mem, src, &mappings);
  free_node_mappings(mem, mappings);
  return result;
}

static cmark_map *copy_reference_map(cmark_mem *mem, cmark_map *src)
{
  if (!src)
    return NULL;

  cmark_map *dst = cmark_reference_map_new(mem);
  if (!dst)
    return NULL;

  dst->max_ref_size = src->max_ref_size;

  cmark_map_entry *entry = src->refs;
  while (entry)
  {
    if (entry->label)
    {
      // found at references.c
      // `cmark_reference *ref = (cmark_reference *)_ref;`
      // so we assume entry is a cmark_reference
      cmark_reference *ref = (cmark_reference *)entry;

      cmark_chunk label_chunk = cmark_chunk_literal((char *)entry->label);
      cmark_chunk url_chunk = {0};
      cmark_chunk title_chunk = {0};

      if (ref->is_attributes_reference)
      {
        cmark_chunk attributes_chunk = {0};
        if (ref->attributes.data)
        {
          attributes_chunk = cmark_chunk_literal((char *)ref->attributes.data);
        }
        cmark_reference_create_attributes(dst, &label_chunk, &attributes_chunk);
      }
      else
      {
        if (ref->url.data)
        {
          url_chunk = cmark_chunk_literal((char *)ref->url.data);
        }
        if (ref->title.data)
        {
          title_chunk = cmark_chunk_literal((char *)ref->title.data);
        }
        cmark_reference_create(dst, &label_chunk, &url_chunk, &title_chunk);
      }
    }
    entry = entry->next;
  }

  return dst;
}

static cmark_llist *copy_syntax_extensions(cmark_mem *mem, cmark_llist *src)
{
  if (!src)
    return NULL;

  cmark_llist *dst = NULL;
  cmark_llist *current = src;

  while (current)
  {
    cmark_syntax_extension *src_ext = (cmark_syntax_extension *)current->data;
    if (src_ext)
    {
      // as extensions are usually stateless and shared
      dst = cmark_llist_append(mem, dst, src_ext);
    }
    current = current->next;
  }

  return dst;
}

cmark_parser *cmark_parser_fork(cmark_parser *parser)
{
  if (!parser)
    return NULL;

  cmark_mem *mem = parser->mem;
  cmark_parser *fork = (cmark_parser *)mem->calloc(1, sizeof(*fork));
  if (!fork)
    return NULL;

  fork->mem = parser->mem;
  fork->refmap = copy_reference_map(mem, parser->refmap);

  struct node_mapping *mappings = NULL;
  fork->root = copy_node_tree_with_mapping(mem, parser->root, &mappings);

  if (!fork->root)
  {
    if (fork->refmap)
    {
      cmark_map_free(fork->refmap);
    }
    mem->free(fork);
    free_node_mappings(mem, mappings);
    return NULL;
  }

  if (parser->current)
  {
    fork->current = find_mapped_node(mappings, parser->current);
    if (!fork->current)
    {
      fork->current = fork->root;
    }
  }
  else
  {
    fork->current = fork->root;
  }

  free_node_mappings(mem, mappings);

  if (!fork->current)
  {
    fork->current = fork->root;
  }

  fork->line_number = parser->line_number;
  fork->offset = parser->offset;
  fork->column = parser->column;
  fork->first_nonspace = parser->first_nonspace;
  fork->first_nonspace_column = parser->first_nonspace_column;
  fork->thematic_break_kill_pos = parser->thematic_break_kill_pos;
  fork->indent = parser->indent;
  fork->blank = parser->blank;
  fork->partially_consumed_tab = parser->partially_consumed_tab;
  fork->last_line_length = parser->last_line_length;
  fork->options = parser->options;
  fork->last_buffer_ended_with_cr = parser->last_buffer_ended_with_cr;
  fork->total_size = parser->total_size;
  fork->backslash_ispunct = parser->backslash_ispunct;

  cmark_strbuf_init(mem, &fork->curline, 0);
  if (parser->curline.ptr && parser->curline.size > 0)
  {
    cmark_strbuf_put(&fork->curline, parser->curline.ptr, parser->curline.size);
  }

  cmark_strbuf_init(mem, &fork->linebuf, 0);
  if (parser->linebuf.ptr && parser->linebuf.size > 0)
  {
    cmark_strbuf_put(&fork->linebuf, parser->linebuf.ptr, parser->linebuf.size);
  }

  fork->syntax_extensions = copy_syntax_extensions(mem, parser->syntax_extensions);
  fork->inline_syntax_extensions = copy_syntax_extensions(mem, parser->inline_syntax_extensions);

  if (parser->skip_chars)
  {
    // parser->skip_chars = (int8_t *)parser->mem->calloc(sizeof(int8_t), 256);
    size_t table_size = 256 * sizeof(int8_t);
    fork->skip_chars = (int8_t *)mem->calloc(1, table_size);
    if (fork->skip_chars)
    {
      memcpy(fork->skip_chars, parser->skip_chars, table_size);
    }
  }
  if (parser->special_chars)
  {
    // parser->special_chars = (int8_t *)parser->mem->calloc(sizeof(int8_t), 256);
    size_t table_size = 256 * sizeof(int8_t);
    fork->special_chars = (int8_t *)mem->calloc(1, table_size);
    if (fork->special_chars)
    {
      memcpy(fork->special_chars, parser->special_chars, table_size);
    }
  }

  return fork;
}
