#include <cmark.h>

#include "test_utils.h"

int test_code_simple() {
  return test_xml("`code`",
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
      "<document sourcepos=\"1:1-1:6\" xmlns=\"http://commonmark.org/xml/1.0\">\n"
      "  <paragraph sourcepos=\"1:1-1:6\">\n"
      "    <code sourcepos=\"1:1-1:6\" xml:space=\"preserve\">code</code>\n"
      "  </paragraph>\n"
      "</document>\n",
      CMARK_OPT_SOURCEPOS);
}

int test_code_multiple() {
  return test_xml("`first` and `second`",
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
      "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
      "  <paragraph>\n"
      "    <code xml:space=\"preserve\">first</code>\n"
      "    <text xml:space=\"preserve\"> and </text>\n"
      "    <code xml:space=\"preserve\">second</code>\n"
      "  </paragraph>\n"
      "</document>\n",
      CMARK_OPT_DEFAULT);
}

int test_code_with_spaces() {
  return test_xml("`  code  with  spaces  `",
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
      "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
      "  <paragraph>\n"
      "    <code xml:space=\"preserve\"> code  with  spaces </code>\n"
      "  </paragraph>\n"
      "</document>\n",
      CMARK_OPT_DEFAULT);
}

int test_code_double_backticks() {
  return test_xml("``code with `backticks` inside``",
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
      "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
      "  <paragraph>\n"
      "    <code xml:space=\"preserve\">code with `backticks` inside</code>\n"
      "  </paragraph>\n"
      "</document>\n",
      CMARK_OPT_DEFAULT);
}

int test_code_not_closed() {
  return test_xml("`not closed",
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
      "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
      "  <paragraph>\n"
      "    <text xml:space=\"preserve\">`not closed</text>\n"
      "  </paragraph>\n"
      "</document>\n",
      CMARK_OPT_DEFAULT);
}

int test_code_empty() {
  return test_xml("``",
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
      "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
      "  <paragraph>\n"
      "    <text xml:space=\"preserve\">``</text>\n"
      "  </paragraph>\n"
      "</document>\n",
      CMARK_OPT_DEFAULT);
}

int main() {
  CASE(test_code_simple);
  CASE(test_code_multiple);
  CASE(test_code_with_spaces);
  CASE(test_code_double_backticks);
  CASE(test_code_not_closed);
  CASE(test_code_empty);
  return 0;
}
