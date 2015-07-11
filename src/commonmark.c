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
#include "scanners.h"
#include "render.h"

// Functions to convert cmark_nodes to commonmark strings.

static inline void outc(cmark_render_state *state,
			cmark_escaping escape,
			int32_t c,
			unsigned char nextc)
{
	bool needs_escaping = false;
	needs_escaping =
		escape != LITERAL &&
		((escape == NORMAL &&
		  (c == '*' || c == '_' || c == '[' || c == ']' ||
		   c == '<' || c == '>' || c == '\\' || c == '`' ||
		   (c == '&' && isalpha(nextc)) ||
		   (c == '!' && nextc == '[') ||
		   (state->begin_line &&
		    (c == '-' || c == '+' || c == '#' || c == '=')) ||
		   ((c == '.' || c == ')') &&
		    isdigit(state->buffer->ptr[state->buffer->size - 1])))) ||
		 (escape == URL &&
		  (c == '`' || c == '<' || c == '>' || isspace(c) ||
		        c == '\\' || c == ')' || c == '(')) ||
		 (escape == TITLE &&
		  (c == '`' || c == '<' || c == '>' || c == '"' ||
		        c == '\\')));

	if (needs_escaping) {
		if (isspace(c)) {
			// use percent encoding for spaces
			cmark_strbuf_printf(state->buffer, "%%%2x", c);
			state->column += 3;
		} else {
			cmark_strbuf_putc(state->buffer, '\\');
			utf8proc_encode_char(c, state->buffer);
			state->column += 2;
		}
		state->begin_line = false;
	} else {
		utf8proc_encode_char(c, state->buffer);
		state->column += 1;
		state->begin_line = false;
	}

}

static int
longest_backtick_sequence(cmark_chunk *code)
{
	int longest = 0;
	int current = 0;
	int i = 0;
	while (i <= code->len) {
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
shortest_unused_backtick_sequence(cmark_chunk *code)
{
	int32_t used = 1;
	int current = 0;
	int i = 0;
	while (i <= code->len) {
		if (code->data[i] == '`') {
			current++;
		} else {
			if (current) {
				used |= (1 << current);
			}
			current = 0;
		}
		i++;
	}
	// return number of first bit that is 0:
	i = 0;
	while (used & 1) {
		used = used >> 1;
		i++;
	}
	return i;
}

static bool
is_autolink(cmark_node *node)
{
	cmark_chunk *title;
	cmark_chunk *url;
	cmark_node *link_text;
	char *realurl;
	int realurllen;

	if (node->type != CMARK_NODE_LINK) {
		return false;
	}

	url = &node->as.link.url;
	if (url->len == 0 || scan_scheme(url, 0) == 0) {
		return false;
	}

	title = &node->as.link.title;
	// if it has a title, we can't treat it as an autolink:
	if (title->len > 0) {
		return false;
	}

	link_text = node->first_child;
	cmark_consolidate_text_nodes(link_text);
	realurl = (char*)url->data;
	realurllen = url->len;
	if (strncmp(realurl, "mailto:", 7) == 0) {
		realurl += 7;
		realurllen -= 7;
	}
	return (realurllen == link_text->as.literal.len &&
	        strncmp(realurl,
	                (char*)link_text->as.literal.data,
	                link_text->as.literal.len) == 0);
}

// if node is a block node, returns node.
// otherwise returns first block-level node that is an ancestor of node.
static cmark_node*
get_containing_block(cmark_node *node)
{
	while (node &&
	       (node->type < CMARK_NODE_FIRST_BLOCK ||
	        node->type > CMARK_NODE_LAST_BLOCK)) {
		node = node->parent;
	}
	return node;
}

static int
S_render_node(cmark_node *node, cmark_event_type ev_type,
              cmark_render_state *state)
{
	cmark_node *tmp;
	cmark_chunk *code;
	int list_number;
	cmark_delim_type list_delim;
	int numticks;
	int i;
	bool entering = (ev_type == CMARK_EVENT_ENTER);
	cmark_chunk *info;
	cmark_chunk *title;
	cmark_strbuf listmarker = GH_BUF_INIT;
	char *emph_delim;
	bufsize_t marker_width;

	// Don't adjust tight list status til we've started the list.
	// Otherwise we loose the blank line between a paragraph and
	// a following list.
	if (!(node->type == CMARK_NODE_ITEM && node->prev == NULL &&
	      entering)) {
		tmp = get_containing_block(node);
		state->in_tight_list_item =
		    (tmp->type == CMARK_NODE_ITEM &&
		     cmark_node_get_list_tight(tmp->parent)) ||
		    (tmp &&
		     tmp->parent &&
		     tmp->parent->type == CMARK_NODE_ITEM &&
		     cmark_node_get_list_tight(tmp->parent->parent));
	}

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
		if (!entering && node->next &&
		    (node->next->type == CMARK_NODE_CODE_BLOCK ||
		     node->next->type == CMARK_NODE_LIST)) {
			// this ensures 2 blank lines after list,
			// if before code block or list:
			lit(state, "\n", false);
			state->need_cr = 0;
		}
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
			cmark_strbuf_printf(&listmarker,
			                    "%d%s%s", list_number,
			                    list_delim == CMARK_PAREN_DELIM ?
			                    ")" : ".",
			                    list_number < 10 ? "  " : " ");
			marker_width = listmarker.size;
		}
		if (entering) {
			if (cmark_node_get_list_type(node->parent) ==
			    CMARK_BULLET_LIST) {
				lit(state, "* ", false);
				cmark_strbuf_puts(state->prefix, "  ");
			} else {
				lit(state, (char *)listmarker.ptr, false);
				for (i = marker_width; i--;) {
					cmark_strbuf_putc(state->prefix, ' ');
				}
			}
		} else {
			cmark_strbuf_truncate(state->prefix,
			                      state->prefix->size -
			                      marker_width);
			cr(state);
		}
		cmark_strbuf_free(&listmarker);
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
		info = &node->as.code.info;
		code = &node->as.code.literal;
		// use indented form if no info, and code doesn't
		// begin or end with a blank line, and code isn't
		// first thing in a list item
		if (info->len == 0 &&
		    (code->len > 2 &&
		     !isspace(code->data[0]) &&
		     !(isspace(code->data[code->len - 1]) &&
		       isspace(code->data[code->len - 2]))) &&
		    !(node->prev == NULL && node->parent &&
		      node->parent->type == CMARK_NODE_ITEM)) {
			lit(state, "    ", false);
			cmark_strbuf_puts(state->prefix, "    ");
			out(state, node->as.code.literal, false, LITERAL);
			cmark_strbuf_truncate(state->prefix,
			                      state->prefix->size - 4);
		} else {
			numticks = longest_backtick_sequence(code) + 1;
			if (numticks < 3) {
				numticks = 3;
			}
			for (i = 0; i < numticks; i++) {
				lit(state, "`", false);
			}
			lit(state, " ", false);
			out(state, *info, false, LITERAL);
			cr(state);
			out(state, node->as.code.literal, false, LITERAL);
			cr(state);
			for (i = 0; i < numticks; i++) {
				lit(state, "`", false);
			}
		}
		blankline(state);
		break;

	case CMARK_NODE_HTML:
		blankline(state);
		out(state, node->as.literal, false, LITERAL);
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
		out(state, node->as.literal, true, NORMAL);
		break;

	case CMARK_NODE_LINEBREAK:
		if (!(CMARK_OPT_HARDBREAKS & state->options)) {
			lit(state, "\\", false);
		}
		cr(state);
		break;

	case CMARK_NODE_SOFTBREAK:
		if (state->width == 0) {
			cr(state);
		} else {
			lit(state, " ", true);
		}
		break;

	case CMARK_NODE_CODE:
		code = &node->as.literal;
		numticks = shortest_unused_backtick_sequence(code);
		for (i = 0; i < numticks; i++) {
			lit(state, "`", false);
		}
		if (code->len == 0 || code->data[0] == '`') {
			lit(state, " ", false);
		}
		out(state, node->as.literal, true, LITERAL);
		if (code->len == 0 || code->data[code->len - 1] == '`') {
			lit(state, " ", false);
		}
		for (i = 0; i < numticks; i++) {
			lit(state, "`", false);
		}
		break;

	case CMARK_NODE_INLINE_HTML:
		out(state, node->as.literal, false, LITERAL);
		break;

	case CMARK_NODE_STRONG:
		if (entering) {
			lit(state, "**", false);
		} else {
			lit(state, "**", false);
		}
		break;

	case CMARK_NODE_EMPH:
		// If we have EMPH(EMPH(x)), we need to use *_x_*
		// because **x** is STRONG(x):
		if (node->parent && node->parent->type == CMARK_NODE_EMPH &&
		    node->next == NULL && node->prev == NULL) {
			emph_delim = "_";
		} else {
			emph_delim = "*";
		}
		if (entering) {
			lit(state, emph_delim, false);
		} else {
			lit(state, emph_delim, false);
		}
		break;

	case CMARK_NODE_LINK:
		if (is_autolink(node)) {
			if (entering) {
				lit(state, "<", false);
				if (strncmp(cmark_node_get_url(node),
				            "mailto:", 7) == 0) {
					lit(state,
					    (char *)cmark_node_get_url(node) + 7,
					    false);
				} else {
					lit(state,
					    (char *)cmark_node_get_url(node),
					    false);
				}
				lit(state, ">", false);
				// return signal to skip contents of node...
				return 0;
			}
		} else {
			if (entering) {
				lit(state, "[", false);
			} else {
				lit(state, "](", false);
				out(state,
				    cmark_chunk_literal(cmark_node_get_url(node)),
				    false, URL);
				title = &node->as.link.title;
				if (title->len > 0) {
					lit(state, " \"", true);
					out(state, *title, false, TITLE);
					lit(state, "\"", false);
				}
				lit(state, ")", false);
			}
		}
		break;

	case CMARK_NODE_IMAGE:
		if (entering) {
			lit(state, "![", false);
		} else {
			lit(state, "](", false);
			out(state, cmark_chunk_literal(cmark_node_get_url(node)), false, URL);
			title = &node->as.link.title;
			if (title->len > 0) {
				lit(state, " \"", true);
				out(state, *title, false, TITLE);
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
	return cmark_render(root, options, width, outc, S_render_node);
}
