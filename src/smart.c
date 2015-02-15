#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "config.h"
#include "cmark.h"
#include "node.h"
#include "utf8.h"
#include "buffer.h"
#include "chunk.h"

static const char SMART_PUNCT_TABLE[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

void escape_with_smart(cmark_strbuf *buf,
		       cmark_node *node,
		       void (*escape)(cmark_strbuf *, const unsigned char *, int),
		       const char *left_double_quote,
		       const char *right_double_quote,
		       const char *left_single_quote,
		       const char *right_single_quote,
		       const char *em_dash,
		       const char *en_dash,
		       const char *ellipses)
{
	char c;
	int32_t after_char = 0;
	int32_t before_char = 0;
	bool left_flanking, right_flanking;
	int lastout = 0;
	int i = 0, j = 0;
	cmark_chunk lit = node->as.literal;
	int len;

	while (i < lit.len) {
		c = lit.data[i];
		i++;
		if (SMART_PUNCT_TABLE[(int)c] == 0) {
			continue;
		}

		if (i - 1 - lastout > 0) {
			(*escape)(buf, lit.data + lastout, i - 1 - lastout);
		}

		if (c == 34 || c == 39) {
			if (i == 1) {
                                // set before_char based on previous text node if there is one:
				if (node->prev) {
					if (node->prev->type == CMARK_NODE_TEXT) {

						// walk to the beginning of the UTF_8 sequence:
						j = node->prev->as.literal.len - 1;
						while (j > 0 &&
						       node->prev->as.literal.data[j] >> 6 == 2) {
							j--;
						}
						len = utf8proc_iterate(node->prev->as.literal.data + i,
								       node->prev->as.literal.len - i,
								       &before_char);
						if (len == -1) {
							before_char = 10;
						}

					} else if (node->prev->type == CMARK_NODE_SOFTBREAK ||
						   node->prev->type == CMARK_NODE_LINEBREAK) {
						before_char = 10;

					} else {
						before_char = 65;
					}
				} else {
					before_char = 10;
				}
			} else {
				j = i - 2;
				// walk back to the beginning of the UTF_8 sequence:
				while (j > 0 && lit.data[j] >> 6 == 2) {
					j--;
				}
				utf8proc_iterate(lit.data + j, lit.len - j, &before_char);
			}

			if (i >= lit.len) {
				if (node->next) {
					if (node->next->type == CMARK_NODE_TEXT) {
						utf8proc_iterate(node->next->as.literal.data,
								 node->next->as.literal.len,
								 &after_char);
					} else if (node->next->type == CMARK_NODE_SOFTBREAK ||
						   node->next->type == CMARK_NODE_LINEBREAK) {
						after_char = 10;
					} else {
						after_char = 65;
					}
				} else {
					after_char = 10;
				}
			} else {
				utf8proc_iterate(lit.data + i, lit.len - i, &after_char);
			}

			left_flanking = !utf8proc_is_space(after_char) &&
				!(utf8proc_is_punctuation(after_char) &&
				  !utf8proc_is_space(before_char) &&
				  !utf8proc_is_punctuation(before_char));
			right_flanking = !utf8proc_is_space(before_char) &&
				!(utf8proc_is_punctuation(before_char) &&
				  !utf8proc_is_space(after_char) &&
				  !utf8proc_is_punctuation(after_char));
		}

		switch (c) {
		case '"':
			if (right_flanking) {
				cmark_strbuf_puts(buf, right_double_quote);
			} else {
				cmark_strbuf_puts(buf, left_double_quote);
			}
			break;
		case '\'':
			if (left_flanking && !right_flanking) {
				cmark_strbuf_puts(buf, left_single_quote);
			} else {
				cmark_strbuf_puts(buf, right_single_quote);
			}
			break;
		case '-':
			if (i < lit.len && lit.data[i] == '-') {
				if (lit.data[i + 1] == '-') {
					cmark_strbuf_puts(buf, em_dash);
					i += 2;
				} else {
					cmark_strbuf_puts(buf, en_dash);
					i += 1;
				}
			} else {
				cmark_strbuf_putc(buf, c);
			}
			break;
		case '.':
			if (i < lit.len - 1 && lit.data[i] == '.' &&
			    lit.data[i + 1] == '.') {
				cmark_strbuf_puts(buf, ellipses);
				i += 2;
			} else {
				cmark_strbuf_putc(buf, c);
			}
			break;
		default:
			cmark_strbuf_putc(buf, c);
		}
		lastout = i;
	}
	(*escape)(buf, node->as.literal.data + lastout, lit.len - lastout);

}
