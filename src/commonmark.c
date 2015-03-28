#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "config.h"
#include "cmark.h"
#include "node.h"
#include "buffer.h"
#include "utf8.h"

// Functions to convert cmark_nodes to commonmark strings.

struct render_state {
	int options;
	cmark_strbuf* buffer;
	cmark_strbuf* prefix;
	int column;
	int width;
	int need_cr;
	int last_breakable;
	bool begin_line;
	bool no_wrap;
	bool in_tight_list_item;
};

static inline void cr(struct render_state *state)
{
	if (state->need_cr < 1) {
		state->need_cr = 1;
	}
}

static inline void blankline(struct render_state *state)
{
	if (state->need_cr < 2) {
		state->need_cr = 2;
	}
}

static inline bool
needs_escaping(int32_t c, unsigned char next_c, struct render_state *state)
{
	return (c == '*' || c == '_' || c == '[' || c == ']' ||
		c == '<' || c == '>' || c == '\\' ||
		(c == '&' && isalpha(next_c)) ||
		(c == '!' && next_c == '[') ||
		(state->begin_line &&
		 (c == '-' || c == '+' || c == '#' || c == '=')) ||
		((c == '.' || c == ')') &&
		 isdigit(state->buffer->ptr[state->buffer->size - 1])));
}

static inline void out(struct render_state *state,
		       cmark_chunk str,
		       bool wrap,
		       bool escape)
{
	unsigned char* source = str.data;
	int length = str.len;
	unsigned char nextc;
	int32_t c;
	int i = 0;
	int len;
	cmark_chunk remainder = cmark_chunk_literal("");
	int k = state->buffer->size - 1;

	wrap = wrap && !state->no_wrap;

	if (state->in_tight_list_item && state->need_cr > 1) {
		state->need_cr = 1;
	}
	while (state->need_cr) {
		if (k < 0 || state->buffer->ptr[k] == '\n') {
			k -= 1;
		} else {
			cmark_strbuf_putc(state->buffer, '\n');
			if (state->need_cr > 1) {
				cmark_strbuf_put(state->buffer, state->prefix->ptr,
						 state->prefix->size);
			}
		}
		state->column = 0;
		state->begin_line = true;
		state->need_cr -= 1;
	}

	while (i < length) {
		if (state->begin_line) {
			cmark_strbuf_put(state->buffer, state->prefix->ptr,
					 state->prefix->size);
			// note: this assumes prefix is ascii:
			state->column = state->prefix->size;
		}

		len = utf8proc_iterate(source + i, length - i, &c);
		nextc = source[i + len];
		if (c == 32 && wrap) {
			if (!state->begin_line) {
				cmark_strbuf_putc(state->buffer, ' ');
				state->column += 1;
				state->begin_line = false;
				state->last_breakable = state->buffer->size -
					1;
				// skip following spaces
				while (source[i + 1] == ' ') {
					i++;
				}
			}

		} else if (c == 10) {
			cmark_strbuf_putc(state->buffer, '\n');
			state->column = 0;
			state->begin_line = true;
			state->last_breakable = 0;
		} else if (escape &&
			   needs_escaping(c, nextc, state)) {
			cmark_strbuf_putc(state->buffer, '\\');
			utf8proc_encode_char(c, state->buffer);
			state->column += 2;
			state->begin_line = false;
		} else {
			utf8proc_encode_char(c, state->buffer);
			state->column += 1;
			state->begin_line = false;
		}

		// If adding the character went beyond width, look for an
		// earlier place where the line could be broken:
		if (state->width > 0 &&
		    state->column > state->width &&
		    !state->begin_line &&
		    state->last_breakable > 0) {

			// copy from last_breakable to remainder
			cmark_chunk_set_cstr(&remainder, (char *) state->buffer->ptr + state->last_breakable + 1);
			// truncate at last_breakable
			cmark_strbuf_truncate(state->buffer, state->last_breakable);
			// add newline, prefix, and remainder
			cmark_strbuf_putc(state->buffer, '\n');
			cmark_strbuf_put(state->buffer, state->prefix->ptr,
					 state->prefix->size);
			cmark_strbuf_put(state->buffer, remainder.data, remainder.len);
			state->column = state->prefix->size + remainder.len;
			cmark_chunk_free(&remainder);
			state->last_breakable = 0;
			state->begin_line = false;
		}

		i += len;
	}
}

static void lit(struct render_state *state, char *s, bool wrap)
{
	cmark_chunk str = cmark_chunk_literal(s);
	out(state, str, wrap, false);
}

static int
longest_backtick_sequence(cmark_chunk *code)
{
	int longest = 0;
	int current = 0;
	int i = 0;
	while (i < code->len) {
		if (code->data[i] == '`') {
			current++;
		} else {
			if (current > longest) {
				longest = current;
			}
			current = 0;
		}
		i++;
	}
	return longest;
}

static int
S_render_node(cmark_node *node, cmark_event_type ev_type,
              struct render_state *state)
{
	cmark_node *tmp;
	int list_number;
	cmark_delim_type list_delim;
	int numticks;
	int i;
	bool entering = (ev_type == CMARK_EVENT_ENTER);
	const char *info;
	const char *title;
	char listmarker[64];
	int marker_width;

	state->in_tight_list_item =
		node->type == CMARK_NODE_ITEM &&
		cmark_node_get_list_tight(node->parent);

	switch (node->type) {
	case CMARK_NODE_DOCUMENT:
		if (!entering) {
			cmark_strbuf_putc(state->buffer, '\n');
		}
		break;

	case CMARK_NODE_BLOCK_QUOTE:
		if (entering) {
			lit(state, "> ", false);
			cmark_strbuf_puts(state->prefix, "> ");
		} else {
			cmark_strbuf_truncate(state->prefix,
					      state->prefix->size - 2);
			blankline(state);
		}
		break;

	case CMARK_NODE_LIST:
		break;

	case CMARK_NODE_ITEM:
		if (cmark_node_get_list_type(node->parent) ==
		    CMARK_BULLET_LIST) {
			marker_width = 2;
		} else {
			list_number = cmark_node_get_list_start(node->parent);
			list_delim = cmark_node_get_list_delim(node->parent);
			tmp = node;
			while (tmp->prev) {
				tmp = tmp->prev;
				list_number += 1;
			}
			// we ensure a width of at least 4 so
			// we get nice transition from single digits
			// to double
			snprintf(listmarker, 63, "%d%s%s", list_number,
				 list_delim == CMARK_PAREN_DELIM ?
				 ")" : ".",
				 list_number < 10 ? "  " : " ");
			marker_width = strlen(listmarker);
		}
		if (entering) {
			if (cmark_node_get_list_type(node->parent) ==
			    CMARK_BULLET_LIST) {
				lit(state, "- ", false);
				cmark_strbuf_puts(state->prefix, "  ");
			} else {
				lit(state, listmarker, false);
				for (i=marker_width; i--;) {
					cmark_strbuf_putc(state->prefix, ' ');
				}
			}
		} else {
			cmark_strbuf_truncate(state->prefix,
					      state->prefix->size -
					      marker_width);
			cr(state);
		}
		break;

	case CMARK_NODE_HEADER:
		if (entering) {
			for (int i = cmark_node_get_header_level(node); i > 0; i--) {
				lit(state, "#", false);
			}
			lit(state, " ", false);
			state->no_wrap = true;
		} else {
			state->no_wrap = false;
			blankline(state);
		}
		break;

	case CMARK_NODE_CODE_BLOCK:
		blankline(state);
		info = cmark_node_get_fence_info(node);
		if (info == NULL || strlen(info) == 0) {
			// use indented form if no info
			lit(state, "    ", false);
			cmark_strbuf_puts(state->prefix, "    ");
			out(state, node->as.code.literal, false, false);
			cmark_strbuf_truncate(state->prefix,
					      state->prefix->size - 4);
		} else {
			numticks = longest_backtick_sequence(&node->as.code.literal)
				+ 1;
			if (numticks < 3) {
				numticks = 3;
			}
			for (i = 0; i < numticks; i++) {
				lit(state, "`", false);
			}
			lit(state, " ", false);
			out(state, cmark_chunk_literal(info), false, false);
			cr(state);
			out(state, node->as.code.literal, false, true);
			cr(state);
			for (i = 0; i < numticks; i++) {
				lit(state, "`", false);
			}
		}
		blankline(state);
		break;

	case CMARK_NODE_HTML:
		blankline(state);
		out(state, node->as.literal, false, false);
		blankline(state);
		break;

	case CMARK_NODE_HRULE:
		blankline(state);
		lit(state, "-----", false);
		blankline(state);
		break;

	case CMARK_NODE_PARAGRAPH:
		if (!entering) {
			blankline(state);
		}
		break;

	case CMARK_NODE_TEXT:
		out(state, node->as.literal, true, true);
		break;

	case CMARK_NODE_LINEBREAK:
		if (!(CMARK_OPT_HARDBREAKS & state->options)) {
			lit(state, "\\", false);
		}
		cr(state);
		break;

	case CMARK_NODE_SOFTBREAK:
		lit(state, " ", true);
		break;

	case CMARK_NODE_CODE:
		numticks = longest_backtick_sequence(&node->as.literal) + 1;
		for (i = 0; i < numticks; i++) {
			lit(state, "`", false);
		}
		if (numticks > 1) {
			lit(state, " ", false);
		}
		out(state, node->as.literal, true, false);
		if (numticks > 1) {
			lit(state, " ", false);
		}
		for (i = 0; i < numticks; i++) {
			lit(state, "`", false);
		}
		break;

	case CMARK_NODE_INLINE_HTML:
		out(state, node->as.literal, true, false);
		break;

	case CMARK_NODE_STRONG:
		if (entering) {
			lit(state, "**", false);
		} else {
			lit(state, "**", false);
		}
		break;

	case CMARK_NODE_EMPH:
		if (entering) {
			lit(state, "*", false);
		} else {
			lit(state, "*", false);
		}
		break;

	case CMARK_NODE_LINK:
		if (entering) {
			lit(state, "[", false);
		} else {
			// TODO - backslash-escape " and \ inside url, title
			// for both links and images
			lit(state, "](", false);
			out(state, cmark_chunk_literal(cmark_node_get_url(node)), false, true);
			title = cmark_node_get_title(node);
			if (title && strlen(title) > 0) {
				lit(state, " \"", true);
				out(state, cmark_chunk_literal(title), false, true);
				lit(state, "\"", false);
			}
			lit(state, ")", false);
		}
		break;

	case CMARK_NODE_IMAGE:
		if (entering) {
			lit(state, "![", false);
		} else {
			lit(state, "](", false);
			out(state, cmark_chunk_literal(cmark_node_get_url(node)), false, true);
			title = cmark_node_get_title(node);
			if (title && strlen(title) > 0) {
				lit(state, " \"", true);
				out(state, cmark_chunk_literal(title), false, true);
				lit(state, "\"", false);
			}
			lit(state, ")", false);
		}
		break;

	default:
		assert(false);
		break;
	}

	return 1;
}

char *cmark_render_commonmark(cmark_node *root, int options, int width)
{
	char *result;
	cmark_strbuf commonmark = GH_BUF_INIT;
	cmark_strbuf prefix = GH_BUF_INIT;
	if (CMARK_OPT_HARDBREAKS & options) {
		width = 0;
	}
	struct render_state state =
		{ options, &commonmark, &prefix, 0, width,
		  0, 0, true, false, false};
	cmark_node *cur;
	cmark_event_type ev_type;
	cmark_iter *iter = cmark_iter_new(root);

	while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
		cur = cmark_iter_get_node(iter);
		S_render_node(cur, ev_type, &state);
	}
	result = (char *)cmark_strbuf_detach(&commonmark);

	cmark_strbuf_free(&prefix);
	cmark_iter_free(iter);
	return result;
}
