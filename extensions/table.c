#include <cmark_extension_api.h>
#include <html.h>
#include <inlines.h>
#include <parser.h>
#include <references.h>
#include <string.h>

#include "ext_scanners.h"
#include "strikethrough.h"
#include "table.h"

static cmark_node_type CMARK_NODE_TABLE, CMARK_NODE_TABLE_ROW,
    CMARK_NODE_TABLE_CELL;

typedef struct {
  uint16_t n_columns;
  cmark_llist *cells;
} table_row;

typedef struct {
  uint16_t n_columns;
  uint8_t *alignments;
} node_table;

typedef struct {
  bool is_header;
  unsigned char *raw_content;
  size_t raw_content_len;
} node_table_row;

static void free_table_cell(cmark_mem *mem, void *data) {
  cmark_node_free((cmark_node *)data);
}

static void free_table_row(cmark_mem *mem, table_row *row) {
  if (!row)
    return;

  cmark_llist_free_full(mem, row->cells, (cmark_free_func)free_table_cell);

  mem->free(row);
}

static void free_node_table(cmark_mem *mem, void *ptr) {
  node_table *t = (node_table *)ptr;
  mem->free(t->alignments);
  mem->free(t);
}

static void free_node_table_row(cmark_mem *mem, void *ptr) {
  node_table_row *ntr = (node_table_row *)ptr;
  mem->free(ntr->raw_content);
  mem->free(ntr);
}

static int get_n_table_columns(cmark_node *node) {
  if (!node || node->type != CMARK_NODE_TABLE)
    return -1;

  return (int)((node_table *)node->as.opaque)->n_columns;
}

static int set_n_table_columns(cmark_node *node, uint16_t n_columns) {
  if (!node || node->type != CMARK_NODE_TABLE)
    return 0;

  ((node_table *)node->as.opaque)->n_columns = n_columns;
  return 1;
}

static uint8_t *get_table_alignments(cmark_node *node) {
  if (!node || node->type != CMARK_NODE_TABLE)
    return 0;

  return ((node_table *)node->as.opaque)->alignments;
}

static int set_table_alignments(cmark_node *node, uint8_t *alignments) {
  if (!node || node->type != CMARK_NODE_TABLE)
    return 0;

  ((node_table *)node->as.opaque)->alignments = alignments;
  return 1;
}

static void maybe_consume_pipe(cmark_node **n, int *offset) {
  if (*n && (*n)->type == CMARK_NODE_TEXT && *offset < (*n)->as.literal.len &&
      (*n)->as.literal.data[*offset] == '|')
    ++*offset;
}

static int find_unescaped_pipe(const cmark_chunk *chunk, int offset) {
  bool escaping = false;
  for (; offset < chunk->len; ++offset) {
    if (escaping)
      escaping = false;
    else if (chunk->data[offset] == '\\')
      escaping = true;
    else if (chunk->data[offset] == '|')
      return offset;
  }
  return -1;
}

static cmark_node *consume_until_pipe_or_eol(cmark_syntax_extension *self,
                                             cmark_parser *parser,
                                             cmark_node **n, int *offset) {
  cmark_node *result =
      cmark_node_new_with_mem(CMARK_NODE_TABLE_CELL, parser->mem);
  cmark_node_set_syntax_extension(result, self);
  bool was_escape = false;

  while (*n) {
    cmark_node *node = *n;

    if (node->type == CMARK_NODE_TEXT) {
      cmark_node *child = cmark_parser_add_child(
          parser, result, CMARK_NODE_TEXT, cmark_parser_get_offset(parser));

      if (was_escape) {
        child->as.literal = cmark_chunk_dup(&node->as.literal, *offset, 1);
        cmark_node_own(child);
        if (child->as.literal.data[0] == '|')
          cmark_node_free(child->prev);
        ++*offset;
        if (*offset >= node->as.literal.len) {
          *offset = 0;
          *n = node->next;
        }
        was_escape = false;
        continue;
      }

      const char *lit = (char *)node->as.literal.data + *offset;
      const int lit_len = node->as.literal.len - *offset;

      if (lit_len == 1 && lit[0] == '\\' &&
          node->next &&
          node->next->type == CMARK_NODE_TEXT) {
        child->as.literal = cmark_chunk_dup(&node->as.literal, *offset, 1);
        cmark_node_own(child);
        was_escape = true;
        *n = node->next;
        continue;
      }

      int pipe = find_unescaped_pipe(&node->as.literal, *offset);
      if (pipe == -1) {
        child->as.literal = cmark_chunk_dup(&node->as.literal, *offset,
                                            node->as.literal.len - *offset);
        cmark_node_own(child);
      } else {
        pipe -= *offset;

        child->as.literal = cmark_chunk_dup(&node->as.literal, *offset, pipe);
        cmark_node_own(child);

        *offset += pipe + 1;
        if (*offset >= node->as.literal.len) {
          *offset = 0;
          *n = node->next;
        }
        break;
      }

      *n = node->next;
      *offset = 0;
    } else {
      cmark_node *next = node->next;
      cmark_node_append_child(result, node);
      cmark_node_own(node);
      *n = next;
      *offset = 0;
    }
  }

  if (!result->first_child) {
    cmark_node_free(result);
    return NULL;
  }

  cmark_consolidate_text_nodes(result);

  if (result->first_child->type == CMARK_NODE_TEXT) {
    cmark_chunk c = cmark_chunk_ltrim_new(parser->mem, &result->first_child->as.literal);
    cmark_chunk_free(parser->mem, &result->first_child->as.literal);
    result->first_child->as.literal = c;
  }

  if (result->last_child->type == CMARK_NODE_TEXT) {
    cmark_chunk c = cmark_chunk_rtrim_new(parser->mem, &result->last_child->as.literal);
    cmark_chunk_free(parser->mem, &result->last_child->as.literal);
    result->last_child->as.literal = c;
  }

  return result;
}

static int table_ispunct(char c) {
  return cmark_ispunct(c) && c != '|';
}

static table_row *row_from_string(cmark_syntax_extension *self,
                                  cmark_parser *parser, unsigned char *string,
                                  int len) {
  table_row *row = NULL;

  cmark_node *temp_container =
      cmark_node_new_with_mem(CMARK_NODE_PARAGRAPH, parser->mem);
  cmark_strbuf_set(&temp_container->content, string, len);

  cmark_manage_extensions_special_characters(parser, true);
  cmark_parser_set_backslash_ispunct_func(parser, table_ispunct);
  cmark_parse_inlines(parser, temp_container, parser->refmap, parser->options);
  cmark_parser_set_backslash_ispunct_func(parser, NULL);
  cmark_manage_extensions_special_characters(parser, false);

  if (!temp_container->first_child) {
    cmark_node_free(temp_container);
    return NULL;
  }

  row = (table_row *)parser->mem->calloc(1, sizeof(table_row));
  row->n_columns = 0;
  row->cells = NULL;

  cmark_node *node = temp_container->first_child;
  int offset = 0;

  maybe_consume_pipe(&node, &offset);
  cmark_node *child;
  while ((child = consume_until_pipe_or_eol(self, parser, &node, &offset)) !=
         NULL) {
    ++row->n_columns;
    row->cells = cmark_llist_append(parser->mem, row->cells, child);
  }

  cmark_node_free(temp_container);

  return row;
}

static cmark_node *try_opening_table_header(cmark_syntax_extension *self,
                                            cmark_parser *parser,
                                            cmark_node *parent_container,
                                            unsigned char *input, int len) {
  bufsize_t matched =
      scan_table_start(input, len, cmark_parser_get_first_nonspace(parser));
  cmark_node *table_header;
  table_row *header_row = NULL;
  table_row *marker_row = NULL;
  node_table_row *ntr;
  const char *parent_string;
  uint16_t i;

  if (!matched)
    return parent_container;

  parent_string = cmark_node_get_string_content(parent_container);

  cmark_arena_push();

  header_row = row_from_string(self, parser, (unsigned char *)parent_string,
                               (int)strlen(parent_string));

  if (!header_row) {
    free_table_row(parser->mem, header_row);
    cmark_arena_pop();
    return parent_container;
  }

  marker_row = row_from_string(self, parser,
                               input + cmark_parser_get_first_nonspace(parser),
                               len - cmark_parser_get_first_nonspace(parser));

  assert(marker_row);

  if (header_row->n_columns != marker_row->n_columns) {
    free_table_row(parser->mem, header_row);
    free_table_row(parser->mem, marker_row);
    cmark_arena_pop();
    return parent_container;
  }

  if (cmark_arena_pop()) {
    header_row = row_from_string(self, parser, (unsigned char *)parent_string,
                                 (int)strlen(parent_string));
    marker_row = row_from_string(self, parser,
                                 input + cmark_parser_get_first_nonspace(parser),
                                 len - cmark_parser_get_first_nonspace(parser));
  }

  if (!cmark_node_set_type(parent_container, CMARK_NODE_TABLE)) {
    free_table_row(parser->mem, header_row);
    free_table_row(parser->mem, marker_row);
    return parent_container;
  }

  cmark_node_set_syntax_extension(parent_container, self);

  parent_container->as.opaque = parser->mem->calloc(1, sizeof(node_table));

  set_n_table_columns(parent_container, header_row->n_columns);

  uint8_t *alignments =
      (uint8_t *)parser->mem->calloc(header_row->n_columns, sizeof(uint8_t));
  cmark_llist *it = marker_row->cells;
  for (i = 0; it; it = it->next, ++i) {
    cmark_node *node = (cmark_node *)it->data;
    assert(node->type == CMARK_NODE_TABLE_CELL);

    cmark_strbuf strbuf;
    cmark_strbuf_init(parser->mem, &strbuf, 0);
    assert(node->first_child->type == CMARK_NODE_TEXT);
    assert(node->first_child == node->last_child);
    cmark_strbuf_put(&strbuf, node->first_child->as.literal.data, node->first_child->as.literal.len);
    cmark_strbuf_trim(&strbuf);
    char const *text = cmark_strbuf_cstr(&strbuf);

    bool left = text[0] == ':', right = text[strbuf.size - 1] == ':';
    cmark_strbuf_free(&strbuf);

    if (left && right)
      alignments[i] = 'c';
    else if (left)
      alignments[i] = 'l';
    else if (right)
      alignments[i] = 'r';
  }
  set_table_alignments(parent_container, alignments);

  table_header =
      cmark_parser_add_child(parser, parent_container, CMARK_NODE_TABLE_ROW,
                             cmark_parser_get_offset(parser));
  cmark_node_set_syntax_extension(table_header, self);

  table_header->as.opaque = ntr = (node_table_row *)parser->mem->calloc(1, sizeof(node_table_row));
  ntr->is_header = true;
  ntr->raw_content_len = strlen(parent_string);
  ntr->raw_content = (unsigned char *)malloc(ntr->raw_content_len);
  memcpy(ntr->raw_content, parent_string, ntr->raw_content_len);

  cmark_parser_advance_offset(
      parser, (char *)input,
      (int)strlen((char *)input) - 1 - cmark_parser_get_offset(parser), false);

  free_table_row(parser->mem, header_row);
  free_table_row(parser->mem, marker_row);
  return parent_container;
}

static cmark_node *try_opening_table_row(cmark_syntax_extension *self,
                                         cmark_parser *parser,
                                         cmark_node *parent_container,
                                         unsigned char *input, int len) {
  cmark_node *table_row_block;
  node_table_row *ntr;

  if (cmark_parser_is_blank(parser))
    return NULL;

  table_row_block =
      cmark_parser_add_child(parser, parent_container, CMARK_NODE_TABLE_ROW,
                             cmark_parser_get_offset(parser));

  cmark_node_set_syntax_extension(table_row_block, self);
  table_row_block->as.opaque = ntr = (node_table_row *)parser->mem->calloc(1, sizeof(node_table_row));

  ntr->raw_content_len = len - cmark_parser_get_first_nonspace(parser);
  ntr->raw_content = (unsigned char *)malloc(len);
  memcpy(ntr->raw_content, input + cmark_parser_get_first_nonspace(parser), ntr->raw_content_len);

  cmark_parser_advance_offset(parser, (char *)input,
                              len - 1 - cmark_parser_get_offset(parser), false);

  return table_row_block;
}

static cmark_node *try_opening_table_block(cmark_syntax_extension *self,
                                           int indented, cmark_parser *parser,
                                           cmark_node *parent_container,
                                           unsigned char *input, int len) {
  cmark_node_type parent_type = cmark_node_get_type(parent_container);

  if (!indented && parent_type == CMARK_NODE_PARAGRAPH) {
    return try_opening_table_header(self, parser, parent_container, input, len);
  } else if (!indented && parent_type == CMARK_NODE_TABLE) {
    return try_opening_table_row(self, parser, parent_container, input, len);
  }

  return NULL;
}

static int matches(cmark_syntax_extension *self, cmark_parser *parser,
                   unsigned char *input, int len,
                   cmark_node *parent_container) {
  int res = 0;

  if (cmark_node_get_type(parent_container) == CMARK_NODE_TABLE) {
    cmark_arena_push();
    table_row *new_row = row_from_string(
        self, parser, input + cmark_parser_get_first_nonspace(parser),
        len - cmark_parser_get_first_nonspace(parser));
    if (new_row && new_row->n_columns)
      res = 1;
    free_table_row(parser->mem, new_row);
    cmark_arena_pop();
  }

  return res;
}

static const char *get_type_string(cmark_syntax_extension *self,
                                   cmark_node *node) {
  if (node->type == CMARK_NODE_TABLE) {
    return "table";
  } else if (node->type == CMARK_NODE_TABLE_ROW) {
    if (((node_table_row *)node->as.opaque)->is_header)
      return "table_header";
    else
      return "table_row";
  } else if (node->type == CMARK_NODE_TABLE_CELL) {
    return "table_cell";
  }

  return "<unknown>";
}

static int can_contain(cmark_syntax_extension *extension, cmark_node *node,
                       cmark_node_type child_type) {
  if (node->type == CMARK_NODE_TABLE) {
    return child_type == CMARK_NODE_TABLE_ROW;
  } else if (node->type == CMARK_NODE_TABLE_ROW) {
    return child_type == CMARK_NODE_TABLE_CELL;
  } else if (node->type == CMARK_NODE_TABLE_CELL) {
    return child_type == CMARK_NODE_TEXT || child_type == CMARK_NODE_CODE ||
           child_type == CMARK_NODE_EMPH || child_type == CMARK_NODE_STRONG ||
           child_type == CMARK_NODE_LINK || child_type == CMARK_NODE_IMAGE ||
           child_type == CMARK_NODE_STRIKETHROUGH ||
           child_type == CMARK_NODE_HTML_INLINE;
  }
  return false;
}

static int contains_inlines(cmark_syntax_extension *extension,
                            cmark_node *node) {
  return node->type == CMARK_NODE_TABLE_CELL;
}

static void commonmark_render(cmark_syntax_extension *extension,
                              cmark_renderer *renderer, cmark_node *node,
                              cmark_event_type ev_type, int options) {
  bool entering = (ev_type == CMARK_EVENT_ENTER);

  if (node->type == CMARK_NODE_TABLE) {
    renderer->blankline(renderer);
  } else if (node->type == CMARK_NODE_TABLE_ROW) {
    if (entering) {
      renderer->cr(renderer);
      renderer->out(renderer, node, "|", false, LITERAL);
    }
  } else if (node->type == CMARK_NODE_TABLE_CELL) {
    if (entering) {
      renderer->out(renderer, node, " ", false, LITERAL);
    } else {
      renderer->out(renderer, node, " |", false, LITERAL);
      if (((node_table_row *)node->parent->as.opaque)->is_header &&
          !node->next) {
        int i;
        uint8_t *alignments = get_table_alignments(node->parent->parent);
        uint16_t n_cols =
            ((node_table *)node->parent->parent->as.opaque)->n_columns;
        renderer->cr(renderer);
        renderer->out(renderer, node, "|", false, LITERAL);
        for (i = 0; i < n_cols; i++) {
          switch (alignments[i]) {
          case 0:   renderer->out(renderer, node, " --- |", false, LITERAL); break;
          case 'l': renderer->out(renderer, node, " :-- |", false, LITERAL); break;
          case 'c': renderer->out(renderer, node, " :-: |", false, LITERAL); break;
          case 'r': renderer->out(renderer, node, " --: |", false, LITERAL); break;
          }
        }
        renderer->cr(renderer);
      }
    }
  } else {
    assert(false);
  }
}

static void latex_render(cmark_syntax_extension *extension,
                         cmark_renderer *renderer, cmark_node *node,
                         cmark_event_type ev_type, int options) {
  bool entering = (ev_type == CMARK_EVENT_ENTER);

  if (node->type == CMARK_NODE_TABLE) {
    if (entering) {
      int i;
      uint16_t n_cols;
      uint8_t *alignments = get_table_alignments(node);

      renderer->cr(renderer);
      renderer->out(renderer, node, "\\begin{table}", false, LITERAL);
      renderer->cr(renderer);
      renderer->out(renderer, node, "\\begin{tabular}{", false, LITERAL);

      n_cols = ((node_table *)node->as.opaque)->n_columns;
      for (i = 0; i < n_cols; i++) {
        switch(alignments[i]) {
        case 0:
        case 'l':
          renderer->out(renderer, node, "l", false, LITERAL);
          break;
        case 'c':
          renderer->out(renderer, node, "c", false, LITERAL);
          break;
        case 'r':
          renderer->out(renderer, node, "r", false, LITERAL);
          break;
        }
      }
      renderer->out(renderer, node, "}", false, LITERAL);
      renderer->cr(renderer);
    } else {
      renderer->out(renderer, node, "\\end{tabular}", false, LITERAL);
      renderer->cr(renderer);
      renderer->out(renderer, node, "\\end{table}", false, LITERAL);
      renderer->cr(renderer);
    }
  } else if (node->type == CMARK_NODE_TABLE_ROW) {
    if (!entering) {
      renderer->cr(renderer);
    }
  } else if (node->type == CMARK_NODE_TABLE_CELL) {
    if (!entering) {
      if (node->next) {
        renderer->out(renderer, node, " & ", false, LITERAL);
      } else {
        renderer->out(renderer, node, " \\\\", false, LITERAL);
      }
    }
  } else {
    assert(false);
  }
}

static void man_render(cmark_syntax_extension *extension,
                       cmark_renderer *renderer, cmark_node *node,
                       cmark_event_type ev_type, int options) {
  bool entering = (ev_type == CMARK_EVENT_ENTER);

  if (node->type == CMARK_NODE_TABLE) {
    if (entering) {
      int i;
      uint16_t n_cols;
      uint8_t *alignments = get_table_alignments(node);

      renderer->cr(renderer);
      renderer->out(renderer, node, ".TS", false, LITERAL);
      renderer->cr(renderer);
      renderer->out(renderer, node, "tab(@);", false, LITERAL);
      renderer->cr(renderer);

      n_cols = ((node_table *)node->as.opaque)->n_columns;

      for (i = 0; i < n_cols; i++) {
        switch (alignments[i]) {
        case 'l':
          renderer->out(renderer, node, "l", false, LITERAL);
          break;
        case 0:
        case 'c':
          renderer->out(renderer, node, "c", false, LITERAL);
          break;
        case 'r':
          renderer->out(renderer, node, "r", false, LITERAL);
          break;
        }
      }

      if (n_cols) {
        renderer->out(renderer, node, ".", false, LITERAL);
        renderer->cr(renderer);
      }
    } else {
      renderer->out(renderer, node, ".TE", false, LITERAL);
      renderer->cr(renderer);
    }
  } else if (node->type == CMARK_NODE_TABLE_ROW) {
    if (!entering) {
      renderer->cr(renderer);
    }
  } else if (node->type == CMARK_NODE_TABLE_CELL) {
    if (!entering && node->next) {
      renderer->out(renderer, node, "@", false, LITERAL);
    }
  } else {
    assert(false);
  }
}

struct html_table_state {
  unsigned need_closing_table_body : 1;
  unsigned in_table_header : 1;
};

static void html_render(cmark_syntax_extension *extension,
                        cmark_html_renderer *renderer, cmark_node *node,
                        cmark_event_type ev_type, int options) {
  bool entering = (ev_type == CMARK_EVENT_ENTER);
  cmark_strbuf *html = renderer->html;
  cmark_node *n;

  // XXX: we just monopolise renderer->opaque.
  struct html_table_state *table_state =
      (struct html_table_state *)&renderer->opaque;

  if (node->type == CMARK_NODE_TABLE) {
    if (entering) {
      cmark_html_render_cr(html);
      cmark_strbuf_puts(html, "<table");
      cmark_html_render_sourcepos(node, html, options);
      cmark_strbuf_putc(html, '>');
      table_state->need_closing_table_body = false;
    } else {
      if (table_state->need_closing_table_body)
        cmark_strbuf_puts(html, "</tbody>");
      table_state->need_closing_table_body = false;
      cmark_strbuf_puts(html, "</table>\n");
    }
  } else if (node->type == CMARK_NODE_TABLE_ROW) {
    if (entering) {
      cmark_html_render_cr(html);
      if (((node_table_row *)node->as.opaque)->is_header) {
        table_state->in_table_header = 1;
        cmark_strbuf_puts(html, "<thead>");
        cmark_html_render_cr(html);
      }
      cmark_strbuf_puts(html, "<tr");
      cmark_html_render_sourcepos(node, html, options);
      cmark_strbuf_putc(html, '>');
    } else {
      cmark_html_render_cr(html);
      cmark_strbuf_puts(html, "</tr>");
      if (((node_table_row *)node->as.opaque)->is_header) {
        cmark_html_render_cr(html);
        cmark_strbuf_puts(html, "</thead>");
        cmark_html_render_cr(html);
        cmark_strbuf_puts(html, "<tbody>");
        table_state->need_closing_table_body = 1;
        table_state->in_table_header = false;
      }
    }
  } else if (node->type == CMARK_NODE_TABLE_CELL) {
    uint8_t *alignments = get_table_alignments(node->parent->parent);
    if (entering) {
      cmark_html_render_cr(html);
      if (table_state->in_table_header) {
        cmark_strbuf_puts(html, "<th");
      } else {
        cmark_strbuf_puts(html, "<td");
      }

      int i = 0;
      for (n = node->parent->first_child; n; n = n->next, ++i)
        if (n == node)
          break;

      switch (alignments[i]) {
      case 'l': cmark_strbuf_puts(html, " align=\"left\""); break;
      case 'c': cmark_strbuf_puts(html, " align=\"center\""); break;
      case 'r': cmark_strbuf_puts(html, " align=\"right\""); break;
      }

      cmark_html_render_sourcepos(node, html, options);
      cmark_strbuf_putc(html, '>');
    } else {
      if (table_state->in_table_header) {
        cmark_strbuf_puts(html, "</th>");
      } else {
        cmark_strbuf_puts(html, "</td>");
      }
    }
  } else {
    assert(false);
  }
}

static void opaque_free(cmark_syntax_extension *self, cmark_mem *mem, cmark_node *node) {
  if (node->type == CMARK_NODE_TABLE) {
    free_node_table(mem, node->as.opaque);
  } else if (node->type == CMARK_NODE_TABLE_ROW) {
    free_node_table_row(mem, node->as.opaque);
  }
}

static int escape(cmark_syntax_extension *self, cmark_node *node, int c) {
  return c == '|';
}

static cmark_node *postprocess(cmark_syntax_extension *self, cmark_parser *parser, cmark_node *root) {
  cmark_iter *iter;
  cmark_event_type ev;
  cmark_node *node;
  node_table_row *ntr;
  table_row *row;

  iter = cmark_iter_new(root);

  while ((ev = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
    node = cmark_iter_get_node(iter);
      if (ev == CMARK_EVENT_EXIT && node->type == CMARK_NODE_TABLE_ROW) {
        ntr = (node_table_row *)node->as.opaque;
        if (!ntr->raw_content)
          continue;
        row = row_from_string(self, parser,
                              ntr->raw_content,
                              (int)ntr->raw_content_len);
        free(ntr->raw_content);
        ntr->raw_content = NULL;
        ntr->raw_content_len = 0;

        {
          cmark_llist *tmp, *next;
          int i;
          int table_columns = get_n_table_columns(node->parent);

          for (tmp = row->cells, i = 0; tmp && i < table_columns; tmp = next, ++i) {
            cmark_node *cell = (cmark_node *)tmp->data;
            assert(cell->type == CMARK_NODE_TABLE_CELL);
            cmark_node_append_child(node, cell);
            row->cells = next = tmp->next;
            parser->mem->free(tmp);
          }

          for (; i < table_columns; ++i) {
            cmark_node *cell =
                cmark_parser_add_child(parser, node, CMARK_NODE_TABLE_CELL,
                                       cmark_parser_get_offset(parser));
            cmark_node_set_syntax_extension(cell, self);
          }
        }

        free_table_row(parser->mem, row);
      }
  }

  cmark_iter_free(iter);

  return root;
}

cmark_syntax_extension *create_table_extension(void) {
  cmark_syntax_extension *self = cmark_syntax_extension_new("table");

  cmark_syntax_extension_set_match_block_func(self, matches);
  cmark_syntax_extension_set_open_block_func(self, try_opening_table_block);
  cmark_syntax_extension_set_get_type_string_func(self, get_type_string);
  cmark_syntax_extension_set_can_contain_func(self, can_contain);
  cmark_syntax_extension_set_contains_inlines_func(self, contains_inlines);
  cmark_syntax_extension_set_commonmark_render_func(self, commonmark_render);
  cmark_syntax_extension_set_latex_render_func(self, latex_render);
  cmark_syntax_extension_set_man_render_func(self, man_render);
  cmark_syntax_extension_set_html_render_func(self, html_render);
  cmark_syntax_extension_set_opaque_free_func(self, opaque_free);
  cmark_syntax_extension_set_commonmark_escape_func(self, escape);
  cmark_syntax_extension_set_postprocess_func(self, postprocess);
  CMARK_NODE_TABLE = cmark_syntax_extension_add_node(0);
  CMARK_NODE_TABLE_ROW = cmark_syntax_extension_add_node(0);
  CMARK_NODE_TABLE_CELL = cmark_syntax_extension_add_node(0);

  return self;
}
