#ifndef CMARK_ENTITY_H
#define CMARK_ENTITY_H

#include "entity.h"

#ifdef __cplusplus
extern "C" {
#endif

extern const unsigned char *cmark_lookup_entity(const unsigned char *s, int len);

#ifdef __cplusplus
}
#endif

#endif
