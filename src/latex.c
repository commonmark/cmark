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

#define safe_strlen(s) cmark_strbuf_safe_strlen(s)
#define OUT(s, wrap, escaping) renderer->out(renderer, s, wrap, escaping)
#define LIT(s) renderer->out(renderer, s, false, LITERAL)
#define CR() renderer->cr(renderer)
#define BLANKLINE() renderer->blankline(renderer)

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
	size_t title_len, url_len;
	cmark_node *link_text;
	char *realurl;
	int realurllen;
	bool isemail = false;

	if (node->type != CMARK_NODE_LINK) {
		return NO_LINK;
	}

	const char* url = cmark_node_get_url(node);
	cmark_chunk url_chunk = cmark_chunk_literal(url);

	url_len = safe_strlen(url);
	if (url_len == 0 || scan_scheme(&url_chunk, 0) == 0) {
		return NO_LINK;
	}

	const char* title = cmark_node_get_title(node);
	title_len = safe_strlen(title);
	// if it has a title, we can't treat it as an autolink:
	if (title_len > 0) {
		return NORMAL_LINK;
	}

	link_text = node->first_child;
	cmark_consolidate_text_nodes(link_text);
	realurl = (char*)url;
	realurllen = url_len;
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

static int
S_render_node(cmark_node *node, cmark_event_type ev_type,
              cmark_renderer *renderer)
{
	int list_number;
	char list_number_string[20];
	bool entering = (ev_type == CMARK_EVENT_ENTER);
	cmark_list_type list_type;
	const char* roman_numerals[] = { "", "i", "ii", "iii", "iv", "v",
	                                 "vi", "vii", "viii", "ix", "x"
	                               };

	switch (node->type) {
	case CMARK_NODE_DOCUMENT:
		break;

	case CMARK_NODE_BLOCK_QUOTE:
		if (entering) {
			LIT("\\begin{quote}");
			CR();
		} else {
			LIT("\\end{quote}");
			BLANKLINE();
		}
		break;

	case CMARK_NODE_LIST:
		list_type = cmark_node_get_list_type(node);
		if (entering) {
			if (list_type == CMARK_ORDERED_LIST) {
				renderer->enumlevel++;
			}
			LIT("\\begin{");
			LIT(list_type == CMARK_ORDERED_LIST ?
			    "enumerate" : "itemize");
			LIT("}");
			CR();
			list_number = cmark_node_get_list_start(node);
			if (list_number > 1) {
				sprintf(list_number_string,
				         "%d", list_number);
				LIT("\\setcounter{enum");
				LIT((char *)roman_numerals[renderer->enumlevel]);
				LIT("}{");
				OUT(list_number_string, false, NORMAL);
				LIT("}");
				CR();
			}
		} else {
			if (list_type == CMARK_ORDERED_LIST) {
				renderer->enumlevel--;
			}
			LIT("\\end{");
			LIT(list_type == CMARK_ORDERED_LIST ?
			    "enumerate" : "itemize");
			LIT("}");
			BLANKLINE();
		}
		break;

	case CMARK_NODE_ITEM:
		if (entering) {
			LIT("\\item ");
		} else {
			CR();
		}
		break;

	case CMARK_NODE_HEADER:
		if (entering) {
			switch (cmark_node_get_header_level(node)) {
			case 1:
				LIT("\\section");
				break;
			case 2:
				LIT("\\subsection");
				break;
			case 3:
				LIT("\\subsubsection");
				break;
			case 4:
				LIT("\\paragraph");
				break;
			case 5:
				LIT("\\subparagraph");
				break;
			}
			LIT("{");
		} else {
			LIT("}");
			BLANKLINE();
		}
		break;

	case CMARK_NODE_CODE_BLOCK:
		CR();
		LIT("\\begin{verbatim}");
		CR();
		OUT(cmark_node_get_literal(node), false, LITERAL);
		CR();
		LIT("\\end{verbatim}");
		BLANKLINE();
		break;

	case CMARK_NODE_HTML:
		break;

	case CMARK_NODE_HRULE:
		BLANKLINE();
		LIT("\\begin{center}\\rule{0.5\\linewidth}{\\linethickness}\\end{center}");
		BLANKLINE();
		break;

	case CMARK_NODE_PARAGRAPH:
		if (!entering) {
			BLANKLINE();
		}
		break;

	case CMARK_NODE_TEXT:
		OUT(cmark_node_get_literal(node), true, NORMAL);
		break;

	case CMARK_NODE_LINEBREAK:
		LIT("\\\\");
		CR();
		break;

	case CMARK_NODE_SOFTBREAK:
		if (renderer->width == 0) {
			CR();
		} else {
			OUT(" ", true, NORMAL);
		}
		break;

	case CMARK_NODE_CODE:
		LIT("\\texttt{");
		OUT(cmark_node_get_literal(node), false, NORMAL);
		LIT("}");
		break;

	case CMARK_NODE_INLINE_HTML:
		break;

	case CMARK_NODE_STRONG:
		if (entering) {
			LIT("\\textbf{");
		} else {
			LIT("}");
		}
		break;

	case CMARK_NODE_EMPH:
		if (entering) {
			LIT("\\emph{");
		} else {
			LIT("}");
		}
		break;

	case CMARK_NODE_LINK:
		if (entering) {
			const char* url = cmark_node_get_url(node);
			// requires \usepackage{hyperref}
			switch(get_link_type(node)) {
			case URL_AUTOLINK:
				LIT("\\url{");
				OUT(url, false, URL);
				break;
			case EMAIL_AUTOLINK:
				LIT("\\href{");
				OUT(url, false, URL);
				LIT("}\\nolinkurl{");
				break;
			case NORMAL_LINK:
				LIT("\\href{");
				OUT(url, false, URL);
				LIT("}{");
				break;
			case NO_LINK:
				LIT("{");  // error?
			}
		} else {
			LIT("}");
		}

		break;

	case CMARK_NODE_IMAGE:
		if (entering) {
			LIT("\\protect\\includegraphics{");
			// requires \include{graphicx}
			OUT(cmark_node_get_url(node), false, URL);
			LIT("}");
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
