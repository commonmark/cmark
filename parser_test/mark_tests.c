#include <cmark.h>

#include "test_utils.h"

int test_mark_simple() {
  return test_xml("==mark==",
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
      "<document sourcepos=\"1:1-1:8\" xmlns=\"http://commonmark.org/xml/1.0\">\n"
      "  <paragraph sourcepos=\"1:1-1:8\">\n"
      "    <mark sourcepos=\"1:1-1:8\">\n"
      "      <text sourcepos=\"1:3-1:6\" xml:space=\"preserve\">mark</text>\n"
      "    </mark>\n"
      "  </paragraph>\n"
      "</document>\n",
      CMARK_OPT_SOURCEPOS);
}

int test_mark_multiple() {
  return test_xml("==mark== and ==another==",
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
      "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
      "  <paragraph>\n"
      "    <mark>\n"
      "      <text xml:space=\"preserve\">mark</text>\n"
      "    </mark>\n"
      "    <text xml:space=\"preserve\"> and </text>\n"
      "    <mark>\n"
      "      <text xml:space=\"preserve\">another</text>\n"
      "    </mark>\n"
      "  </paragraph>\n"
      "</document>\n",
      CMARK_OPT_DEFAULT);
}

int test_mark_nested() {
  return test_xml("==mark *with emphasis*==",
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
      "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
      "  <paragraph>\n"
      "    <mark>\n"
      "      <text xml:space=\"preserve\">mark </text>\n"
      "      <emph>\n"
      "        <text xml:space=\"preserve\">with emphasis</text>\n"
      "      </emph>\n"
      "    </mark>\n"
      "  </paragraph>\n"
      "</document>\n",
      CMARK_OPT_DEFAULT);
}

int test_mark_incomplete() {
  return test_xml("~mark~",
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
      "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
      "  <paragraph>\n"
      "    <text xml:space=\"preserve\">~mark~</text>\n"
      "  </paragraph>\n"
      "</document>\n",
      CMARK_OPT_DEFAULT);
}

int test_mark_not_closed() {
  return test_xml("==mark",
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
      "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
      "  <paragraph>\n"
      "    <text xml:space=\"preserve\">==mark</text>\n"
      "  </paragraph>\n"
      "</document>\n",
      CMARK_OPT_DEFAULT);
}

int main() {
  CASE(test_mark_simple);
  CASE(test_mark_multiple);
  CASE(test_mark_nested);
  CASE(test_mark_incomplete);
  CASE(test_mark_not_closed);
  return 0;
}
