#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "node.h"
#include "houdini.h"
#include "cmark.h"
#include "buffer.h"

int cmark_version() { return CMARK_VERSION; }

const char *cmark_version_string() { return CMARK_VERSION_STRING; }

static void *xcalloc(size_t nmem, size_t size) {
  void *ptr = calloc(nmem, size);
  if (!ptr)
    abort();
  return ptr;
}

static void *xrealloc(void *ptr, size_t size) {
  void *new_ptr = realloc(ptr, size);
  if (!new_ptr)
    abort();
  return new_ptr;
}

void cmark_default_mem_free(void *ptr)
{
  free(ptr);
}

cmark_mem DEFAULT_MEM_ALLOCATOR = {xcalloc, xrealloc, free};

char *cmark_markdown_to_html(const char *text, size_t len, int options) {
  cmark_node *doc;
  char *result;

  doc = cmark_parse_document(text, len, options);
  if (doc == NULL) {
    return NULL;
  }

  result = cmark_render_html(doc, options);
  cmark_node_free(doc);

  return result;
}
