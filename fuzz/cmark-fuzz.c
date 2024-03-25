/* for fmemopen */
#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmark.h"

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  struct __attribute__((packed)) {
    int options;
    int width;
  } fuzz_config;

  if (size >= sizeof(fuzz_config)) {
    /* The beginning of `data` is treated as fuzzer configuration */
    memcpy(&fuzz_config, data, sizeof(fuzz_config));
    int options = fuzz_config.options;

    /* Mask off valid option bits */
    options &= (CMARK_OPT_SOURCEPOS | CMARK_OPT_HARDBREAKS | CMARK_OPT_UNSAFE | CMARK_OPT_NOBREAKS | CMARK_OPT_NORMALIZE | CMARK_OPT_VALIDATE_UTF8 | CMARK_OPT_SMART);

    /* Remainder of input is the markdown */
    const char *markdown = (const char *)(data + sizeof(fuzz_config));
    size_t markdown_size = size - sizeof(fuzz_config);
    cmark_node *doc = NULL;

    /* Use upper bits of options to select parsing mode */
    switch (((unsigned) fuzz_config.options >> 30) & 3) {
      case 0:
        doc = cmark_parse_document(markdown, markdown_size, options);
        break;

      case 1:
        if (markdown_size > 0) {
          FILE *file = fmemopen((void *) markdown, markdown_size, "r");
          doc = cmark_parse_file(file, options);
          fclose(file);
        }
        break;

      case 2: {
        size_t block_max = 20;
        cmark_parser *parser = cmark_parser_new(options);

        while (markdown_size > 0) {
          size_t block_size = markdown_size > block_max ? block_max : markdown_size;
          cmark_parser_feed(parser, markdown, block_size);
          markdown += block_size;
          markdown_size -= block_size;
        }

        doc = cmark_parser_finish(parser);
        cmark_parser_free(parser);
        break;
      }

      case 3:
        free(cmark_markdown_to_html(markdown, markdown_size, options));
        break;
    }

    if (doc != NULL) {
      free(cmark_render_commonmark(doc, options, fuzz_config.width));
      free(cmark_render_html(doc, options));
      free(cmark_render_latex(doc, options, fuzz_config.width));
      free(cmark_render_man(doc, options, fuzz_config.width));
      free(cmark_render_xml(doc, options));

      cmark_node_free(doc);
    }
  }
  return 0;
}
