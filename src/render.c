#include <stdlib.h>
#include "buffer.h"
#include "cmark.h"
#include "utf8.h"
#include "render.h"
#include "node.h"
#include "cmark_ctype.h"

static inline void S_cr(cmark_renderer *renderer) {
  if (renderer->need_cr < 1) {
    renderer->need_cr = 1;
  }
}

static inline void S_blankline(cmark_renderer *renderer) {
  if (renderer->need_cr < 2) {
    renderer->need_cr = 2;
  }
}

// Decide whether the text at the start of a line produced by wrapping
// could be read as the start of a block in commonmark output (list marker,
// heading, block quote, HTML block, thematic break, setext underline, code
// fence). Returns 1 if so, 0 if not, -1 if more text is needed to tell.
static int S_bad_line_start(const unsigned char *s, bufsize_t len) {
  bufsize_t i = 0;

  if (len == 0) {
    return -1;
  }
  switch (s[0]) {
  case '#':
  case '>':
  case '<':
  case '-':
  case '=':
  case '+':
  case '*':
  case '_':
  case '~':
  case '`':
    return 1;
  }
  if (cmark_isdigit(s[0])) {
    // ordered list markers have at most 9 digits
    while (i < len && i < 10 && cmark_isdigit(s[i])) {
      i++;
    }
    if (i == len) {
      return -1;
    }
    return i < 10 && (s[i] == '.' || s[i] == ')');
  }
  if (cmark_isalpha(s[0])) {
    if (len < 2) {
      return -1;
    }
    return s[1] == '.';
  }
  return 0;
}

static void S_out(cmark_renderer *renderer, const char *source, bool wrap,
                  cmark_escaping escape) {
  int length = (int)strlen(source);
  unsigned char nextc;
  int32_t c;
  int i = 0;
  int last_nonspace;
  int len;
  int breakable;
  unsigned char peek[12];
  int n;
  bufsize_t j;
  int k = renderer->buffer->size - 1;

  wrap = wrap && !renderer->no_linebreaks;

  if (renderer->in_tight_list_item && renderer->need_cr > 1) {
    renderer->need_cr = 1;
  }
  while (renderer->need_cr) {
    if (k < 0 || renderer->buffer->ptr[k] == '\n') {
      k -= 1;
    } else {
      cmark_strbuf_putc(renderer->buffer, '\n');
      if (renderer->need_cr > 1) {
        cmark_strbuf_put(renderer->buffer, renderer->prefix->ptr,
                         renderer->prefix->size);
      }
    }
    renderer->column = 0;
    renderer->last_breakable = 0;
    renderer->begin_line = true;
    renderer->begin_content = true;
    renderer->need_cr -= 1;
  }

  while (i < length) {
    if (renderer->begin_line) {
      cmark_strbuf_put(renderer->buffer, renderer->prefix->ptr,
                       renderer->prefix->size);
      // note: this assumes prefix is ascii:
      renderer->column = renderer->prefix->size;
    }

    len = cmark_utf8proc_iterate((const uint8_t *)source + i, length - i, &c);
    if (len == -1) { // error condition
      return;        // return without rendering rest of string
    }
    nextc = source[i + len];
    if (c == 32 && wrap) {
      if (!renderer->begin_line) {
        last_nonspace = renderer->buffer->size;
        cmark_strbuf_putc(renderer->buffer, ' ');
        renderer->column += 1;
        renderer->begin_line = false;
        renderer->begin_content = false;
        // skip following spaces
        while (source[i + 1] == ' ') {
          i++;
        }
        // Keep the previous break point if this one would put something
        // that can start a block at the beginning of a line (see below).
        if (S_bad_line_start((const unsigned char *)source + i + 1,
                             length - i - 1) != 1) {
          renderer->last_breakable = last_nonspace;
        }
      }

    } else if (escape == LITERAL) {
      if (c == 10) {
        cmark_strbuf_putc(renderer->buffer, '\n');
        renderer->column = 0;
        renderer->begin_line = true;
        renderer->begin_content = true;
        renderer->last_breakable = 0;
      } else {
        cmark_render_code_point(renderer, c);
        renderer->begin_line = false;
        // we don't set 'begin_content' to false til we've
        // finished parsing a digit.  Reason:  in commonmark
        // we need to escape a potential list marker after
        // a digit:
        renderer->begin_content =
            renderer->begin_content && cmark_isdigit(c) == 1;
      }
    } else {
      (renderer->outc)(renderer, escape, c, nextc);
      renderer->begin_line = false;
      renderer->begin_content =
          renderer->begin_content && cmark_isdigit(c) == 1;
    }

    // We don't allow breaks that would put something that can start a
    // block at the beginning of a line, because this causes problems with
    // commonmark output. The text after a break may come from a later
    // node than the space, so check what has been output since it
    // (and at what is still to come from this one, if that isn't enough):
    breakable = 0;
    if (renderer->last_breakable > 0) {
      n = 0;
      for (j = renderer->last_breakable + 1;
           j < renderer->buffer->size && n < (int)sizeof(peek); j++) {
        peek[n++] = renderer->buffer->ptr[j];
      }
      for (j = i + len; j < length && n < (int)sizeof(peek); j++) {
        peek[n++] = (unsigned char)source[j];
      }
      breakable = S_bad_line_start(peek, n);
      if (breakable == 1) {
        renderer->last_breakable = 0;
      }
    }

    // If adding the character went beyond width, look for an
    // earlier place where the line could be broken (once we know
    // what the new line would start with):
    if (renderer->width > 0 && renderer->column > renderer->width &&
        !renderer->begin_line && renderer->last_breakable > 0 &&
        breakable == 0) {

      // copy from last_breakable to remainder
      unsigned char *src = renderer->buffer->ptr +
                           renderer->last_breakable + 1;
      bufsize_t remainder_len = renderer->buffer->size -
                                renderer->last_breakable - 1;
      unsigned char *remainder =
          (unsigned char *)renderer->mem->realloc(NULL, remainder_len);
      memcpy(remainder, src, remainder_len);
      // truncate at last_breakable
      cmark_strbuf_truncate(renderer->buffer, renderer->last_breakable);
      // add newline, prefix, and remainder
      cmark_strbuf_putc(renderer->buffer, '\n');
      cmark_strbuf_put(renderer->buffer, renderer->prefix->ptr,
                       renderer->prefix->size);
      cmark_strbuf_put(renderer->buffer, remainder, remainder_len);
      renderer->column = renderer->prefix->size + remainder_len;
      renderer->mem->free(remainder);
      renderer->last_breakable = 0;
      renderer->begin_line = false;
      renderer->begin_content = false;
    }

    i += len;
  }
}

// Assumes no newlines, assumes ascii content:
void cmark_render_ascii(cmark_renderer *renderer, const char *s) {
  int origsize = renderer->buffer->size;
  cmark_strbuf_puts(renderer->buffer, s);
  renderer->column += renderer->buffer->size - origsize;
}

void cmark_render_code_point(cmark_renderer *renderer, uint32_t c) {
  cmark_utf8proc_encode_char(c, renderer->buffer);
  renderer->column += 1;
}

char *cmark_render(cmark_node *root, int options, int width,
                   void (*outc)(cmark_renderer *, cmark_escaping, int32_t,
                                unsigned char),
                   int (*render_node)(cmark_renderer *renderer,
                                      cmark_node *node,
                                      cmark_event_type ev_type, int options)) {
  cmark_mem *mem = root->mem;
  cmark_strbuf pref = CMARK_BUF_INIT(mem);
  cmark_strbuf buf = CMARK_BUF_INIT(mem);
  cmark_node *cur;
  cmark_event_type ev_type;
  char *result;
  cmark_iter *iter = cmark_iter_new(root);

  cmark_renderer renderer = {options,
                             mem,    &buf,    &pref,      0,      width,
                             0,      0,       true,       true,   false,
                             false,  NULL,
                             outc,   S_cr,    S_blankline, S_out};

  while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
    cur = cmark_iter_get_node(iter);
    if (!render_node(&renderer, cur, ev_type, options)) {
      // a false value causes us to skip processing
      // the node's contents.  this is used for
      // autolinks.
      cmark_iter_reset(iter, cur, CMARK_EVENT_EXIT);
    }
  }

  // If the root node is a block type (i.e. not inline), ensure there's a final newline:
  if (cmark_node_is_block(root)) {
    if (renderer.buffer->size == 0 || renderer.buffer->ptr[renderer.buffer->size - 1] != '\n') {
      cmark_strbuf_putc(renderer.buffer, '\n');
    }
  }

  result = (char *)cmark_strbuf_detach(renderer.buffer);

  cmark_iter_free(iter);
  cmark_strbuf_free(renderer.prefix);
  cmark_strbuf_free(renderer.buffer);

  return result;
}
