#ifndef CMARK_INLINES_H
#define CMARK_INLINES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "references.h"

cmark_chunk cmark_clean_url(cmark_mem *mem, cmark_chunk *url);
cmark_chunk cmark_clean_title(cmark_mem *mem, cmark_chunk *title);

CMARK_EXPORT
void cmark_parse_inlines(cmark_parser *parser,
                         cmark_node *parent,
                         cmark_reference_map *refmap,
                         int options);

bufsize_t cmark_parse_reference_inline(cmark_mem *mem, cmark_strbuf *input,
                                       cmark_reference_map *refmap);

void cmark_inlines_add_special_character(unsigned char c);
void cmark_inlines_remove_special_character(unsigned char c);

#ifdef __cplusplus
}
#endif

#endif
