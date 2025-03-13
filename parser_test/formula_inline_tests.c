#include <cmark.h>

#include "test_utils.h"

int test_formula_inline_simple() {
  return test_xml("$E=mc^2$",
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
      "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
      "  <paragraph>\n"
      "    <formula_inline xml:space=\"preserve\">E=mc^2</formula_inline>\n"
      "  </paragraph>\n"
      "</document>\n",
      CMARK_OPT_DEFAULT);
}

int test_formula_inline_multiple() {
  return test_xml("$a+b$ and $c+d$",
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
      "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
      "  <paragraph>\n"
      "    <formula_inline xml:space=\"preserve\">a+b</formula_inline>\n"
      "    <text xml:space=\"preserve\"> and </text>\n"
      "    <formula_inline xml:space=\"preserve\">c+d</formula_inline>\n"
      "  </paragraph>\n"
      "</document>\n",
      CMARK_OPT_DEFAULT);
}

int test_formula_inline_with_escape() {
  return test_xml("$a\\$b$",
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
      "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
      "  <paragraph>\n"
      "    <formula_inline xml:space=\"preserve\">a$b</formula_inline>\n"
      "  </paragraph>\n"
      "</document>\n",
      CMARK_OPT_DEFAULT);
}

int test_formula_inline_not_closed() {
  return test_xml("$formula",
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
      "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
      "  <paragraph>\n"
      "    <text xml:space=\"preserve\">$formula</text>\n"
      "  </paragraph>\n"
      "</document>\n",
      CMARK_OPT_DEFAULT);
}

int test_formula_inline_empty() {
  return test_xml("$$",
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
      "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
      "  <paragraph>\n"
      "    <text xml:space=\"preserve\">$$</text>\n"
      "  </paragraph>\n"
      "</document>\n",
      CMARK_OPT_DEFAULT);
}

int main() {
  CASE(test_formula_inline_simple);
  CASE(test_formula_inline_multiple);
  CASE(test_formula_inline_with_escape);
  CASE(test_formula_inline_not_closed);
  CASE(test_formula_inline_empty);
  return 0;
}