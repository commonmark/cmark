#ifndef CMARK_CMARK_CTYPE_H
#define CMARK_CMARK_CTYPE_H

#ifdef __cplusplus
extern "C" {
#endif

/** Locale-independent versions of functions from ctype.h.
 * We want cmark to behave the same no matter what the system locale.
 */

int cmark_isspace(unsigned char c);

int cmark_ispunct(unsigned char c);

int cmark_isalnum(unsigned char c);

int cmark_isdigit(unsigned char c);

int cmark_isalpha(unsigned char c);

#ifdef __cplusplus
}
#endif

#endif
