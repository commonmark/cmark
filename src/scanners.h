#include "cmark.h"
#include "chunk.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pair {
  void *first;
  void *second;
} pair;

bufsize_t _scan_at(bufsize_t (*scanner)(const unsigned char *), cmark_chunk *c,
                   bufsize_t offset);
// @d is the user data.
bufsize_t _scan_at_ext(bufsize_t (*scanner)(const unsigned char *, void *), cmark_chunk *c,
                       bufsize_t offset, void *d);
bufsize_t _scan_scheme(const unsigned char *p);
bufsize_t _scan_autolink_uri(const unsigned char *p);
bufsize_t _scan_autolink_email(const unsigned char *p);
bufsize_t _scan_html_tag(const unsigned char *p);
bufsize_t _scan_html_comment(const unsigned char *p);
bufsize_t _scan_html_pi(const unsigned char *p);
bufsize_t _scan_html_declaration(const unsigned char *p);
bufsize_t _scan_html_cdata(const unsigned char *p);
bufsize_t _scan_html_block_start(const unsigned char *p);
bufsize_t _scan_html_block_start_7(const unsigned char *p);
bufsize_t _scan_html_block_end_1(const unsigned char *p);
bufsize_t _scan_html_block_end_2(const unsigned char *p);
bufsize_t _scan_html_block_end_3(const unsigned char *p);
bufsize_t _scan_html_block_end_4(const unsigned char *p);
bufsize_t _scan_html_block_end_5(const unsigned char *p);
bufsize_t _scan_link_title(const unsigned char *p);
bufsize_t _scan_image_size(const unsigned char *p, void *d);
bufsize_t _scan_spacechars(const unsigned char *p);
bufsize_t _scan_atx_heading_start(const unsigned char *p);
bufsize_t _scan_setext_heading_line(const unsigned char *p);
bufsize_t _scan_open_code_fence(const unsigned char *p);
bufsize_t _scan_close_code_fence(const unsigned char *p);
bufsize_t _scan_dangerous_url(const unsigned char *p);
bufsize_t _scan_formula_block_start(const unsigned char *p);
bufsize_t _scan_formula_block_end(const unsigned char *p);

#define scan_scheme(c, n) _scan_at(&_scan_scheme, c, n)
#define scan_autolink_uri(c, n) _scan_at(&_scan_autolink_uri, c, n)
#define scan_autolink_email(c, n) _scan_at(&_scan_autolink_email, c, n)
#define scan_html_tag(c, n) _scan_at(&_scan_html_tag, c, n)
#define scan_html_comment(c, n) _scan_at(&_scan_html_comment, c, n)
#define scan_html_pi(c, n) _scan_at(&_scan_html_pi, c, n)
#define scan_html_declaration(c, n) _scan_at(&_scan_html_declaration, c, n)
#define scan_html_cdata(c, n) _scan_at(&_scan_html_cdata, c, n)
#define scan_html_block_start(c, n) _scan_at(&_scan_html_block_start, c, n)
#define scan_html_block_start_7(c, n) _scan_at(&_scan_html_block_start_7, c, n)
#define scan_html_block_end_1(c, n) _scan_at(&_scan_html_block_end_1, c, n)
#define scan_html_block_end_2(c, n) _scan_at(&_scan_html_block_end_2, c, n)
#define scan_html_block_end_3(c, n) _scan_at(&_scan_html_block_end_3, c, n)
#define scan_html_block_end_4(c, n) _scan_at(&_scan_html_block_end_4, c, n)
#define scan_html_block_end_5(c, n) _scan_at(&_scan_html_block_end_5, c, n)
#define scan_link_title(c, n) _scan_at(&_scan_link_title, c, n)
#define scan_image_size(c, n, d) _scan_at_ext(&_scan_image_size, c, n, d)
#define scan_spacechars(c, n) _scan_at(&_scan_spacechars, c, n)
#define scan_atx_heading_start(c, n) _scan_at(&_scan_atx_heading_start, c, n)
#define scan_setext_heading_line(c, n)                                         \
  _scan_at(&_scan_setext_heading_line, c, n)
#define scan_open_code_fence(c, n) _scan_at(&_scan_open_code_fence, c, n)
#define scan_close_code_fence(c, n) _scan_at(&_scan_close_code_fence, c, n)
#define scan_dangerous_url(c, n) _scan_at(&_scan_dangerous_url, c, n)
#define scan_formula_block_start(c, n) _scan_at(&_scan_formula_block_start, c, n)
#define scan_formula_block_end(c, n) _scan_at(&_scan_formula_block_end, c, n)

#ifdef __cplusplus
}
#endif
