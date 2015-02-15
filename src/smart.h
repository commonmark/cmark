#ifndef CMARK_SMART_H
#define CMARK_SMART_H

#include <stddef.h>
#include <stdarg.h>
#include "config.h"

#ifdef __cplusplus
extern "C" {
#endif

void escape_with_smart(cmark_strbuf *buf,
		       cmark_node *node,
		       void (*escape)(cmark_strbuf *, const unsigned char *, int),
		       const char *left_double_quote,
		       const char *right_double_quote,
		       const char *left_single_quote,
		       const char *right_single_quote,
		       const char *em_dash,
		       const char *en_dash,
		       const char *ellipses);

#ifdef __cplusplus
}
#endif

#endif

