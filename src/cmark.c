#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "node.h"
#include "houdini.h"
#include "cmark.h"
#include "buffer.h"

int cmark_version() { return CMARK_VERSION; }

const char *cmark_version_string() { return CMARK_VERSION_STRING; }

void (*_cmark_on_oom)(void) = NULL;

void cmark_trigger_oom(void)
{
	if (_cmark_on_oom)
		_cmark_on_oom();
	abort();
}

void cmark_set_oom_handler(void (*handler)(void))
{
	_cmark_on_oom = handler;
}

void *cmark_calloc(size_t nmem, size_t size)
{
	void *ptr = calloc(nmem, size);
	if (!ptr)
		cmark_trigger_oom();
	return ptr;
}

void *cmark_realloc(void *ptr, size_t size)
{
	void *ptr_new = realloc(ptr, size);
	if (!ptr_new)
		cmark_trigger_oom();
	return ptr_new;
}

char *cmark_markdown_to_html(const char *text, size_t len, int options) {
  cmark_node *doc;
  char *result;

  doc = cmark_parse_document(text, len, options);

  result = cmark_render_html(doc, options);
  cmark_node_free(doc);

  return result;
}
