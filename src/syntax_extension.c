#include <stdlib.h>

#include "cmark.h"
#include "syntax_extension.h"
#include "buffer.h"

void cmark_syntax_extension_free(cmark_syntax_extension *extension) {
  cmark_llist_free(extension->special_inline_chars);
  free(extension->name);
  free(extension);
}

cmark_syntax_extension *cmark_syntax_extension_new(const char *name) {
  cmark_syntax_extension *res = (cmark_syntax_extension *) calloc(1, sizeof(cmark_syntax_extension));
  res->name = (char *) malloc(sizeof(char) * (strlen(name)) + 1);
  strcpy(res->name, name);
  return res;
}

void cmark_syntax_extension_set_open_block_func(cmark_syntax_extension *extension,
                                                cmark_open_block_func func) {
  extension->try_opening_block = func;
}

void cmark_syntax_extension_set_match_block_func(cmark_syntax_extension *extension,
                                                 cmark_match_block_func func) {
  extension->last_block_matches = func;
}

void cmark_syntax_extension_set_match_inline_func(cmark_syntax_extension *extension,
                                                  cmark_match_inline_func func) {
  extension->match_inline = func;
}

void cmark_syntax_extension_set_inline_from_delim_func(cmark_syntax_extension *extension,
                                                       cmark_inline_from_delim_func func) {
  extension->insert_inline_from_delim = func;
}

void cmark_syntax_extension_set_special_inline_chars(cmark_syntax_extension *extension,
                                                     cmark_llist *special_chars) {
  extension->special_inline_chars = special_chars;
}
