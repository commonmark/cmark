#ifndef CMARK_HOUDINI_H
#define CMARK_HOUDINI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "config.h"
#include "buffer.h"

#ifdef HAVE___BUILTIN_EXPECT
#define likely(x) __builtin_expect((x), 1)
#define unlikely(x) __builtin_expect((x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

#define HOUDINI_ESCAPED_SIZE(x) (((x)*12) / 10)
#define HOUDINI_UNESCAPED_SIZE(x) (x)

extern bufsize_t houdini_unescape_ent(cmark_strbuf *ob, const uint8_t *src,
                                      bufsize_t size, int convert_entities);
extern int houdini_escape_html(cmark_strbuf *ob, const uint8_t *src,
                               bufsize_t size);
extern int houdini_escape_html0(cmark_strbuf *ob, const uint8_t *src,
                                bufsize_t size, int secure);
extern int houdini_unescape_html(cmark_strbuf *ob, const uint8_t *src,
                                 bufsize_t size);
extern void houdini_unescape_html_f(cmark_strbuf *ob, const uint8_t *src,
                                    bufsize_t size);
extern int houdini_escape_href(cmark_strbuf *ob, const uint8_t *src,
                               bufsize_t size);

#ifdef __cplusplus
}
#endif

#endif
