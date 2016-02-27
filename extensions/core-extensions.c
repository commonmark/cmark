#include <stdio.h>
#include <string.h>

#include <cmark.h>
#include <cmark_extension_api.h>

#include "buffer.h"
#include "ext_scanners.h"

typedef struct {
  int n_columns;
  cmark_llist *cells;
} table_row;

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

static cmark_strbuf *unescape_pipes(unsigned char *string, bufsize_t len)
{
  cmark_strbuf *res = (cmark_strbuf *)malloc(sizeof(cmark_strbuf));
  bufsize_t r, w;

  cmark_strbuf_init(res, len + 1);
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

static table_row *row_from_string(unsigned char *string, int len) {
  table_row *row = NULL;
  bufsize_t cell_matched = 0;
  bufsize_t cell_offset = 0;

  row = malloc(sizeof(table_row));
  row->n_columns = 0;
  row->cells = NULL;

  do {
    cell_matched = scan_table_cell(string, len, cell_offset);
    if (cell_matched) {
      cmark_strbuf *cell_buf = unescape_pipes(string + cell_offset + 1,
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
                                            cmark_parser * parser,
                                            cmark_node   * parent_container,
                                            unsigned char   * input,
                                            int len) {
  bufsize_t matched = scan_table_start(input, len, cmark_parser_get_first_nonspace(parser));
  cmark_node *table_header;
  table_row *header_row = NULL;
  table_row *marker_row = NULL;
  const char *parent_string;

  if (!matched)
    goto done;

  parent_string = cmark_node_get_string_content(parent_container);

  header_row = row_from_string((unsigned char *) parent_string, strlen(parent_string));

  if (!header_row) {
    goto done;
  }

  marker_row = row_from_string(input + cmark_parser_get_first_nonspace(parser),
      len - cmark_parser_get_first_nonspace(parser));

  assert(marker_row);

  if (header_row->n_columns != marker_row->n_columns) {
    goto done;
  }

  if (!cmark_node_set_type(parent_container, CMARK_NODE_TABLE)) {
    goto done;
  }

  cmark_node_set_syntax_extension(parent_container, self);
  cmark_node_set_n_table_columns(parent_container, header_row->n_columns);

  table_header = cmark_parser_add_child(parser, parent_container,
      CMARK_NODE_TABLE_ROW, cmark_parser_get_offset(parser));
  cmark_node_set_syntax_extension(table_header, self);
  cmark_node_set_is_table_header(table_header, true);

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

  cmark_parser_advance_offset(parser, input,
                   strlen(input) - 1 - cmark_parser_get_offset(parser),
                   false);
done:
  free_table_row(header_row);
  free_table_row(marker_row);
  return parent_container;
}

static cmark_node *try_opening_table_row(cmark_syntax_extension *self,
                                         cmark_parser * parser,
                                         cmark_node   * parent_container,
                                         unsigned char   * input,
                                         int len) {
  cmark_node *table_row_block;
  table_row *row;

  if (cmark_parser_is_blank(parser))
    return NULL;

  table_row_block = cmark_parser_add_child(parser, parent_container,
      CMARK_NODE_TABLE_ROW, cmark_parser_get_offset(parser));

  cmark_node_set_syntax_extension(table_row_block, self);

  /* We don't advance the offset here */

  row = row_from_string(input + cmark_parser_get_first_nonspace(parser),
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

  cmark_parser_advance_offset(parser, input,
                   len - 1 - cmark_parser_get_offset(parser),
                   false);

  return table_row_block;
}

static cmark_node *try_opening_table_block(cmark_syntax_extension * syntax_extension,
                                           int               indented,
                                           cmark_parser    * parser,
                                           cmark_node      * parent_container,
                                           unsigned char      * input,
                                           int len) {
  cmark_node_type parent_type = cmark_node_get_type(parent_container);

  if (!indented && parent_type == CMARK_NODE_PARAGRAPH) {
    return try_opening_table_header(syntax_extension, parser, parent_container, input, len);
  } else if (!indented && parent_type == CMARK_NODE_TABLE) {
    return try_opening_table_row(syntax_extension, parser, parent_container, input, len);
  }

  return NULL;
}

static int table_matches(cmark_syntax_extension *self,
                          cmark_parser * parser,
                          unsigned char   * input,
                          int len,
                          cmark_node   * parent_container) {
  int res = 0;

  if (cmark_node_get_type(parent_container) == CMARK_NODE_TABLE) {
    table_row *new_row = row_from_string(input + cmark_parser_get_first_nonspace(parser),
        len - cmark_parser_get_first_nonspace(parser));
    if (new_row) {
        if (new_row->n_columns == cmark_node_get_n_table_columns(parent_container))
          res = 1;
    }
    free_table_row(new_row);
  }

  return res;
}

static cmark_syntax_extension *register_table_syntax_extension(void) {
  cmark_syntax_extension *ext = cmark_syntax_extension_new("piped-tables");

  cmark_syntax_extension_set_match_block_func(ext, table_matches);
  cmark_syntax_extension_set_open_block_func(ext, try_opening_table_block);

  return ext;
}

int init_libcmarkextensions(cmark_plugin *plugin) {
  cmark_plugin_register_syntax_extension(plugin, register_table_syntax_extension());
  return 1;
}
