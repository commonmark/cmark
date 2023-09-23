/*
 * utf8.h is derived from utf8proc
 * (<http://www.public-software-group.org/utf8proc>),
 * (C) 2009 Public Software Group e. V., Berlin, Germany.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
*/

#ifndef CMARK_UTF8_H
#define CMARK_UTF8_H

#include <stdint.h>
#include "buffer.h"

#ifdef __cplusplus
extern "C" {
#endif

void cmark_utf8proc_case_fold(cmark_strbuf *dest, const uint8_t *str,
                              bufsize_t len);
void cmark_utf8proc_encode_char(int32_t uc, cmark_strbuf *buf);
int cmark_utf8proc_iterate(const uint8_t *str, bufsize_t str_len, int32_t *dst);
void cmark_utf8proc_check(cmark_strbuf *dest, const uint8_t *line,
                          bufsize_t size);
int cmark_utf8proc_is_space(int32_t uc);
int cmark_utf8proc_is_punctuation(int32_t uc);

#ifdef __cplusplus
}
#endif

#endif
