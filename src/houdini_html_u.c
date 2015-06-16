#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "buffer.h"
#include "houdini.h"
#include "utf8.h"
#include "entities.h"

/* Binary tree lookup code for entities added by JGM */

static unsigned long
S_hash(const unsigned char *str, int len)
{
	unsigned long hash = 5381;
	int i;

	for (i = 0; i < len; i++) {
		hash = (((hash << 5) + hash) + str[i]) & 0xFFFFFFFF; /* hash * 33 + c */
	}

	return hash;
}

static unsigned char *
S_lookup(int i, unsigned long key)
{
	if (cmark_entities[i].value == key) {
		return cmark_entities[i].bytes;
	} else {
		int next = key < cmark_entities[i].value ?
		           cmark_entities[i].less : cmark_entities[i].greater;
		if (next == -1) { // leaf node
			return NULL;
		} else {
			return S_lookup(next, key);
		}
	}
}

static unsigned char *
S_lookup_entity(const unsigned char *s, int len)
{
	return S_lookup(cmark_entities_root, S_hash(s, len));
}

bufsize_t
houdini_unescape_ent(cmark_strbuf *ob, const uint8_t *src, bufsize_t size)
{
	bufsize_t i = 0;

	if (size >= 3 && src[0] == '#') {
		int codepoint  = 0;
		int num_digits = 0;

		if (_isdigit(src[1])) {
			for (i = 1; i < size && _isdigit(src[i]); ++i) {
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
			for (i = 2; i < size && _isxdigit(src[i]); ++i) {
				codepoint = (codepoint * 16) + ((src[i] | 32) % 39 - 9);

				if (codepoint >= 0x110000) {
					// Keep counting digits but
					// avoid integer overflow.
					codepoint = 0x110000;
				}
			}

			num_digits = i - 2;
		}

		if (num_digits >= 1 && num_digits <= 8 &&
		    i < size && src[i] == ';') {
			if (codepoint == 0 ||
			    (codepoint >= 0xD800 && codepoint < 0xE000) ||
			    codepoint >= 0x110000) {
				codepoint = 0xFFFD;
			}
			utf8proc_encode_char(codepoint, ob);
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
				const unsigned char *entity = S_lookup_entity(src, i);

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

int
houdini_unescape_html(cmark_strbuf *ob, const uint8_t *src, bufsize_t size)
{
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

		i++;

		ent = houdini_unescape_ent(ob, src + i, size - i);
		i += ent;

		/* not really an entity */
		if (ent == 0)
			cmark_strbuf_putc(ob, '&');
	}

	return 1;
}

void houdini_unescape_html_f(cmark_strbuf *ob, const uint8_t *src, bufsize_t size)
{
	if (!houdini_unescape_html(ob, src, size))
		cmark_strbuf_put(ob, src, size);
}
