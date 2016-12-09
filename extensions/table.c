#include <html.h>
#include <inlines.h>
#include <parser.h>
#include <references.h>

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

typedef enum {
  ALIGN_NONE,
  ALIGN_LEFT,
  ALIGN_CENTER,
  ALIGN_RIGHT
} table_column_alignment;

typedef struct { bool is_header; } node_table_row;

static void free_node_table(cmark_mem *mem, void *ptr) {
  node_table *t = (node_table *)ptr;
  mem->free(t->alignments);
  mem->free(t);
}

static void free_node_table_row(cmark_mem *mem, void *ptr) { mem->free(ptr); }

static int get_n_table_columns(cmark_node *node) {
  if (!node || node->type != CMARK_NODE_TABLE)
    return -1;

  return (int)((node_table *)node->user_data)->n_columns;
}

static int set_n_table_columns(cmark_node *node, uint16_t n_columns) {
  if (!node || node->type != CMARK_NODE_TABLE)
    return 0;

  ((node_table *)node->user_data)->n_columns = n_columns;
  return 1;
}

static uint8_t *get_table_alignments(cmark_node *node) {
  if (!node || node->type != CMARK_NODE_TABLE)
    return 0;

  return ((node_table *)node->user_data)->alignments;
}

static int set_table_alignments(cmark_node *node, uint8_t *alignments) {
  if (!node || node->type != CMARK_NODE_TABLE)
    return 0;

  ((node_table *)node->user_data)->alignments = alignments;
  return 1;
}

static int is_table_header(cmark_node *node, int is_table_header) {
  if (!node || node->type != CMARK_NODE_TABLE_ROW)
    return 0;

  ((node_table_row *)node->user_data)->is_header = (is_table_header != 0);
  return 1;
}

static void free_table_cell(cmark_mem *mem, void *data) {
  cmark_node_free((cmark_node *)data);
}

static void free_table_row(cmark_mem *mem, table_row *row) {
  if (!row)
    return;

  cmark_llist_free_full(mem, row->cells, (cmark_free_func)free_table_cell);

  mem->free(row);
}

static void reescape_pipes(cmark_strbuf *strbuf, cmark_mem *mem,
                           unsigned char *string, bufsize_t len) {
  bufsize_t r;

  cmark_strbuf_init(mem, strbuf, len * 2);
  for (r = 0; r < len; ++r) {
    if (string[r] == '\\' && r + 1 < len &&
        (string[r + 1] == '|' || string[r + 1] == '\\'))
      cmark_strbuf_putc(strbuf, '\\');

    cmark_strbuf_putc(strbuf, string[r]);
  }
}

static void maybe_consume_pipe(cmark_node **n, int *offset) {
  if (*n && (*n)->type == CMARK_NODE_TEXT && *offset < (*n)->as.literal.len &&
      (*n)->as.literal.data[*offset] == '|')
    ++(*offset);
}

static const char *find_unescaped_pipe(const char *cstr, size_t len) {
  bool escaping = false;
  for (; len; --len, ++cstr) {
    if (escaping)
      escaping = false;
    else if (*cstr == '\\')
      escaping = true;
    else if (*cstr == '|')
      return cstr;
  }
  return NULL;
}

static cmark_node *consume_until_pipe_or_eol(cmark_syntax_extension *self,
                                             cmark_parser *parser,
                                             cmark_node **n, int *offset) {
  cmark_node *result =
      cmark_node_new_with_mem(CMARK_NODE_TABLE_CELL, parser->mem);
  cmark_node_set_syntax_extension(result, self);
  bool was_escape = false;

  while (*n) {
    if ((*n)->type == CMARK_NODE_TEXT) {
      cmark_node *child = cmark_parser_add_child(
          parser, result, CMARK_NODE_TEXT, cmark_parser_get_offset(parser));

      const char *cstr = cmark_chunk_to_cstr(parser->mem, &(*n)->as.literal);

      if (was_escape) {
        child->as.literal = cmark_chunk_dup(&(*n)->as.literal, *offset, 1);
        cmark_node_own(child);
        ++*offset;
        was_escape = false;
        continue;
      }

      if (strcmp(cstr + *offset, "\\") == 0 && (*n)->next &&
          (*n)->next->type == CMARK_NODE_TEXT) {
        was_escape = true;
        *n = (*n)->next;
        continue;
      }

      const char *pipe =
          find_unescaped_pipe(cstr + *offset, (*n)->as.literal.len - *offset);

      if (!pipe) {
        child->as.literal = cmark_chunk_dup(&(*n)->as.literal, *offset,
                                            (*n)->as.literal.len - *offset);
        cmark_node_own(child);
      } else {
        int len = (int)(pipe - cstr - *offset);
        child->as.literal = cmark_chunk_dup(&(*n)->as.literal, *offset, len);
        cmark_node_own(child);
        *offset += len + 1;
        if (*offset >= (*n)->as.literal.len) {
          *offset = 0;
          *n = (*n)->next;
        }
        return result;
      }

      *n = (*n)->next;
      *offset = 0;
    } else {
      cmark_node *next = (*n)->next;
      cmark_node_append_child(result, *n);
      cmark_node_own(*n);
      *n = next;
      *offset = 0;
    }
  }

  if (!result->first_child) {
    cmark_node_free(result);
    result = NULL;
  }

  return result;
}

static table_row *row_from_string(cmark_syntax_extension *self,
                                  cmark_parser *parser, unsigned char *string,
                                  int len) {
  table_row *row = NULL;

  cmark_node *temp_container =
      cmark_node_new_with_mem(CMARK_NODE_PARAGRAPH, parser->mem);
  reescape_pipes(&temp_container->content, parser->mem, string, len);

  cmark_manage_extensions_special_characters(parser, true);
  cmark_parse_inlines(parser, temp_container, parser->refmap, parser->options);
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
  cmark_node *table_header, *child;
  table_row *header_row = NULL;
  table_row *marker_row = NULL;
  const char *parent_string;
  uint16_t i;

  if (!matched)
    goto done;

  parent_string = cmark_node_get_string_content(parent_container);

  header_row = row_from_string(self, parser, (unsigned char *)parent_string,
                               (int)strlen(parent_string));

  if (!header_row) {
    goto done;
  }

  marker_row = row_from_string(self, parser,
                               input + cmark_parser_get_first_nonspace(parser),
                               len - cmark_parser_get_first_nonspace(parser));

  assert(marker_row);

  if (header_row->n_columns != marker_row->n_columns) {
    goto done;
  }

  if (!cmark_node_set_type(parent_container, CMARK_NODE_TABLE)) {
    goto done;
  }

  cmark_node_set_syntax_extension(parent_container, self);

  cmark_node_set_user_data(parent_container,
                           parser->mem->calloc(1, sizeof(node_table)));
  cmark_node_set_user_data_free_func(parent_container, free_node_table);

  set_n_table_columns(parent_container, header_row->n_columns);

  uint8_t *alignments =
      (uint8_t *)parser->mem->calloc(header_row->n_columns, sizeof(uint8_t));
  cmark_llist *it = marker_row->cells;
  for (i = 0; it; it = it->next, ++i) {
    cmark_node *node = (cmark_node *)it->data;
    assert(node->type == CMARK_NODE_TABLE_CELL);

    cmark_strbuf strbuf;
    cmark_strbuf_init(parser->mem, &strbuf, 0);
    for (child = node->first_child; child; child = child->next) {
      assert(child->type == CMARK_NODE_TEXT);
      cmark_strbuf_put(&strbuf, child->as.literal.data, child->as.literal.len);
    }
    cmark_strbuf_trim(&strbuf);
    char const *text = cmark_strbuf_cstr(&strbuf);

    bool left = text[0] == ':', right = text[strbuf.size - 1] == ':';
    cmark_strbuf_free(&strbuf);

    if (left && right)
      alignments[i] = ALIGN_CENTER;
    else if (left)
      alignments[i] = ALIGN_LEFT;
    else if (right)
      alignments[i] = ALIGN_RIGHT;
  }
  set_table_alignments(parent_container, alignments);

  table_header =
      cmark_parser_add_child(parser, parent_container, CMARK_NODE_TABLE_ROW,
                             cmark_parser_get_offset(parser));
  cmark_node_set_syntax_extension(table_header, self);

  cmark_node_set_user_data(table_header,
                           parser->mem->calloc(1, sizeof(node_table_row)));
  cmark_node_set_user_data_free_func(table_header, free_node_table_row);
  is_table_header(table_header, true);

  {
    cmark_llist *tmp, *next;

    for (tmp = header_row->cells; tmp; tmp = next) {
      cmark_node *header_cell = (cmark_node *)tmp->data;
      cmark_node_append_child(table_header, header_cell);
      next = header_row->cells = tmp->next;
      parser->mem->free(tmp);
    }
  }

  cmark_parser_advance_offset(
      parser, (char *)input,
      (int)strlen((char *)input) - 1 - cmark_parser_get_offset(parser), false);
done:
  free_table_row(parser->mem, header_row);
  free_table_row(parser->mem, marker_row);
  return parent_container;
}

static cmark_node *try_opening_table_row(cmark_syntax_extension *self,
                                         cmark_parser *parser,
                                         cmark_node *parent_container,
                                         unsigned char *input, int len) {
  cmark_node *table_row_block;
  table_row *row;

  if (cmark_parser_is_blank(parser))
    return NULL;

  table_row_block =
      cmark_parser_add_child(parser, parent_container, CMARK_NODE_TABLE_ROW,
                             cmark_parser_get_offset(parser));

  cmark_node_set_syntax_extension(table_row_block, self);
  cmark_node_set_user_data(table_row_block,
                           parser->mem->calloc(1, sizeof(node_table_row)));
  cmark_node_set_user_data_free_func(table_row_block, free_node_table_row);

  /* We don't advance the offset here */

  row = row_from_string(self, parser,
                        input + cmark_parser_get_first_nonspace(parser),
                        len - cmark_parser_get_first_nonspace(parser));

  {
    cmark_llist *tmp, *next;
    int i;
    int table_columns = get_n_table_columns(parent_container);

    for (tmp = row->cells, i = 0; tmp && i < table_columns; tmp = next, ++i) {
      cmark_node *cell = (cmark_node *)tmp->data;
      assert(cell->type == CMARK_NODE_TABLE_CELL);
      cmark_node_append_child(table_row_block, cell);
      row->cells = next = tmp->next;
      parser->mem->free(tmp);
    }

    for (; i < table_columns; ++i) {
      cmark_node *cell =
          cmark_parser_add_child(parser, table_row_block, CMARK_NODE_TABLE_CELL,
                                 cmark_parser_get_offset(parser));
      cmark_node_set_syntax_extension(cell, self);
    }
  }

  free_table_row(parser->mem, row);

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
    table_row *new_row = row_from_string(
        self, parser, input + cmark_parser_get_first_nonspace(parser),
        len - cmark_parser_get_first_nonspace(parser));
    if (new_row && new_row->n_columns)
      res = 1;
    free_table_row(parser->mem, new_row);
  }

  return res;
}

static const char *get_type_string(cmark_syntax_extension *ext,
                                   cmark_node *node) {
  if (node->type == CMARK_NODE_TABLE) {
    return "table";
  } else if (node->type == CMARK_NODE_TABLE_ROW) {
    if (((node_table_row *)node->user_data)->is_header)
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
      renderer->out(renderer, "|", false, LITERAL);
    }
  } else if (node->type == CMARK_NODE_TABLE_CELL) {
    if (entering) {
    } else {
      renderer->out(renderer, " |", false, LITERAL);
      if (((node_table_row *)node->parent->user_data)->is_header &&
          !node->next) {
        int i;
        uint8_t *alignments = get_table_alignments(node->parent->parent);
        uint16_t n_cols =
            ((node_table *)node->parent->parent->user_data)->n_columns;
        renderer->cr(renderer);
        renderer->out(renderer, "|", false, LITERAL);
        for (i = 0; i < n_cols; i++) {
          if (alignments[i] == ALIGN_NONE)
            renderer->out(renderer, " --- |", false, LITERAL);
          else if (alignments[i] == ALIGN_LEFT)
            renderer->out(renderer, " :-- |", false, LITERAL);
          else if (alignments[i] == ALIGN_CENTER)
            renderer->out(renderer, " :-: |", false, LITERAL);
          else if (alignments[i] == ALIGN_RIGHT)
            renderer->out(renderer, " --: |", false, LITERAL);
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
      renderer->cr(renderer);
      renderer->out(renderer, "\\begin{table}", false, LITERAL);
      renderer->cr(renderer);
      renderer->out(renderer, "\\begin{tabular}{", false, LITERAL);

      n_cols = ((node_table *)node->user_data)->n_columns;
      for (i = 0; i < n_cols; i++) {
        renderer->out(renderer, "l", false, LITERAL);
      }
      renderer->out(renderer, "}", false, LITERAL);
      renderer->cr(renderer);
    } else {
      renderer->out(renderer, "\\end{tabular}", false, LITERAL);
      renderer->cr(renderer);
      renderer->out(renderer, "\\end{table}", false, LITERAL);
      renderer->cr(renderer);
    }
  } else if (node->type == CMARK_NODE_TABLE_ROW) {
    if (!entering) {
      renderer->cr(renderer);
    }
  } else if (node->type == CMARK_NODE_TABLE_CELL) {
    if (!entering) {
      if (node->next) {
        renderer->out(renderer, " & ", false, LITERAL);
      } else {
        renderer->out(renderer, " \\\\", false, LITERAL);
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
      renderer->cr(renderer);
      renderer->out(renderer, ".TS", false, LITERAL);
      renderer->cr(renderer);
      renderer->out(renderer, "tab(@);", false, LITERAL);
      renderer->cr(renderer);

      n_cols = ((node_table *)node->user_data)->n_columns;

      for (i = 0; i < n_cols; i++) {
        renderer->out(renderer, "c", false, LITERAL);
      }

      if (n_cols) {
        renderer->out(renderer, ".", false, LITERAL);
        renderer->cr(renderer);
      }
    } else {
      renderer->out(renderer, ".TE", false, LITERAL);
      renderer->cr(renderer);
    }
  } else if (node->type == CMARK_NODE_TABLE_ROW) {
    if (!entering) {
      renderer->cr(renderer);
    }
  } else if (node->type == CMARK_NODE_TABLE_CELL) {
    if (!entering && node->next) {
      renderer->out(renderer, "@", false, LITERAL);
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
      if (((node_table_row *)node->user_data)->is_header) {
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
      if (((node_table_row *)node->user_data)->is_header) {
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

      if (alignments[i] == ALIGN_LEFT)
        cmark_strbuf_puts(html, " align=\"left\"");
      else if (alignments[i] == ALIGN_CENTER)
        cmark_strbuf_puts(html, " align=\"center\"");
      else if (alignments[i] == ALIGN_RIGHT)
        cmark_strbuf_puts(html, " align=\"right\"");

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

cmark_syntax_extension *create_table_extension(void) {
  cmark_syntax_extension *ext = cmark_syntax_extension_new("table");

  cmark_syntax_extension_set_match_block_func(ext, matches);
  cmark_syntax_extension_set_open_block_func(ext, try_opening_table_block);
  cmark_syntax_extension_set_get_type_string_func(ext, get_type_string);
  cmark_syntax_extension_set_can_contain_func(ext, can_contain);
  cmark_syntax_extension_set_contains_inlines_func(ext, contains_inlines);
  cmark_syntax_extension_set_commonmark_render_func(ext, commonmark_render);
  cmark_syntax_extension_set_latex_render_func(ext, latex_render);
  cmark_syntax_extension_set_man_render_func(ext, man_render);
  cmark_syntax_extension_set_html_render_func(ext, html_render);
  CMARK_NODE_TABLE = cmark_syntax_extension_add_node(0);
  CMARK_NODE_TABLE_ROW = cmark_syntax_extension_add_node(0);
  CMARK_NODE_TABLE_CELL = cmark_syntax_extension_add_node(0);

  return ext;
}
