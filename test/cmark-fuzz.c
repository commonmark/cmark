#include <stdint.h>
#include <stdlib.h>
#include "cmark.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  int options = 0;
  if (size > sizeof(options)) {
    /* First 4 bytes of input are treated as options */
    int options = *(const int *)data;

    /* Mask off valid option bits */
    options = options & (CMARK_OPT_SOURCEPOS | CMARK_OPT_HARDBREAKS | CMARK_OPT_SAFE | CMARK_OPT_NOBREAKS | CMARK_OPT_NORMALIZE | CMARK_OPT_VALIDATE_UTF8 | CMARK_OPT_SMART);

    /* Remainder of input is the markdown */
    const char *markdown = (const char *)(data + sizeof(options));
    const size_t markdown_size = size - sizeof(options);
    cmark_node *doc = cmark_parse_document(markdown, markdown_size, options);

    free(cmark_render_commonmark(doc, options, 80));
    free(cmark_render_html(doc, options));
    free(cmark_render_latex(doc, options, 80));
    free(cmark_render_man(doc, options, 80));
    free(cmark_render_xml(doc, options));

    cmark_node_free(doc);
  }
  return 0;
}
