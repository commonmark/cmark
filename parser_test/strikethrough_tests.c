#include <cmark.h>

#include "test_utils.h"

int test_strikethrough_simple() {
  return test_xml("~~strikethrough~~",
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
      "<document sourcepos=\"1:1-1:17\" xmlns=\"http://commonmark.org/xml/1.0\">\n"
      "  <paragraph sourcepos=\"1:1-1:17\">\n"
      "    <strikethrough sourcepos=\"1:1-1:17\">\n"
      "      <text sourcepos=\"1:3-1:15\" xml:space=\"preserve\">strikethrough</text>\n"
      "    </strikethrough>\n"
      "  </paragraph>\n"
      "</document>\n",
      CMARK_OPT_SOURCEPOS);
}

int test_strikethrough_multiple() {
  return test_xml("~~strikethrough~~ and ~~another~~",
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
      "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
      "  <paragraph>\n"
      "    <strikethrough>\n"
      "      <text xml:space=\"preserve\">strikethrough</text>\n"
      "    </strikethrough>\n"
      "    <text xml:space=\"preserve\"> and </text>\n"
      "    <strikethrough>\n"
      "      <text xml:space=\"preserve\">another</text>\n"
      "    </strikethrough>\n"
      "  </paragraph>\n"
      "</document>\n",
      CMARK_OPT_DEFAULT);
}

int test_strikethrough_nested() {
  return test_xml("~~strikethrough *with emphasis*~~",
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
      "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
      "  <paragraph>\n"
      "    <strikethrough>\n"
      "      <text xml:space=\"preserve\">strikethrough </text>\n"
      "      <emph>\n"
      "        <text xml:space=\"preserve\">with emphasis</text>\n"
      "      </emph>\n"
      "    </strikethrough>\n"
      "  </paragraph>\n"
      "</document>\n",
      CMARK_OPT_DEFAULT);
}

int test_strikethrough_incomplete() {
  return test_xml("~strikethrough~",
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
      "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
      "  <paragraph>\n"
      "    <text xml:space=\"preserve\">~strikethrough~</text>\n"
      "  </paragraph>\n"
      "</document>\n",
      CMARK_OPT_DEFAULT);
}

int test_strikethrough_not_closed() {
  return test_xml("~~strikethrough",
      "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
      "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
      "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
      "  <paragraph>\n"
      "    <text xml:space=\"preserve\">~~strikethrough</text>\n"
      "  </paragraph>\n"
      "</document>\n",
      CMARK_OPT_DEFAULT);
}

int main() {
  CASE(test_strikethrough_simple);
  CASE(test_strikethrough_multiple);
  CASE(test_strikethrough_nested);
  CASE(test_strikethrough_incomplete);
  CASE(test_strikethrough_not_closed);
  return 0;
}
