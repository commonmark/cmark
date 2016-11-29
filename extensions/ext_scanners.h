#include "chunk.h"
#include "cmark.h"

#ifdef __cplusplus
extern "C" {
#endif

bufsize_t _ext_scan_at(bufsize_t (*scanner)(const unsigned char *),
                       unsigned char *ptr, int len, bufsize_t offset);
bufsize_t _scan_table_start(const unsigned char *p);

#define scan_table_start(c, l, n) _ext_scan_at(&_scan_table_start, c, l, n)

#ifdef __cplusplus
}
#endif
