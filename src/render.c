#include <stdlib.h>
#include "buffer.h"
#include "chunk.h"
#include "cmark.h"
#include "utf8.h"
#include "render.h"

static inline
cmark_render_state
cmark_initialize_render_state(int options, int width,
			      void (*outc)(cmark_render_state*,
				           cmark_escaping,
					   int32_t,
					   unsigned char))
{
	cmark_strbuf *pref = (cmark_strbuf*)malloc(sizeof(cmark_strbuf));
	cmark_strbuf *buf  = (cmark_strbuf*)malloc(sizeof(cmark_strbuf));
	cmark_strbuf_init(pref, 16);
	cmark_strbuf_init(buf, 1024);
	cmark_render_state state = { options, buf, pref, 0, width,
				     0, 0, 0, true, false, false, outc };
	return state;
}

static inline
char *
cmark_finalize_render_state(cmark_render_state *state)
{
	char * result;
	result = (char *)cmark_strbuf_detach(state->buffer);
	cmark_strbuf_free(state->prefix);
	cmark_strbuf_free(state->buffer);
	free(state->prefix);
	free(state->buffer);
	return result;
}

void cr(cmark_render_state *state)
{
	if (state->need_cr < 1) {
		state->need_cr = 1;
	}
}

void blankline(cmark_render_state *state)
{
	if (state->need_cr < 2) {
		state->need_cr = 2;
	}
}

void out(cmark_render_state *state,
	 cmark_chunk str,
	 bool wrap,
	 cmark_escaping escape)
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
		} else {
			(state->outc)(state, escape, c, nextc);
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

void lit(cmark_render_state *state, char *s, bool wrap)
{
	cmark_chunk str = cmark_chunk_literal(s);
	out(state, str, wrap, LITERAL);
}

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
				 cmark_render_state *state))
{
	if (CMARK_OPT_HARDBREAKS & options) {
		width = 0;
	}
	cmark_render_state state = cmark_initialize_render_state(options, width, outc);
	cmark_node *cur;
	cmark_event_type ev_type;
	cmark_iter *iter = cmark_iter_new(root);

	while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
		cur = cmark_iter_get_node(iter);
		if (!render_node(cur, ev_type, &state)) {
			// a false value causes us to skip processing
			// the node's contents.  this is used for
			// autolinks.
			cmark_iter_reset(iter, cur, CMARK_EVENT_EXIT);
		}
	}

	cmark_iter_free(iter);

	return cmark_finalize_render_state(&state);
}
