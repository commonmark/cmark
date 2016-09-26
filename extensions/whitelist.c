#include "whitelist.h"
#include <parser.h>

static const char *whitelist[] = {
  "template", "h1", "h2", "h3", "h4", "h5", "h6", "h7", "h8", "br", "b", "i",
  "strong", "em", "a", "pre", "code", "img", "tt", "div", "ins", "del", "sup",
  "sub", "p", "ol", "ul", "table", "thead", "tbody", "tfoot", "blockquote",
  "dl", "dt", "dd", "kbd", "q", "samp", "var", "hr", "ruby", "rt", "rp", "li",
  "tr", "td", "th", "s", "strike", "summary", "details",
  NULL,
};

static int is_tag(const unsigned char *tag_data, size_t tag_size, const char *tagname)
{
  size_t i;

  if (tag_size < 3 || tag_data[0] != '<')
    return 0;

  i = 1;

  if (tag_data[i] == '/') {
    i++;
  }

  for (; i < tag_size; ++i, ++tagname) {
    if (*tagname == 0)
      break;

    if (tag_data[i] != *tagname)
      return 0;
  }

  if (i == tag_size)
    return 0;

  if (cmark_isspace(tag_data[i]) || tag_data[i] == '>')
    return 1;

  return 0;
}

static int filter(cmark_syntax_extension *ext, const unsigned char *tag, size_t tag_len) {
  const char **it;

  for (it = whitelist; *it; ++it) {
    if (is_tag(tag, tag_len, *it)) {
      return 1;
    }
  }

  return 0;
}

cmark_syntax_extension *create_whitelist_extension(void) {
  cmark_syntax_extension *ext = cmark_syntax_extension_new("whitelist");
  cmark_syntax_extension_set_html_filter_func(ext, filter);
  return ext;
}
