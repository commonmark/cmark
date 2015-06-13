#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "buffer.h"
#include "houdini.h"
#include "utf8.h"
#include "html_unescape.h"

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
		if (size > MAX_WORD_LENGTH)
			size = MAX_WORD_LENGTH;

		for (i = MIN_WORD_LENGTH; i < size; ++i) {
			if (src[i] == ' ')
				break;

			if (src[i] == ';') {
				const struct html_ent *entity = find_entity((char *)src, i);

				if (entity != NULL) {
					bufsize_t len = 0;
					while (len < 8 && entity->utf8[len] != '\0') {
						++len;
					}
					cmark_strbuf_put(ob, entity->utf8, len);
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
