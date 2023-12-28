/* 
 * chunk.h is derived from code (C) 2012 Github, Inc.
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

#ifndef CMARK_CHUNK_H
#define CMARK_CHUNK_H

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "cmark.h"
#include "buffer.h"
#include "cmark_ctype.h"

#define CMARK_CHUNK_EMPTY                                                      \
  { NULL, 0 }

typedef struct {
  const unsigned char *data;
  bufsize_t len;
} cmark_chunk;

// NOLINTNEXTLINE(clang-diagnostic-unused-function)
static CMARK_INLINE void cmark_chunk_free(cmark_chunk *c) {
  c->data = NULL;
  c->len = 0;
}

static CMARK_INLINE void cmark_chunk_ltrim(cmark_chunk *c) {
  while (c->len && cmark_isspace(c->data[0])) {
    c->data++;
    c->len--;
  }
}

static CMARK_INLINE void cmark_chunk_rtrim(cmark_chunk *c) {
  while (c->len > 0) {
    if (!cmark_isspace(c->data[c->len - 1]))
      break;

    c->len--;
  }
}

// NOLINTNEXTLINE(clang-diagnostic-unused-function)
static CMARK_INLINE void cmark_chunk_trim(cmark_chunk *c) {
  cmark_chunk_ltrim(c);
  cmark_chunk_rtrim(c);
}

// NOLINTNEXTLINE(clang-diagnostic-unused-function)
static CMARK_INLINE bufsize_t cmark_chunk_strchr(cmark_chunk *ch, int c,
                                                 bufsize_t offset) {
  const unsigned char *p =
      (unsigned char *)memchr(ch->data + offset, c, ch->len - offset);
  return p ? (bufsize_t)(p - ch->data) : ch->len;
}

// NOLINTNEXTLINE(clang-diagnostic-unused-function)
static CMARK_INLINE cmark_chunk cmark_chunk_literal(const char *data) {
  bufsize_t len = data ? (bufsize_t)strlen(data) : 0;
  cmark_chunk c = {(unsigned char *)data, len};
  return c;
}

// NOLINTNEXTLINE(clang-diagnostic-unused-function)
static CMARK_INLINE cmark_chunk cmark_chunk_dup(const cmark_chunk *ch,
                                                bufsize_t pos, bufsize_t len) {
  cmark_chunk c = {ch->data + pos, len};
  return c;
}

#endif
