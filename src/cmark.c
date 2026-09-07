#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include "node.h"
#include "houdini.h"
#include "cmark.h"
#include "buffer.h"

int cmark_version(void) { return CMARK_VERSION; }

const char *cmark_version_string(void) { return CMARK_VERSION_STRING; }

static void *xcalloc(void *ctx, size_t nmem, size_t size) {
  (void)ctx; // explicitly discard
  void *ptr = calloc(nmem, size);
  if (!ptr) {
    fprintf(stderr, "[cmark] calloc returned null pointer, aborting\n");
    abort();
  }
  return ptr;
}

static void *xrealloc(void *ctx, void *ptr, size_t size) {
  (void)ctx; // explicitly discard
  void *new_ptr = realloc(ptr, size);
  if (!new_ptr) {
    fprintf(stderr, "[cmark] realloc returned null pointer, aborting\n");
    abort();
  }
  return new_ptr;
}

static void xfree(void *ctx, void *ptr) {
  (void)ctx; // explicitly discard
  free(ptr);
}

cmark_mem DEFAULT_MEM_ALLOCATOR = {NULL, xcalloc, xrealloc, xfree};

cmark_mem *cmark_get_default_mem_allocator(void) {
  return &DEFAULT_MEM_ALLOCATOR;
}


char *cmark_markdown_to_html(const char *text, size_t len, int options) {
  cmark_node *doc;
  char *result;

  doc = cmark_parse_document(text, len, options);

  result = cmark_render_html(doc, options);
  cmark_node_free(doc);

  return result;
}
