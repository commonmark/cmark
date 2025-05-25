#ifndef CMARK_PARSER_FORK_H
#define CMARK_PARSER_FORK_H

#include "parser.h"

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * Create a fork of an existing parser. (Experimental)
   * The fork will have its own copy of the document tree and reference map,
   * but shares the same memory allocator and configuration.
   */
  cmark_parser *cmark_parser_fork(cmark_parser *parser);

#ifdef __cplusplus
}
#endif

#endif
