#ifndef CMARK_REGISTRY_H
#define CMARK_REGISTRY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmark.h"

void cmark_discover_plugins(void);
void cmark_release_plugins(void);
cmark_llist *cmark_list_syntax_extensions(void);

#ifdef __cplusplus
}
#endif

#endif
