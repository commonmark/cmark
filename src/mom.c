#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "config.h"
#include "cmark.h"
#include "node.h"
#include "buffer.h"
#include "utf8.h"
#include "render.h"

#define OUT(s, wrap, escaping) renderer->out(renderer, s, wrap, escaping)
#define LIT(s) renderer->out(renderer, s, false, LITERAL)
#define CR() renderer->cr(renderer)
#define BLANKLINE() renderer->blankline(renderer)
#define LIST_NUMBER_SIZE 20

// Functions to convert cmark_nodes to groff man strings.
static void S_outc(cmark_renderer *renderer, cmark_escaping escape, int32_t c,
                   unsigned char nextc) {
  (void)(nextc);

  if (escape == LITERAL) {
    cmark_render_code_point(renderer, c);
    return;
  }

  switch (c) {
  case 46:
    if (renderer->begin_line) {
      cmark_render_ascii(renderer, "\\&.");
    } else {
      cmark_render_code_point(renderer, c);
    }
    break;
  case 39:
    if (renderer->begin_line) {
      cmark_render_ascii(renderer, "\\&'");
    } else {
      cmark_render_code_point(renderer, c);
    }
    break;
  case 45:
    cmark_render_ascii(renderer, "\\-");
    break;
  case 92:
    cmark_render_ascii(renderer, "\\e");
    break;
  case 8216: // left single quote
    cmark_render_ascii(renderer, "\\[oq]");
    break;
  case 8217: // right single quote
    cmark_render_ascii(renderer, "\\[cq]");
    break;
  case 8220: // left double quote
    cmark_render_ascii(renderer, "\\[lq]");
    break;
  case 8221: // right double quote
    cmark_render_ascii(renderer, "\\[rq]");
    break;
  case 8212: // em dash
    cmark_render_ascii(renderer, "\\[em]");
    break;
  case 8211: // en dash
    cmark_render_ascii(renderer, "\\[en]");
    break;
  default:
    cmark_render_code_point(renderer, c);
  }
}

static int S_render_node(cmark_renderer *renderer, cmark_node *node,
                         cmark_event_type ev_type, int options) {
  bool entering = (ev_type == CMARK_EVENT_ENTER);
  bool allow_wrap = renderer->width > 0 && !(CMARK_OPT_NOBREAKS & options);
  char s_tmp[128];


  // avoid unused parameter error:
  (void)(options);

  switch (node->type) {
  case CMARK_NODE_DOCUMENT:
    if (entering) {
      LIT(".DOCTYPE DEFAULT");
      CR();
      LIT(".PRINTSTYLE TYPESET");
      CR();
      LIT(".JUSTIFY");
      CR();
      LIT(".START");
      CR();
      CR();
    }
    break;

  case CMARK_NODE_BLOCK_QUOTE:
    if (entering) {
      CR();
      LIT(".BLOCKQUOTE");
      CR();
    } else {
      CR();
      LIT(".BLOCKQUOTE END");
      CR();
    }
    break;

  case CMARK_NODE_LIST:
    if (entering) {
      CR();
      LIT(".LIST ");
      if (cmark_node_get_list_type(node) == CMARK_BULLET_LIST) {
        LIT("BULLET");
      } else {
        LIT("DIGIT");
      }
      CR();
      LIT(".SHIFT_LIST 20p");
      CR();
    } else {
      CR();
      LIT(".LIST OFF");
      CR();
    }
    break;

  case CMARK_NODE_ITEM:
    if (entering) {
      CR();
      LIT(".ITEM");
      CR();
    } else {
      CR();
    }
    break;

  case CMARK_NODE_HEADING:
    if (entering) {
      CR();
      sprintf(s_tmp, ".HEADING %d \"", cmark_node_get_heading_level(node));
      LIT(s_tmp);
    } else {
      LIT("\"");
      CR();
    }
    break;


  case CMARK_NODE_CODE_BLOCK:
    CR();
    LIT(".QUOTE");
    CR();
    LIT(".CODE BR");
    CR();
    OUT(cmark_node_get_literal(node), false, NORMAL);
    CR();
    LIT(".QUOTE OFF");
    CR();
    break;

  case CMARK_NODE_HTML_BLOCK:
    break;

  case CMARK_NODE_CUSTOM_BLOCK:
    CR();
    OUT(entering ? cmark_node_get_on_enter(node) : cmark_node_get_on_exit(node),
        false, LITERAL);
    CR();
    break;

  case CMARK_NODE_THEMATIC_BREAK:
    CR();
    LIT(".PP\n  -+-+-+-+-");
    CR();
    break;

  case CMARK_NODE_PARAGRAPH:
    if (entering) {
      // no blank line if first paragraph in list:
        CR();
        LIT(".PP");
        CR();
    } else {
      CR();
    }
    break;

  case CMARK_NODE_TEXT:
    OUT(cmark_node_get_literal(node), allow_wrap, NORMAL);
    break;

  case CMARK_NODE_LINEBREAK:
    CR();
    LIT(".BR");
    CR();
    break;

  case CMARK_NODE_SOFTBREAK:
    if (options & CMARK_OPT_HARDBREAKS) {
      LIT(".PD 0\n.P\n.PD");
      CR();
    } else if (renderer->width == 0 && !(CMARK_OPT_NOBREAKS & options)) {
      CR();
    } else {
      OUT(" ", allow_wrap, LITERAL);
    }
    break;

  case CMARK_NODE_CODE:
    LIT("\\f[C]");
    OUT(cmark_node_get_literal(node), allow_wrap, NORMAL);
    LIT("\\f[]");
    break;

  case CMARK_NODE_HTML_INLINE:
    break;

  case CMARK_NODE_CUSTOM_INLINE:
    OUT(entering ? cmark_node_get_on_enter(node) : cmark_node_get_on_exit(node),
        false, LITERAL);
    break;

  case CMARK_NODE_STRONG:
    if (entering) {
      LIT("\\*[BOLDER]");

    } else {
      LIT("\\*[BOLDERX]");
    }
    break;

  case CMARK_NODE_EMPH:
    if (entering) {
      LIT("\\*[SLANT]");
    } else {
      LIT("\\*[SLANTX]");
    }
    break;

  case CMARK_NODE_LINK:
    if (!entering) {
      LIT(" (");
      OUT(cmark_node_get_url(node), allow_wrap, URL);
      LIT(")");
    }
    break;

  case CMARK_NODE_IMAGE:
    if (entering) {
      LIT("[IMAGE: ");
    } else {
      LIT("]");
    }
    break;

  default:
    assert(false);
    break;
  }

  return 1;
}

char *cmark_render_mom(cmark_node *root, int options, int width) {
  return cmark_render(root, options, width, S_outc, S_render_node);
}
