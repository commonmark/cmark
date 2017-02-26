#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "buffer.h"
#include "chunk.h"
#include "houdini.h"
#include "utf8.h"
#include "entity.h"

bufsize_t houdini_unescape_ent(cmark_strbuf *ob, const uint8_t *src,
                               bufsize_t size, int convert_entities) {
  int ent_length;
  int hex = 0;
  bufsize_t i = 0;
  int codepoint = 0;
  cmark_chunk chunk = {(unsigned char *)src, size, 0};

  ent_length = scan_entity(&chunk, 0);

  if (ent_length == 0) {

    return 0;

  } else if (!convert_entities) {

    cmark_strbuf_put(ob, (const unsigned char *)src, ent_length);
    return ent_length;

  } else if (src[1] == '#') {

    if (src[2] == 'x' || src[2] == 'X') {
      i = 3;
      hex = 1;
    } else {
      i = 2;
      hex = 0;
    }

    while (i < ent_length) {
      if (hex) {
        codepoint = (codepoint * 16) + ((src[i] | 32) % 39 - 9);
      } else {
        codepoint = (codepoint * 10) + (src[i] - '0');
      }
      if (codepoint >= 0x110000) {
        // Keep counting digits but
        // avoid integer overflow.
        codepoint = 0x110000;
      }
      i++;
    }

    if (codepoint == 0 || (codepoint >= 0xD800 && codepoint < 0xE000) ||
        codepoint >= 0x110000) {
      codepoint = 0xFFFD;
    }

    cmark_utf8proc_encode_char(codepoint, ob);
    return ent_length;

  } else if (ent_length > 3) {
    const unsigned char *entity = cmark_lookup_entity(src + 1, ent_length - 2);

    if (entity == NULL) {
      return 0;
    } else {
      cmark_strbuf_puts(ob, (const char *)entity);
      return ent_length;
    }

  }

  return 0;
}


int houdini_unescape_html(cmark_strbuf *ob, const uint8_t *src,
                          bufsize_t size) {
  bufsize_t i = 0, org, ent;

  while (i < size) {
    org = i;
    while (i < size && src[i] != '&')
      i++;

    if (likely(i > org)) {
      if (unlikely(org == 0)) {
        if (i >= size)
          return 0;

        cmark_strbuf_grow(ob, HOUDINI_UNESCAPED_SIZE(size));
      }

      cmark_strbuf_put(ob, src + org, i - org);
    }

    /* escaping */
    if (i >= size)
      break;

    ent = houdini_unescape_ent(ob, src + i, size - i, 0);
    i += ent;

    /* not really an entity */
    if (ent == 0) {
      i++;
      cmark_strbuf_putc(ob, '&');
    }
  }

  return 1;
}

void houdini_unescape_html_f(cmark_strbuf *ob, const uint8_t *src,
                             bufsize_t size) {
  if (!houdini_unescape_html(ob, src, size))
    cmark_strbuf_put(ob, src, size);
}
