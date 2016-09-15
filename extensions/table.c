#include <parser.h>
#include <html.h>

#include "table.h"
#include "strikethrough.h"
#include "ext_scanners.h"


static cmark_node_type CMARK_NODE_TABLE, CMARK_NODE_TABLE_ROW, CMARK_NODE_TABLE_CELL;

typedef struct {
  int n_columns;
  cmark_llist *cells;
} table_row;

// WARNING: if these grow too large they simply won't fit in the union
// (cmark_node.as).  If you add anything you should probably change it to be
// heap allocated and store the pointer in cmark_node.as.opaque instead.
typedef struct {
  int n_columns;
} node_table;

typedef struct {
  bool is_header;
} node_table_row;


static int get_n_table_columns(cmark_node *node) {
  if (!node || node->type != CMARK_NODE_TABLE)
    return -1;

  return ((node_table *) &node->as.opaque)->n_columns;
}

static int set_n_table_columns(cmark_node *node, int n_columns) {
  if (!node || node->type != CMARK_NODE_TABLE)
    return 0;

  ((node_table *) &node->as.opaque)->n_columns = n_columns;
  return 1;
}

static int is_table_header(cmark_node *node, int is_table_header) {
  if (!node || node->type != CMARK_NODE_TABLE_ROW)
    return 0;

  ((node_table_row *) &node->as.opaque)->is_header = is_table_header;
  return 1;
}

static void free_table_cell(void *data) {
  cmark_strbuf_free((cmark_strbuf *) data);
  free(data);
}

static void free_table_row(table_row *row) {
  if (!row)
    return;

  cmark_llist_free_full(row->cells, (cmark_free_func) free_table_cell);

  free(row);
}

static cmark_strbuf *unescape_pipes(cmark_mem *mem, unsigned char *string, bufsize_t len) {
  cmark_strbuf *res = (cmark_strbuf *)malloc(sizeof(cmark_strbuf));
  bufsize_t r, w;

  cmark_strbuf_init(mem, res, len + 1);
  cmark_strbuf_put(res, string, len);
  cmark_strbuf_putc(res, '\0');

  for (r = 0, w = 0; r < len; ++r) {
    if (res->ptr[r] == '\\' && res->ptr[r + 1] == '|')
      r++;

    res->ptr[w++] = res->ptr[r];
  }

  cmark_strbuf_truncate(res, w);

  return res;
}

static table_row *row_from_string(cmark_mem *mem, unsigned char *string, int len) {
  table_row *row = NULL;
  bufsize_t cell_matched = 0;
  bufsize_t cell_offset = 0;

  row = (table_row *) malloc(sizeof(table_row));
  row->n_columns = 0;
  row->cells = NULL;

  do {
    cell_matched = scan_table_cell(string, len, cell_offset);
    if (cell_matched) {
      cmark_strbuf *cell_buf = unescape_pipes(mem, string + cell_offset + 1,
          cell_matched - 1);
      row->n_columns += 1;
      row->cells = cmark_llist_append(row->cells, cell_buf);
    }
    cell_offset += cell_matched;
  } while (cell_matched);

  cell_matched = scan_table_row_end(string, len, cell_offset);
  cell_offset += cell_matched;

  if (!cell_matched || cell_offset != len) {
    free_table_row(row);
    row = NULL;
  }

  return row;
}

static cmark_node *try_opening_table_header(cmark_syntax_extension *self,
                                            cmark_parser *parser,
                                            cmark_node *parent_container,
                                            unsigned char *input,
                                            int len) {
  bufsize_t matched = scan_table_start(input, len, cmark_parser_get_first_nonspace(parser));
  cmark_node *table_header;
  table_row *header_row = NULL;
  table_row *marker_row = NULL;
  const char *parent_string;

  if (!matched)
    goto done;

  parent_string = cmark_node_get_string_content(parent_container);

  header_row = row_from_string(parser->mem, (unsigned char *) parent_string, strlen(parent_string));

  if (!header_row) {
    goto done;
  }

  marker_row = row_from_string(parser->mem, input + cmark_parser_get_first_nonspace(parser),
      len - cmark_parser_get_first_nonspace(parser));

  assert(marker_row);

  if (header_row->n_columns != marker_row->n_columns) {
    goto done;
  }

  if (!cmark_node_set_type(parent_container, CMARK_NODE_TABLE)) {
    goto done;
  }

  cmark_node_set_syntax_extension(parent_container, self);
  set_n_table_columns(parent_container, header_row->n_columns);

  table_header = cmark_parser_add_child(parser, parent_container,
      CMARK_NODE_TABLE_ROW, cmark_parser_get_offset(parser));
  cmark_node_set_syntax_extension(table_header, self);
  is_table_header(table_header, true);

  {
    cmark_llist *tmp;

    for (tmp = header_row->cells; tmp; tmp = tmp->next) {
      cmark_strbuf *cell_buf = (cmark_strbuf *) tmp->data;
      cmark_node *header_cell = cmark_parser_add_child(parser, table_header,
          CMARK_NODE_TABLE_CELL, cmark_parser_get_offset(parser));
      cmark_node_set_string_content(header_cell, (char *) cell_buf->ptr);
      cmark_node_set_syntax_extension(header_cell, self);
    }
  }

  cmark_parser_advance_offset(parser, (char *) input,
                   strlen((char *) input) - 1 - cmark_parser_get_offset(parser),
                   false);
done:
  free_table_row(header_row);
  free_table_row(marker_row);
  return parent_container;
}

static cmark_node *try_opening_table_row(cmark_syntax_extension *self,
                                         cmark_parser *parser,
                                         cmark_node *parent_container,
                                         unsigned char *input,
                                         int len) {
  cmark_node *table_row_block;
  table_row *row;

  if (cmark_parser_is_blank(parser))
    return NULL;

  table_row_block = cmark_parser_add_child(parser, parent_container,
      CMARK_NODE_TABLE_ROW, cmark_parser_get_offset(parser));

  cmark_node_set_syntax_extension(table_row_block, self);

  /* We don't advance the offset here */

  row = row_from_string(parser->mem, input + cmark_parser_get_first_nonspace(parser),
      len - cmark_parser_get_first_nonspace(parser));

  {
    cmark_llist *tmp;

    for (tmp = row->cells; tmp; tmp = tmp->next) {
      cmark_strbuf *cell_buf = (cmark_strbuf *) tmp->data;
      cmark_node *cell = cmark_parser_add_child(parser, table_row_block,
          CMARK_NODE_TABLE_CELL, cmark_parser_get_offset(parser));
      cmark_node_set_string_content(cell, (char *) cell_buf->ptr);
      cmark_node_set_syntax_extension(cell, self);
    }
  }

  free_table_row(row);

  cmark_parser_advance_offset(parser, (char *) input,
                   len - 1 - cmark_parser_get_offset(parser),
                   false);

  return table_row_block;
}

static cmark_node *try_opening_table_block(cmark_syntax_extension *syntax_extension,
                                           int indented,
                                           cmark_parser *parser,
                                           cmark_node *parent_container,
                                           unsigned char *input,
                                           int len) {
  cmark_node_type parent_type = cmark_node_get_type(parent_container);

  if (!indented && parent_type == CMARK_NODE_PARAGRAPH) {
    return try_opening_table_header(syntax_extension, parser, parent_container, input, len);
  } else if (!indented && parent_type == CMARK_NODE_TABLE) {
    return try_opening_table_row(syntax_extension, parser, parent_container, input, len);
  }

  return NULL;
}

static int matches(cmark_syntax_extension *self,
                   cmark_parser *parser,
                   unsigned char *input,
                   int len,
                   cmark_node *parent_container) {
  int res = 0;

  if (cmark_node_get_type(parent_container) == CMARK_NODE_TABLE) {
    table_row *new_row = row_from_string(parser->mem, input + cmark_parser_get_first_nonspace(parser),
        len - cmark_parser_get_first_nonspace(parser));
    if (new_row) {
        if (new_row->n_columns == get_n_table_columns(parent_container))
          res = 1;
    }
    free_table_row(new_row);
  }

  return res;
}

static const char *get_type_string(cmark_syntax_extension *ext, cmark_node *node) {
  if (node->type == CMARK_NODE_TABLE) {
    return "table";
  } else if (node->type == CMARK_NODE_TABLE_ROW) {
    if (((node_table_row *) &node->as.opaque)->is_header)
      return "table_header";
    else
      return "table_row";
  } else if (node->type == CMARK_NODE_TABLE_CELL) {
    return "table_cell";
  }

  return "<unknown>";
}

static int can_contain(cmark_syntax_extension *extension, cmark_node *node, cmark_node_type child_type) {
  if (node->type == CMARK_NODE_TABLE) {
    return child_type == CMARK_NODE_TABLE_ROW;
  } else if (node->type == CMARK_NODE_TABLE_ROW) {
    return child_type == CMARK_NODE_TABLE_CELL;
  } else if (node->type == CMARK_NODE_TABLE_CELL) {
    return child_type == CMARK_NODE_TEXT ||
           child_type == CMARK_NODE_CODE ||
           child_type == CMARK_NODE_EMPH ||
           child_type == CMARK_NODE_STRONG ||
           child_type == CMARK_NODE_LINK ||
           child_type == CMARK_NODE_IMAGE ||
           child_type == CMARK_NODE_STRIKETHROUGH;
  }
  return false;
}

static int contains_inlines(cmark_syntax_extension *extension, cmark_node *node) {
  return node->type == CMARK_NODE_TABLE_CELL;
}

static void commonmark_render(cmark_syntax_extension *extension, cmark_renderer *renderer, cmark_node *node, cmark_event_type ev_type, int options) {
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
      if (((node_table_row *) &node->parent->as.opaque)->is_header && !node->next) {
        int i;
        int n_cols = ((node_table *) &node->parent->parent->as.opaque)->n_columns;
        renderer->cr(renderer);
        renderer->out(renderer, "|", false, LITERAL);
        for (i = 0; i < n_cols; i++) {
          renderer->out(renderer, " --- |", false, LITERAL);
        }
        renderer->cr(renderer);
      }
    }
  } else {
    assert(false);
  }
}

static void latex_render(cmark_syntax_extension *extension, cmark_renderer *renderer, cmark_node *node, cmark_event_type ev_type, int options) {
  bool entering = (ev_type == CMARK_EVENT_ENTER);

  if (node->type == CMARK_NODE_TABLE) {
    if (entering) {
      int i, n_cols;
      renderer->cr(renderer);
      renderer->out(renderer, "\\begin{table}", false, LITERAL);
      renderer->cr(renderer);
      renderer->out(renderer, "\\begin{tabular}{", false, LITERAL);

      n_cols = ((node_table *) &node->as.opaque)->n_columns;
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

static void man_render(cmark_syntax_extension *extension, cmark_renderer *renderer, cmark_node *node, cmark_event_type ev_type, int options) {
  bool entering = (ev_type == CMARK_EVENT_ENTER);

  if (node->type == CMARK_NODE_TABLE) {
    if (entering) {
      int i, n_cols;
      renderer->cr(renderer);
      renderer->out(renderer, ".TS", false, LITERAL);
      renderer->cr(renderer);
      renderer->out(renderer, "tab(@);", false, LITERAL);
      renderer->cr(renderer);

      n_cols = ((node_table *) &node->as.opaque)->n_columns;

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
  int need_closing_table_body : 1;
  int in_table_header : 1;
};

static void html_render(cmark_syntax_extension *extension,
                              cmark_html_renderer *renderer, cmark_node *node,
                              cmark_event_type ev_type, int options) {
  bool entering = (ev_type == CMARK_EVENT_ENTER);
  cmark_strbuf *html = renderer->html;

  // XXX: we just monopolise renderer->opaque.
  struct html_table_state *table_state = (struct html_table_state *) &renderer->opaque;

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
     if (((node_table_row *) &node->as.opaque)->is_header) {
       table_state->in_table_header = true;
       cmark_strbuf_puts(html, "<thead>");
       cmark_html_render_cr(html);
     }
     cmark_strbuf_puts(html, "<tr");
     cmark_html_render_sourcepos(node, html, options);
     cmark_strbuf_putc(html, '>');
   } else {
     cmark_html_render_cr(html);
     cmark_strbuf_puts(html, "</tr>");
     if (((node_table_row *) &node->as.opaque)->is_header) {
       cmark_html_render_cr(html);
       cmark_strbuf_puts(html, "</thead>");
       cmark_html_render_cr(html);
       cmark_strbuf_puts(html, "<tbody>");
       table_state->need_closing_table_body = true;
       table_state->in_table_header = false;
     }
   }
  } else if (node->type == CMARK_NODE_TABLE_CELL) {
   if (entering) {
     cmark_html_render_cr(html);
     if (table_state->in_table_header) {
       cmark_strbuf_puts(html, "<th");
     } else {
       cmark_strbuf_puts(html, "<td");
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
