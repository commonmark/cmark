#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "cmark.h"

static struct arena_chunk {
  size_t sz, used;
  void *ptr;
  struct arena_chunk *prev;
} *A = NULL;

static struct arena_chunk *alloc_arena_chunk(size_t sz, struct arena_chunk *prev) {
  struct arena_chunk *c = (struct arena_chunk *)calloc(1, sizeof(*c));
  if (!c)
    abort();
  c->sz = sz;
  c->ptr = calloc(1, sz);
  if (!c->ptr)
    abort();
  c->prev = prev;
  return c;
}

static void init_arena(void) {
  A = alloc_arena_chunk(4 * 1048576, NULL);
}

void cmark_arena_reset(void) {
  while (A) {
    free(A->ptr);
    struct arena_chunk *n = A->prev;
    free(A);
    A = n;
  }
}

static void *arena_calloc(size_t nmem, size_t size) {
  if (!A)
    init_arena();

  size_t sz = nmem * size + sizeof(size_t);
  if (sz > A->sz) {
    A->prev = alloc_arena_chunk(sz, A->prev);
    return (uint8_t *) A->prev->ptr + sizeof(size_t);
  }
  if (sz > A->sz - A->used) {
    A = alloc_arena_chunk(A->sz + A->sz / 2, A);
  }
  void *ptr = (uint8_t *) A->ptr + A->used;
  A->used += sz;
  *((size_t *) ptr) = nmem * size;
  return (uint8_t *) ptr + sizeof(size_t);
}

static void *arena_realloc(void *ptr, size_t size) {
  if (!A)
    init_arena();

  void *new_ptr = arena_calloc(1, size);
  if (ptr)
    memcpy(new_ptr, ptr, ((size_t *) ptr)[-1]);
  return new_ptr;
}

static void arena_free(void *ptr) {
  (void) ptr;
  /* no-op */
}

cmark_mem CMARK_ARENA_MEM_ALLOCATOR = {arena_calloc, arena_realloc, arena_free};

cmark_mem *cmark_get_arena_mem_allocator() {
  return &CMARK_ARENA_MEM_ALLOCATOR;
}
