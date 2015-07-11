#ifndef CMARK_RENDER_H
#define CMARK_RENDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include "buffer.h"
#include "chunk.h"

typedef enum  {
	LITERAL,
	NORMAL,
	TITLE,
	URL
} cmark_escaping;

struct cmark_render_state {
	int options;
	cmark_strbuf* buffer;
	cmark_strbuf* prefix;
	int column;
	int width;
	int need_cr;
	bufsize_t last_breakable;
	int enumlevel;
	bool begin_line;
	bool no_wrap;
	bool in_tight_list_item;
	void (*outc)(struct cmark_render_state*,
		     cmark_escaping,
		     int32_t,
		     unsigned char);
};

typedef struct cmark_render_state cmark_render_state;

void cr(cmark_render_state *state);

void blankline(cmark_render_state *state);

void out(cmark_render_state *state,
	 cmark_chunk str,
	 bool wrap,
	 cmark_escaping escape);

void lit(cmark_render_state *state, char *s, bool wrap);

char*
cmark_render(cmark_node *root,
	     int options,
	     int width,
	     void (*outc)(cmark_render_state*,
			  cmark_escaping,
			  int32_t,
			  unsigned char),
	     int (*render_node)(cmark_node *node,
				cmark_event_type ev_type,
				cmark_render_state *state));


#ifdef __cplusplus
}
#endif

#endif
