#ifndef CMARK_INLINES_H
#define CMARK_INLINES_H

#include "chunk.h"
#include "references.h"

#ifdef __cplusplus
extern "C" {
#endif

void cmark_unescape_string(bool backslash_escapes, cmark_strbuf *buf,
                           const unsigned char *src, bufsize_t len);
unsigned char *cmark_clean_url(cmark_mem *mem, cmark_chunk *url);
unsigned char *cmark_clean_title(cmark_mem *mem, cmark_chunk *title);

void cmark_parse_inlines(cmark_mem *mem, cmark_node *parent,
                         cmark_reference_map *refmap, int options);

bufsize_t cmark_parse_reference_inline(cmark_mem *mem, cmark_chunk *input,
                                       cmark_reference_map *refmap);

#ifdef __cplusplus
}
#endif

#endif
