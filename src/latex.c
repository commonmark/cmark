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

static inline void outc(cmark_renderer *renderer,
			cmark_escaping escape,
			int32_t c,
			unsigned char nextc)
{
	if (escape == LITERAL) {
		utf8proc_encode_char(c, renderer->buffer);
		renderer->column += 1;
	} else {
		switch(c) {
		case 123: // '{'
		case 125: // '}'
		case 35: // '#'
		case 37: // '%'
		case 38: // '&'
			cmark_strbuf_putc(renderer->buffer, '\\');
			utf8proc_encode_char(c, renderer->buffer);
			renderer->column += 2;
			break;
		case 36: // '$'
		case 95: // '_'
			if (escape == NORMAL) {
				cmark_strbuf_putc(renderer->buffer, '\\');
				renderer->column += 1;
			}
			utf8proc_encode_char(c, renderer->buffer);
			renderer->column += 1;
			break;
		case 45 : // '-'
			if (nextc == 45) { // prevent ligature
				cmark_strbuf_putc(renderer->buffer, '\\');
				renderer->column += 1;
			}
			utf8proc_encode_char(c, renderer->buffer);
			renderer->column += 1;
			break;
		case 126: // '~'
			if (escape == NORMAL) {
				cmark_strbuf_puts(renderer->buffer,
						  "\\textasciitilde{}");
				renderer->column += 17;
			} else {
				utf8proc_encode_char(c, renderer->buffer);
				renderer->column += 1;
			}
			break;
		case 94: // '^'
			cmark_strbuf_puts(renderer->buffer,
					  "\\^{}");
			renderer->column += 4;
			break;
		case 92: // '\\'
			if (escape == URL) {
				// / acts as path sep even on windows:
				cmark_strbuf_puts(renderer->buffer, "/");
				renderer->column += 1;
			} else {
				cmark_strbuf_puts(renderer->buffer,
						  "\\textbackslash{}");
				renderer->column += 16;
			}
			break;
		case 124: // '|'
			cmark_strbuf_puts(renderer->buffer,
					  "\\textbar{}");
			renderer->column += 10;
			break;
		case 60: // '<'
			cmark_strbuf_puts(renderer->buffer,
					  "\\textless{}");
			renderer->column += 11;
			break;
		case 62: // '>'
			cmark_strbuf_puts(renderer->buffer,
					  "\\textgreater{}");
			renderer->column += 14;
			break;
		case 91: // '['
		case 93: // ']'
			cmark_strbuf_putc(renderer->buffer, '{');
			utf8proc_encode_char(c, renderer->buffer);
			cmark_strbuf_putc(renderer->buffer, '}');
			renderer->column += 3;
			break;
		case 34: // '"'
			cmark_strbuf_puts(renderer->buffer,
					  "\\textquotedbl{}");
			// requires \usepackage[T1]{fontenc}
			renderer->column += 15;
			break;
		case 39: // '\''
			cmark_strbuf_puts(renderer->buffer,
					  "\\textquotesingle{}");
			renderer->column += 18;
			// requires \usepackage{textcomp}
			break;
		case 160: // nbsp
			cmark_strbuf_putc(renderer->buffer, '~');
			renderer->column += 1;
			break;
		case 8230: // hellip
			cmark_strbuf_puts(renderer->buffer, "\\ldots{}");
			renderer->column += 8;
			break;
		case 8216: // lsquo
			if (escape == NORMAL) {
				cmark_strbuf_putc(renderer->buffer, '`');
				renderer->column += 1;
			} else {
				utf8proc_encode_char(c, renderer->buffer);
				renderer->column += 1;
			}
			break;
		case 8217: // rsquo
			if (escape == NORMAL) {
				cmark_strbuf_putc(renderer->buffer, '\'');
				renderer->column += 1;
			} else {
				utf8proc_encode_char(c, renderer->buffer);
				renderer->column += 1;
			}
			break;
		case 8220: // ldquo
			if (escape == NORMAL) {
				cmark_strbuf_puts(renderer->buffer, "``");
				renderer->column += 2;
			} else {
				utf8proc_encode_char(c, renderer->buffer);
				renderer->column += 1;
			}
			break;
		case 8221: // rdquo
			if (escape == NORMAL) {
				cmark_strbuf_puts(renderer->buffer, "''");
				renderer->column += 2;
			} else {
				utf8proc_encode_char(c, renderer->buffer);
				renderer->column += 1;
			}
			break;
		case 8212: // emdash
			if (escape == NORMAL) {
				cmark_strbuf_puts(renderer->buffer, "---");
				renderer->column += 3;
			} else {
				utf8proc_encode_char(c, renderer->buffer);
				renderer->column += 1;
			}
			break;
		case 8211: // endash
			if (escape == NORMAL) {
				cmark_strbuf_puts(renderer->buffer, "--");
				renderer->column += 2;
			} else {
				utf8proc_encode_char(c, renderer->buffer);
				renderer->column += 1;
			}
			break;
		default:
			utf8proc_encode_char(c, renderer->buffer);
			renderer->column += 1;
			renderer->begin_line = false;
		}
	}
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
              cmark_renderer *renderer)
{
	cmark_node *tmp;
	int list_number;
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
		renderer->in_tight_list_item =
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
			cmark_strbuf_putc(renderer->buffer, '\n');
		}
		break;

	case CMARK_NODE_BLOCK_QUOTE:
		if (entering) {
			lit(renderer, "\\begin{quote}", false);
			cr(renderer);
		} else {
			lit(renderer, "\\end{quote}", false);
			blankline(renderer);
		}
		break;

	case CMARK_NODE_LIST:
		list_type = cmark_node_get_list_type(node);
		if (entering) {
			if (list_type == CMARK_ORDERED_LIST) {
				renderer->enumlevel++;
			}
			lit(renderer, "\\begin{", false);
			lit(renderer,
			    list_type == CMARK_ORDERED_LIST ?
			    "enumerate" : "itemize", false);
			lit(renderer, "}", false);
			cr(renderer);
			list_number = cmark_node_get_list_start(node);
			if (list_number > 1) {
				sprintf(list_number_string,
				         "%d", list_number);
				lit(renderer, "\\setcounter{enum", false);
				lit(renderer, (char *)roman_numerals[renderer->enumlevel],
				    false);
				lit(renderer, "}{", false);
				out(renderer,
				    cmark_chunk_literal(list_number_string),
				    false, NORMAL);
				lit(renderer, "}", false);
				cr(renderer);
			}
		} else {
			if (list_type == CMARK_ORDERED_LIST) {
				renderer->enumlevel--;
			}
			lit(renderer, "\\end{", false);
			lit(renderer,
			    list_type == CMARK_ORDERED_LIST ?
			    "enumerate" : "itemize", false);
			lit(renderer, "}", false);
			blankline(renderer);
		}
		break;

	case CMARK_NODE_ITEM:
		if (entering) {
			lit(renderer, "\\item ", false);
		} else {
			cr(renderer);
		}
		break;

	case CMARK_NODE_HEADER:
		if (entering) {
			switch (cmark_node_get_header_level(node)) {
			case 1:
				lit(renderer, "\\section", false);
				break;
			case 2:
				lit(renderer, "\\subsection", false);
				break;
			case 3:
				lit(renderer, "\\subsubsection", false);
				break;
			case 4:
				lit(renderer, "\\paragraph", false);
				break;
			case 5:
				lit(renderer, "\\subparagraph", false);
				break;
			}
			lit(renderer, "{", false);
		} else {
			lit(renderer, "}", false);
			blankline(renderer);
		}
		break;

	case CMARK_NODE_CODE_BLOCK:
		cr(renderer);
		lit(renderer, "\\begin{verbatim}", false);
		cr(renderer);
		out(renderer, node->as.code.literal, false, LITERAL);
		cr(renderer);
		lit(renderer, "\\end{verbatim}", false);
		blankline(renderer);
		break;

	case CMARK_NODE_HTML:
		break;

	case CMARK_NODE_HRULE:
		blankline(renderer);
		lit(renderer, "\\begin{center}\\rule{0.5\\linewidth}{\\linethickness}\\end{center}", false);
		blankline(renderer);
		break;

	case CMARK_NODE_PARAGRAPH:
		if (!entering) {
			blankline(renderer);
		}
		break;

	case CMARK_NODE_TEXT:
		out(renderer, node->as.literal, true, NORMAL);
		break;

	case CMARK_NODE_LINEBREAK:
		lit(renderer, "\\\\", false);
		cr(renderer);
		break;

	case CMARK_NODE_SOFTBREAK:
		if (renderer->width == 0) {
			cr(renderer);
		} else {
			lit(renderer, " ", true);
		}
		break;

	case CMARK_NODE_CODE:
		lit(renderer, "\\texttt{", false);
		out(renderer, node->as.literal, false, NORMAL);
		lit(renderer, "}", false);
		break;

	case CMARK_NODE_INLINE_HTML:
		break;

	case CMARK_NODE_STRONG:
		if (entering) {
			lit(renderer, "\\textbf{", false);
		} else {
			lit(renderer, "}", false);
		}
		break;

	case CMARK_NODE_EMPH:
		if (entering) {
			lit(renderer, "\\emph{", false);
		} else {
			lit(renderer, "}", false);
		}
		break;

	case CMARK_NODE_LINK:
		if (entering) {
			url = cmark_chunk_literal(cmark_node_get_url(node));
			// requires \usepackage{hyperref}
			switch(get_link_type(node)) {
			case URL_AUTOLINK:
				lit(renderer, "\\url{", false);
				out(renderer, url, false, URL);
				break;
			case EMAIL_AUTOLINK:
				lit(renderer, "\\href{", false);
				out(renderer, url, false, URL);
				lit(renderer, "}\\nolinkurl{", false);
				break;
			case NORMAL_LINK:
				lit(renderer, "\\href{", false);
				out(renderer, url, false, URL);
				lit(renderer, "}{", false);
				break;
			case NO_LINK:
				lit(renderer, "{", false);  // error?
			}
		} else {
			lit(renderer, "}", false);
		}

		break;

	case CMARK_NODE_IMAGE:
		if (entering) {
			url = cmark_chunk_literal(cmark_node_get_url(node));
			lit(renderer, "\\protect\\includegraphics{", false);
			// requires \include{graphicx}
			out(renderer, url, false, URL);
			lit(renderer, "}", false);
			return 0;
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
	return cmark_render(root, options, width, outc, S_render_node);
}
