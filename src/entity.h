#ifndef CMARK_ENTITY_H
#define CMARK_ENTITY_H

#include "entity.h"
#include "chunk.h"

#define CMARK_ENTITY_MIN_LENGTH 2
#define CMARK_ENTITY_MAX_LENGTH 32
#define CMARK_NUM_ENTITIES 2125

#ifdef __cplusplus
extern "C" {
#endif

extern int scan_entity(cmark_chunk *chunk, bufsize_t offset);
extern const unsigned char *cmark_lookup_entity(const unsigned char *s, int len);

#ifdef __cplusplus
}
#endif

#endif
