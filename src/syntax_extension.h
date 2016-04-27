#ifndef SYNTAX_EXTENSION_H
#define SYNTAX_EXTENSION_H

#include "cmark.h"
#include "cmark_extension_api.h"

struct cmark_syntax_extension {
  cmark_match_block_func   last_block_matches;
  cmark_open_block_func    try_opening_block;
  char                   * name;
};

#endif
