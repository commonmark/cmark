#ifndef SYNTAX_EXTENSION_H
#define SYNTAX_EXTENSION_H

#include "cmark.h"
#include "cmark_extension_api.h"

struct cmark_syntax_extension {
  cmark_match_block_func          last_block_matches;
  cmark_open_block_func           try_opening_block;
  cmark_match_inline_func         match_inline;
  cmark_inline_from_delim_func    insert_inline_from_delim;
  cmark_llist                   * special_inline_chars;
  char                          * name;
  void                          * priv;
  cmark_free_func                 free_function;
};

#endif
