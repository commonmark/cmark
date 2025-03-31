#include <cmark.h>
#include "test_utils.h"

int test_link_basic() {
  return test_xml(
    "This is a [link](http://example.com)",
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
    "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
    "  <paragraph>\n"
    "    <text xml:space=\"preserve\">This is a </text>\n"
    "    <link destination=\"http://example.com\">\n"
    "      <text xml:space=\"preserve\">link</text>\n"
    "    </link>\n"
    "  </paragraph>\n"
    "</document>\n",
    CMARK_OPT_DEFAULT);
}

int test_link_with_title() {
  return test_xml(
    "Here's a [link](http://example.com \"Example Site\")",
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
    "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
    "  <paragraph>\n"
    "    <text xml:space=\"preserve\">Here's a </text>\n"
    "    <link destination=\"http://example.com\" title=\"Example Site\">\n"
    "      <text xml:space=\"preserve\">link</text>\n"
    "    </link>\n"
    "  </paragraph>\n"
    "</document>\n",
    CMARK_OPT_DEFAULT);
}

int test_reference_link() {
  return test_xml(
    "This is a [reference link][1]\n\n[1]: http://example.com",
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
    "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
    "  <paragraph>\n"
    "    <text xml:space=\"preserve\">This is a </text>\n"
    "    <link destination=\"http://example.com\">\n"
    "      <text xml:space=\"preserve\">reference link</text>\n"
    "    </link>\n"
    "  </paragraph>\n"
    "</document>\n",
    CMARK_OPT_DEFAULT);
}

int test_link_with_escaped_brackets() {
  return test_xml(
    "This link has \\[escaped brackets\\]: [link](http://example.com)",
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
    "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
    "  <paragraph>\n"
    "    <text xml:space=\"preserve\">This link has [escaped brackets]: </text>\n"
    "    <link destination=\"http://example.com\">\n"
    "      <text xml:space=\"preserve\">link</text>\n"
    "    </link>\n"
    "  </paragraph>\n"
    "</document>\n",
    CMARK_OPT_DEFAULT);
}

int test_link_invalid() {
  return test_xml(
    "This is not a [link(http://example.com)",
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
    "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
    "  <paragraph>\n"
    "    <text xml:space=\"preserve\">This is not a [link(http://example.com)</text>\n"
    "  </paragraph>\n"
    "</document>\n",
    CMARK_OPT_DEFAULT);
}

int test_reference_definitions_only() {
  return test_xml(
    "[ref]: http://example.com\n"
    "[other]: http://example.org",
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
    "<document xmlns=\"http://commonmark.org/xml/1.0\" />\n",
    CMARK_OPT_DEFAULT);
}

int test_reference_definitions_with_text() {
  return test_xml(
    "[ref]: http://example.com\n\n"
    "This is a [ref] link\n\n"
    "[other]: http://example.org\n\n"
    "And [other] link",
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
    "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
    "  <paragraph>\n"
    "    <text xml:space=\"preserve\">This is a </text>\n"
    "    <link destination=\"http://example.com\">\n"
    "      <text xml:space=\"preserve\">ref</text>\n"
    "    </link>\n"
    "    <text xml:space=\"preserve\"> link</text>\n"
    "  </paragraph>\n"
    "  <paragraph>\n"
    "    <text xml:space=\"preserve\">And </text>\n"
    "    <link destination=\"http://example.org\">\n"
    "      <text xml:space=\"preserve\">other</text>\n"
    "    </link>\n"
    "    <text xml:space=\"preserve\"> link</text>\n"
    "  </paragraph>\n"
    "</document>\n",
    CMARK_OPT_DEFAULT);
}

int main() {
  CASE(test_link_basic);
  CASE(test_link_with_title);
  CASE(test_reference_link);
  CASE(test_link_with_escaped_brackets);
  CASE(test_link_invalid);
  CASE(test_reference_definitions_only);
  CASE(test_reference_definitions_with_text);
  return 0;
}
