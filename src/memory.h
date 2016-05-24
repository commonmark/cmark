#ifndef CMARK_MEM_H
#define CMARK_MEM_H

#ifdef __cplusplus
extern "C" {
#endif

void *cmark_calloc(size_t nmem, size_t size);
void *cmark_realloc(void *ptr, size_t size);
void cmark_trigger_oom(void);

#endif
