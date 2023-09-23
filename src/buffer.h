/*
 * buffer.c is derived from code (C) 2012 Github, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
*/

#ifndef CMARK_BUFFER_H
#define CMARK_BUFFER_H

#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include "config.h"
#include "cmark.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int32_t bufsize_t;

typedef struct {
  cmark_mem *mem;
  unsigned char *ptr;
  bufsize_t asize, size;
} cmark_strbuf;

extern unsigned char cmark_strbuf__initbuf[];

#define CMARK_BUF_INIT(mem)                                                    \
  { mem, cmark_strbuf__initbuf, 0, 0 }

/**
 * Initialize a cmark_strbuf structure.
 *
 * For the cases where CMARK_BUF_INIT cannot be used to do static
 * initialization.
 */
void cmark_strbuf_init(cmark_mem *mem, cmark_strbuf *buf,
                       bufsize_t initial_size);

/**
 * Grow the buffer to hold at least `target_size` bytes.
 */
void cmark_strbuf_grow(cmark_strbuf *buf, bufsize_t target_size);

void cmark_strbuf_free(cmark_strbuf *buf);
void cmark_strbuf_swap(cmark_strbuf *buf_a, cmark_strbuf *buf_b);

bufsize_t cmark_strbuf_len(const cmark_strbuf *buf);

int cmark_strbuf_cmp(const cmark_strbuf *a, const cmark_strbuf *b);

unsigned char *cmark_strbuf_detach(cmark_strbuf *buf);
void cmark_strbuf_copy_cstr(char *data, bufsize_t datasize,
                            const cmark_strbuf *buf);

/*
static CMARK_INLINE const char *cmark_strbuf_cstr(const cmark_strbuf *buf) {
 return (char *)buf->ptr;
}
*/

#define cmark_strbuf_at(buf, n) ((buf)->ptr[n])

void cmark_strbuf_set(cmark_strbuf *buf, const unsigned char *data,
                      bufsize_t len);
void cmark_strbuf_sets(cmark_strbuf *buf, const char *string);
void cmark_strbuf_putc(cmark_strbuf *buf, int c);
void cmark_strbuf_put(cmark_strbuf *buf, const unsigned char *data,
                      bufsize_t len);
void cmark_strbuf_puts(cmark_strbuf *buf, const char *string);
void cmark_strbuf_clear(cmark_strbuf *buf);

bufsize_t cmark_strbuf_strchr(const cmark_strbuf *buf, int c, bufsize_t pos);
bufsize_t cmark_strbuf_strrchr(const cmark_strbuf *buf, int c, bufsize_t pos);
void cmark_strbuf_drop(cmark_strbuf *buf, bufsize_t n);
void cmark_strbuf_truncate(cmark_strbuf *buf, bufsize_t len);
void cmark_strbuf_rtrim(cmark_strbuf *buf);
void cmark_strbuf_trim(cmark_strbuf *buf);
void cmark_strbuf_normalize_whitespace(cmark_strbuf *s);
void cmark_strbuf_unescape(cmark_strbuf *s);

#ifdef __cplusplus
}
#endif

#endif
