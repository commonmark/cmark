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

struct cmark_renderer {
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
	void (*outc)(struct cmark_renderer*,
		     cmark_escaping,
		     int32_t,
		     unsigned char);
};

typedef struct cmark_renderer cmark_renderer;

void cr(cmark_renderer *renderer);

void blankline(cmark_renderer *renderer);

void out(cmark_renderer *renderer,
	 cmark_chunk str,
	 bool wrap,
	 cmark_escaping escape);

void lit(cmark_renderer *renderer, char *s, bool wrap);

char*
cmark_render(cmark_node *root,
	     int options,
	     int width,
	     void (*outc)(cmark_renderer*,
			  cmark_escaping,
			  int32_t,
			  unsigned char),
	     int (*render_node)(cmark_node *node,
				cmark_event_type ev_type,
				cmark_renderer *renderer));


#ifdef __cplusplus
}
#endif

#endif
