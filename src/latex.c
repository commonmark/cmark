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

// Functions to convert cmark_nodes to commonmark strings.

struct render_state {
	int options;
	cmark_strbuf* buffer;
	cmark_strbuf* prefix;
	int column;
	int width;
	int need_cr;
	int enumlevel;
	bufsize_t last_breakable;
	bool begin_line;
	bool no_wrap;
	bool in_tight_list_item;
	bool silence;
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

typedef enum  {
	LITERAL,
	NORMAL,
	URL
} escaping;

static inline void out(struct render_state *state,
                       cmark_chunk str,
                       bool wrap,
                       escaping escape)
{
	unsigned char* source = str.data;
	int length = str.len;
	unsigned char nextc;
	int32_t c;
	int i = 0;
	int len;
	cmark_chunk remainder = cmark_chunk_literal("");
	int k = state->buffer->size - 1;

	if (state->silence)
		return;

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
		if (len == -1) { // error condition
			return;  // return without rendering rest of string
		}
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
		} else if (escape == LITERAL) {
			utf8proc_encode_char(c, state->buffer);
			state->column += 2;
		} else {
			switch(c) {
			case 123: // '{'
			case 125: // '}'
			case 35: // '#'
			case 37: // '%'
			case 38: // '&'
				cmark_strbuf_putc(state->buffer, '\\');
				utf8proc_encode_char(c, state->buffer);
				state->column += 2;
				break;
			case 36: // '$'
			case 95: // '_'
				if (escape == NORMAL) {
					cmark_strbuf_putc(state->buffer, '\\');
				}
				utf8proc_encode_char(c, state->buffer);
				break;
			case 45 : // '-'
				if (nextc == 45) { // prevent ligature
					cmark_strbuf_putc(state->buffer, '\\');
				}
				utf8proc_encode_char(c, state->buffer);
				break;
			case 126: // '~'
				if (escape == NORMAL) {
					cmark_strbuf_puts(state->buffer,
					                  "\\textasciitilde{}");
				} else {
					utf8proc_encode_char(c, state->buffer);
				}
				break;
			case 94: // '^'
				cmark_strbuf_puts(state->buffer,
				                  "\\^{}");
				break;
			case 92: // '\\'
				if (escape == URL) {
					// / acts as path sep even on windows:
					cmark_strbuf_puts(state->buffer, "/");
				} else {
					cmark_strbuf_puts(state->buffer,
					                  "\\textbackslash{}");
				}
				break;
			case 124: // '|'
				cmark_strbuf_puts(state->buffer,
				                  "\\textbar{}");
				break;
			case 60: // '<'
				cmark_strbuf_puts(state->buffer,
				                  "\\textless{}");
				break;
			case 62: // '>'
				cmark_strbuf_puts(state->buffer,
				                  "\\textgreater{}");
				break;
			case 91: // '['
			case 93: // ']'
				cmark_strbuf_putc(state->buffer, '{');
				utf8proc_encode_char(c, state->buffer);
				cmark_strbuf_putc(state->buffer, '}');
				break;
			case 34: // '"'
				cmark_strbuf_puts(state->buffer,
				                  "\\textquotedbl{}");
				// requires \usepackage[T1]{fontenc}
				break;
			case 39: // '\''
				cmark_strbuf_puts(state->buffer,
				                  "\\textquotesingle{}");
				// requires \usepackage{textcomp}
				break;
			case 160: // nbsp
				cmark_strbuf_putc(state->buffer, '~');
				break;
			case 8230: // hellip
				cmark_strbuf_puts(state->buffer, "\\ldots{}");
				break;
			case 8216: // lsquo
				if (escape == NORMAL) {
					cmark_strbuf_putc(state->buffer, '`');
				} else {
					utf8proc_encode_char(c, state->buffer);
				}
				break;
			case 8217: // rsquo
				if (escape == NORMAL) {
					cmark_strbuf_putc(state->buffer, '\'');
				} else {
					utf8proc_encode_char(c, state->buffer);
				}
				break;
			case 8220: // ldquo
				if (escape == NORMAL) {
					cmark_strbuf_puts(state->buffer, "``");
				} else {
					utf8proc_encode_char(c, state->buffer);
				}
				break;
			case 8221: // rdquo
				if (escape == NORMAL) {
					cmark_strbuf_puts(state->buffer, "''");
				} else {
					utf8proc_encode_char(c, state->buffer);
				}
				break;
			case 8212: // emdash
				if (escape == NORMAL) {
					cmark_strbuf_puts(state->buffer, "---");
				} else {
					utf8proc_encode_char(c, state->buffer);
				}
				break;
			case 8211: // endash
				if (escape == NORMAL) {
					cmark_strbuf_puts(state->buffer, "--");
				} else {
					utf8proc_encode_char(c, state->buffer);
				}
				break;
			default:
				utf8proc_encode_char(c, state->buffer);
				state->column += 1;
				state->begin_line = false;
			}
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
	out(state, str, wrap, LITERAL);
}

typedef enum  {
	NO_LINK,
	URL_AUTOLINK,
	EMAIL_AUTOLINK,
	NORMAL_LINK
} link_type;

static link_type
get_link_type(cmark_node *node)
{
	cmark_chunk *title;
	cmark_chunk *url;
	cmark_node *link_text;
	char *realurl;
	int realurllen;
	bool isemail = false;

	if (node->type != CMARK_NODE_LINK) {
		return NO_LINK;
	}

	url = &node->as.link.url;
	if (url->len == 0 || scan_scheme(url, 0) == 0) {
		return NO_LINK;
	}

	title = &node->as.link.title;
	// if it has a title, we can't treat it as an autolink:
	if (title->len > 0) {
		return NORMAL_LINK;
	}

	link_text = node->first_child;
	cmark_consolidate_text_nodes(link_text);
	realurl = (char*)url->data;
	realurllen = url->len;
	if (strncmp(realurl, "mailto:", 7) == 0) {
		realurl += 7;
		realurllen -= 7;
		isemail = true;
	}
	if (realurllen == link_text->as.literal.len &&
	    strncmp(realurl,
	            (char*)link_text->as.literal.data,
	            link_text->as.literal.len) == 0) {
		if (isemail) {
			return EMAIL_AUTOLINK;
		} else {
			return URL_AUTOLINK;
		}
	} else {
		return NORMAL_LINK;
	}
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
              struct render_state *state)
{
	cmark_node *tmp;
	cmark_chunk *code;
	int list_number;
	int len;
	char list_number_string[20];
	bool entering = (ev_type == CMARK_EVENT_ENTER);
	cmark_list_type list_type;
	cmark_chunk url;
	const char* roman_numerals[] = { "", "i", "ii", "iii", "iv", "v",
	                                 "vi", "vii", "viii", "ix", "x"
	                               };

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
			lit(state, "\\begin{quote}", false);
			cr(state);
		} else {
			lit(state, "\\end{quote}", false);
			blankline(state);
		}
		break;

	case CMARK_NODE_LIST:
		list_type = cmark_node_get_list_type(node);
		if (entering) {
			if (list_type == CMARK_ORDERED_LIST) {
				state->enumlevel++;
			}
			lit(state, "\\begin{", false);
			lit(state,
			    list_type == CMARK_ORDERED_LIST ?
			    "enumerate" : "itemize", false);
			lit(state, "}", false);
			cr(state);
			list_number = cmark_node_get_list_start(node);
			if (list_number > 1) {
				len = snprintf(list_number_string, 19,
				         "%d", list_number);
#ifndef HAVE_C99_SNPRINTF
				// Assume we're on Windows.
				if (len < 0) {
					len = _snprintf("%d", list_number);
				}
#endif
				lit(state, "\\setcounter{enum", false);
				lit(state, (char *)roman_numerals[state->enumlevel],
				    false);
				lit(state, "}{", false);
				out(state,
				    cmark_chunk_literal(list_number_string),
				    false, NORMAL);
				lit(state, "}", false);
				cr(state);
			}
		} else {
			if (list_type == CMARK_ORDERED_LIST) {
				state->enumlevel--;
			}
			lit(state, "\\end{", false);
			lit(state,
			    list_type == CMARK_ORDERED_LIST ?
			    "enumerate" : "itemize", false);
			lit(state, "}", false);
			blankline(state);
		}
		break;

	case CMARK_NODE_ITEM:
		if (entering) {
			lit(state, "\\item ", false);
		} else {
			cr(state);
		}
		break;

	case CMARK_NODE_HEADER:
		if (entering) {
			switch (cmark_node_get_header_level(node)) {
			case 1:
				lit(state, "\\section", false);
				break;
			case 2:
				lit(state, "\\subsection", false);
				break;
			case 3:
				lit(state, "\\subsubsection", false);
				break;
			case 4:
				lit(state, "\\paragraph", false);
				break;
			case 5:
				lit(state, "\\subparagraph", false);
				break;
			}
			lit(state, "{", false);
		} else {
			lit(state, "}", false);
			blankline(state);
		}
		break;

	case CMARK_NODE_CODE_BLOCK:
		cr(state);
		lit(state, "\\begin{verbatim}", false);
		cr(state);
		code = &node->as.code.literal;
		out(state, node->as.code.literal, false, LITERAL);
		cr(state);
		lit(state, "\\end{verbatim}", false);
		blankline(state);
		break;

	case CMARK_NODE_HTML:
		break;

	case CMARK_NODE_HRULE:
		blankline(state);
		lit(state, "\\begin{center}\\rule{0.5\\linewidth}{\\linethickness}\\end{center}", false);
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
		lit(state, "\\\\", false);
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
		lit(state, "\\texttt{", false);
		out(state, node->as.literal, false, NORMAL);
		lit(state, "}", false);
		break;

	case CMARK_NODE_INLINE_HTML:
		break;

	case CMARK_NODE_STRONG:
		if (entering) {
			lit(state, "\\textbf{", false);
		} else {
			lit(state, "}", false);
		}
		break;

	case CMARK_NODE_EMPH:
		if (entering) {
			lit(state, "\\emph{", false);
		} else {
			lit(state, "}", false);
		}
		break;

	case CMARK_NODE_LINK:
		if (entering) {
			url = cmark_chunk_literal(cmark_node_get_url(node));
			// requires \usepackage{hyperref}
			switch(get_link_type(node)) {
			case URL_AUTOLINK:
				lit(state, "\\url{", false);
				out(state, url, false, URL);
				break;
			case EMAIL_AUTOLINK:
				lit(state, "\\href{", false);
				out(state, url, false, URL);
				lit(state, "}\\nolinkurl{", false);
				break;
			case NORMAL_LINK:
				lit(state, "\\href{", false);
				out(state, url, false, URL);
				lit(state, "}{", false);
				break;
			case NO_LINK:
				lit(state, "{", false);  // error?
			}
		} else {
			lit(state, "}", false);
		}

		break;

	case CMARK_NODE_IMAGE:
		if (entering) {
			url = cmark_chunk_literal(cmark_node_get_url(node));
			lit(state, "\\protect\\includegraphics{", false);
			// requires \include{graphicx}
			out(state, url, false, URL);
			lit(state, "}", false);
			state->silence = true; // don't print the alt text
		} else {
			state->silence = false;
		}
		break;

	default:
		assert(false);
		break;
	}

	return 1;
}

char *cmark_render_latex(cmark_node *root, int options, int width)
{
	char *result;
	cmark_strbuf commonmark = GH_BUF_INIT;
	cmark_strbuf prefix = GH_BUF_INIT;
	if (CMARK_OPT_HARDBREAKS & options) {
		width = 0;
	}
	struct render_state state = {
		options, &commonmark, &prefix, 0, width,
		0, 0, 0, true, false, false, false
	};
	cmark_node *cur;
	cmark_event_type ev_type;
	cmark_iter *iter = cmark_iter_new(root);

	while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
		cur = cmark_iter_get_node(iter);
		if (!S_render_node(cur, ev_type, &state)) {
			// a false value causes us to skip processing
			// the node's contents.  this is used for
			// autolinks.
			cmark_iter_reset(iter, cur, CMARK_EVENT_EXIT);
		}
	}
	result = (char *)cmark_strbuf_detach(&commonmark);

	cmark_strbuf_free(&prefix);
	cmark_iter_free(iter);
	return result;
}
