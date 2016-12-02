#include <stdbool.h>
#include "entities.inc"
#include "utf8.h"
#include "chunk.h"
#include "buffer.h"
#include "cmark_ctype.h"

#define MAX_ENTITY_LENGTH 47

#ifndef MIN
#define MIN(x, y) ((x < y) ? x : y)
#endif

int scan_entity(cmark_chunk *chunk, bufsize_t offset) {
  bufsize_t limit = MIN(offset + MAX_ENTITY_LENGTH, chunk->len);
  bufsize_t i = offset;
  const unsigned char *data = chunk->data;
  bool numerical = false;
  bool hex = false;

  if (data == NULL || offset > chunk->len) {
    return 0;
  }

  if (data[i] == '&') {
    i++;
  } else {
    return 0;
  }

  if (data[i] == '#') { // numerical
    i++;
    numerical = true;
    if (data[i+1] == 'X' || data[i+1] == 'x') {
      i++;
      hex = true;
    }
  } else {
    if (cmark_isalpha(data[i])) {
      i++;
    } else {
      return 0;
    }
  }

  while (i <= limit && data[i]) {
    if (numerical) {
      if (!((hex && cmark_ishexdigit(data[i])) || cmark_isdigit(data[i]))) {
	return 0;
      }
    } else {
      if (!(cmark_isalnum(data[i]))) {
	return 0;
      }
    }
    i++;
  }

  if (data[i] == ';') {
    return (i - offset);
  }

  return 0;

}

/* Binary tree lookup code for entities added by JGM */

static const unsigned char *S_lookup(int i, int low, int hi,
                                     const unsigned char *s, int len) {
  int j;
  int cmp =
      strncmp((const char *)s, (const char *)cmark_entities[i].entity, len);
  if (cmp == 0 && cmark_entities[i].entity[len] == 0) {
    return (const unsigned char *)cmark_entities[i].bytes;
  } else if (cmp <= 0 && i > low) {
    j = i - ((i - low) / 2);
    if (j == i)
      j -= 1;
    return S_lookup(j, low, i - 1, s, len);
  } else if (cmp > 0 && i < hi) {
    j = i + ((hi - i) / 2);
    if (j == i)
      j += 1;
    return S_lookup(j, i + 1, hi, s, len);
  } else {
    return NULL;
  }
}

const unsigned char *cmark_lookup_entity(const unsigned char *s, int len) {
  return S_lookup(CMARK_NUM_ENTITIES / 2, 0, CMARK_NUM_ENTITIES - 1, s, len);
}

bufsize_t houdini_unescape_ent(cmark_strbuf *ob, const uint8_t *src,
                               bufsize_t size) {
  bufsize_t i = 0;

  if (size >= 3 && src[0] == '#') {
    int codepoint = 0;
    int num_digits = 0;

    if (cmark_isdigit(src[1])) {
      for (i = 1; i < size && cmark_isdigit(src[i]); ++i) {
        codepoint = (codepoint * 10) + (src[i] - '0');

        if (codepoint >= 0x110000) {
          // Keep counting digits but
          // avoid integer overflow.
          codepoint = 0x110000;
        }
      }

      num_digits = i - 1;
    }

    else if (src[1] == 'x' || src[1] == 'X') {
      for (i = 2; i < size && cmark_ishexdigit(src[i]); ++i) {
        codepoint = (codepoint * 16) + ((src[i] | 32) % 39 - 9);

        if (codepoint >= 0x110000) {
          // Keep counting digits but
          // avoid integer overflow.
          codepoint = 0x110000;
        }
      }

      num_digits = i - 2;
    }

    if (num_digits >= 1 && num_digits <= 8 && i < size && src[i] == ';') {
      if (codepoint == 0 || (codepoint >= 0xD800 && codepoint < 0xE000) ||
          codepoint >= 0x110000) {
        codepoint = 0xFFFD;
      }
      cmark_utf8proc_encode_char(codepoint, ob);
      return i + 1;
    }
  }

  else {
    if (size > CMARK_ENTITY_MAX_LENGTH)
      size = CMARK_ENTITY_MAX_LENGTH;

    for (i = CMARK_ENTITY_MIN_LENGTH; i < size; ++i) {
      if (src[i] == ' ')
        break;

      if (src[i] == ';') {
        const unsigned char *entity = cmark_lookup_entity(src, i);

        if (entity != NULL) {
          cmark_strbuf_puts(ob, (const char *)entity);
          return i + 1;
        }

        break;
      }
    }
  }

  return 0;
}
