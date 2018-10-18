#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "cmark-gfm.h"
#include "cmark-gfm-core-extensions.h"

const char *extension_names[] = {
  "autolink",
  "strikethrough",
  "table",
  "tagfilter",
  NULL,
};

int LLVMFuzzerInitialize(int *argc, char ***argv) {
  cmark_gfm_core_extensions_ensure_registered();
  return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  struct __attribute__((packed)) {
    int options;
    int width;
  } fuzz_config;

  if (size >= sizeof(fuzz_config)) {
    /* The beginning of `data` is treated as fuzzer configuration */
    memcpy(&fuzz_config, data, sizeof(fuzz_config));

    /* Mask off valid option bits */
    fuzz_config.options &= (
      CMARK_OPT_SOURCEPOS |
      CMARK_OPT_HARDBREAKS |
      CMARK_OPT_NOBREAKS |
      CMARK_OPT_NORMALIZE |
      CMARK_OPT_VALIDATE_UTF8 |
      CMARK_OPT_SMART |
      /* GFM specific options */
      CMARK_OPT_GITHUB_PRE_LANG |
      CMARK_OPT_LIBERAL_HTML_TAG |
      CMARK_OPT_FOOTNOTES |
      CMARK_OPT_STRIKETHROUGH_DOUBLE_TILDE |
      CMARK_OPT_TABLE_PREFER_STYLE_ATTRIBUTES |
      CMARK_OPT_FULL_INFO_STRING |
      CMARK_OPT_UNSAFE
    );

    /* Remainder of input is the markdown */
    const char *markdown = (const char *)(data + sizeof(fuzz_config));
    const size_t markdown_size = size - sizeof(fuzz_config);
    cmark_parser *parser = cmark_parser_new(fuzz_config.options);

    for (const char **it = extension_names; *it; ++it) {
      const char *extension_name = *it;
      cmark_syntax_extension *syntax_extension = cmark_find_syntax_extension(extension_name);
      if (!syntax_extension) {
        fprintf(stderr, "%s is not a valid syntax extension\n", extension_name);
        abort();
      }
      cmark_parser_attach_syntax_extension(parser, syntax_extension);
    }

    cmark_parser_feed(parser, markdown, markdown_size);
    cmark_node *doc = cmark_parser_finish(parser);

    free(cmark_render_commonmark(doc, fuzz_config.options, fuzz_config.width));
    free(cmark_render_html(doc, fuzz_config.options, NULL));
    free(cmark_render_latex(doc, fuzz_config.options, fuzz_config.width));
    free(cmark_render_man(doc, fuzz_config.options, fuzz_config.width));
    free(cmark_render_xml(doc, fuzz_config.options));

    cmark_node_free(doc);
    cmark_parser_free(parser);
  }
  return 0;
}
