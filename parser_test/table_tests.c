#include <cmark.h>
#include "test_utils.h"

int test_table_basic() {
  return test_xml(
    "the text before the table\n\n"
    "| foo | bar | zoo |\n"
    "| :--- | :---: | ---: |\n"
    "| baz | bim | xyz |\n"
    "the text after the table",
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
    "<document sourcepos=\"1:1-6:24\" xmlns=\"http://commonmark.org/xml/1.0\">\n"
    "  <paragraph sourcepos=\"1:1-1:25\">\n"
    "    <text sourcepos=\"1:1-1:25\" xml:space=\"preserve\">the text before the table</text>\n"
    "  </paragraph>\n"
    "  <table sourcepos=\"3:1-5:19\" columns=\"3\">\n"
    "    <table_row sourcepos=\"3:1-3:19\" type=\"header\">\n"
    "      <table_cell sourcepos=\"3:2-3:6\" align=\"left\">\n"
    "        <text sourcepos=\"3:2-3:5\" xml:space=\"preserve\"> foo</text>\n"
    "      </table_cell>\n"
    "      <table_cell sourcepos=\"3:8-3:12\" align=\"center\">\n"
    "        <text sourcepos=\"3:8-3:11\" xml:space=\"preserve\"> bar</text>\n"
    "      </table_cell>\n"
    "      <table_cell sourcepos=\"3:14-3:18\" align=\"right\">\n"
    "        <text sourcepos=\"3:14-3:17\" xml:space=\"preserve\"> zoo</text>\n"
    "      </table_cell>\n"
    "    </table_row>\n"
    "    <table_row sourcepos=\"4:1-4:23\" type=\"delimiter\">\n"
    "      <table_cell sourcepos=\"4:2-4:7\" align=\"left\" />\n"
    "      <table_cell sourcepos=\"4:9-4:15\" align=\"center\" />\n"
    "      <table_cell sourcepos=\"4:17-4:22\" align=\"right\" />\n"
    "    </table_row>\n"
    "    <table_row sourcepos=\"5:1-5:19\" type=\"data\">\n"
    "      <table_cell sourcepos=\"5:2-5:6\" align=\"left\">\n"
    "        <text sourcepos=\"5:2-5:5\" xml:space=\"preserve\"> baz</text>\n"
    "      </table_cell>\n"
    "      <table_cell sourcepos=\"5:8-5:12\" align=\"center\">\n"
    "        <text sourcepos=\"5:8-5:11\" xml:space=\"preserve\"> bim</text>\n"
    "      </table_cell>\n"
    "      <table_cell sourcepos=\"5:14-5:18\" align=\"right\">\n"
    "        <text sourcepos=\"5:14-5:17\" xml:space=\"preserve\"> xyz</text>\n"
    "      </table_cell>\n"
    "    </table_row>\n"
    "  </table>\n"
    "  <paragraph sourcepos=\"6:1-6:24\">\n"
    "    <text sourcepos=\"6:1-6:24\" xml:space=\"preserve\">the text after the table</text>\n"
    "  </paragraph>\n"
    "</document>\n",
    CMARK_OPT_SOURCEPOS);
}

int test_table_alignments() {
  return test_xml(
    "| left | center | right |\n"
    "|:-----|:------:|------:|\n"
    "| a    |    b   |     c |",
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
    "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
    "  <table columns=\"3\">\n"
    "    <table_row type=\"header\">\n"
    "      <table_cell align=\"left\">\n"
    "        <text xml:space=\"preserve\"> left</text>\n"
    "      </table_cell>\n"
    "      <table_cell align=\"center\">\n"
    "        <text xml:space=\"preserve\"> center</text>\n"
    "      </table_cell>\n"
    "      <table_cell align=\"right\">\n"
    "        <text xml:space=\"preserve\"> right</text>\n"
    "      </table_cell>\n"
    "    </table_row>\n"
    "    <table_row type=\"delimiter\">\n"
    "      <table_cell align=\"left\" />\n"
    "      <table_cell align=\"center\" />\n"
    "      <table_cell align=\"right\" />\n"
    "    </table_row>\n"
    "    <table_row type=\"data\">\n"
    "      <table_cell align=\"left\">\n"
    "        <text xml:space=\"preserve\"> a</text>\n"
    "      </table_cell>\n"
    "      <table_cell align=\"center\">\n"
    "        <text xml:space=\"preserve\">    b</text>\n"
    "      </table_cell>\n"
    "      <table_cell align=\"right\">\n"
    "        <text xml:space=\"preserve\">     c</text>\n"
    "      </table_cell>\n"
    "    </table_row>\n"
    "  </table>\n"
    "</document>\n",
    CMARK_OPT_DEFAULT);
}

int test_table_empty_cells() {
  return test_xml(
    "| a | b |\n"
    "| --- | --- |\n"
    "|  | d |",
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
    "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
    "  <table columns=\"2\">\n"
    "    <table_row type=\"header\">\n"
    "      <table_cell align=\"none\">\n"
    "        <text xml:space=\"preserve\"> a</text>\n"
    "      </table_cell>\n"
    "      <table_cell align=\"none\">\n"
    "        <text xml:space=\"preserve\"> b</text>\n"
    "      </table_cell>\n"
    "    </table_row>\n"
    "    <table_row type=\"delimiter\">\n"
    "      <table_cell align=\"none\" />\n"
    "      <table_cell align=\"none\" />\n"
    "    </table_row>\n"
    "    <table_row type=\"data\">\n"
    "      <table_cell align=\"none\" />\n"
    "      <table_cell align=\"none\">\n"
    "        <text xml:space=\"preserve\"> d</text>\n"
    "      </table_cell>\n"
    "    </table_row>\n"
    "  </table>\n"
    "</document>\n",
    CMARK_OPT_DEFAULT);
}

int test_table_escaped_pipes() {
  return test_xml(
    "| a \\| b | c |\n"
    "| --- | --- |\n"
    "| d | e \\| f |",
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
    "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
    "  <table columns=\"2\">\n"
    "    <table_row type=\"header\">\n"
    "      <table_cell align=\"none\">\n"
    "        <text xml:space=\"preserve\"> a | b</text>\n"
    "      </table_cell>\n"
    "      <table_cell align=\"none\">\n"
    "        <text xml:space=\"preserve\"> c</text>\n"
    "      </table_cell>\n"
    "    </table_row>\n"
    "    <table_row type=\"delimiter\">\n"
    "      <table_cell align=\"none\" />\n"
    "      <table_cell align=\"none\" />\n"
    "    </table_row>\n"
    "    <table_row type=\"data\">\n"
    "      <table_cell align=\"none\">\n"
    "        <text xml:space=\"preserve\"> d</text>\n"
    "      </table_cell>\n"
    "      <table_cell align=\"none\">\n"
    "        <text xml:space=\"preserve\"> e | f</text>\n"
    "      </table_cell>\n"
    "    </table_row>\n"
    "  </table>\n"
    "</document>\n",
    CMARK_OPT_DEFAULT);
}

int test_table_invalid_no_header() {
  return test_xml(
    "| a | b |\n"
    "| d | e |",
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
    "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
    "  <paragraph>\n"
    "    <text xml:space=\"preserve\">| a | b |</text>\n"
    "    <softbreak />\n"
    "    <text xml:space=\"preserve\">| d | e |</text>\n"
    "  </paragraph>\n"
    "</document>\n",
    CMARK_OPT_DEFAULT);
}

int test_table_invalid_delimiter() {
  return test_xml(
    "| a | b |\n"
    "| -- | --- - |\n"
    "| c | d |",
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
    "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
    "  <paragraph>\n"
    "    <text xml:space=\"preserve\">| a | b |</text>\n"
    "    <softbreak />\n"
    "    <text xml:space=\"preserve\">| -- | --- - |</text>\n"
    "    <softbreak />\n"
    "    <text xml:space=\"preserve\">| c | d |</text>\n"
    "  </paragraph>\n"
    "</document>\n",
    CMARK_OPT_DEFAULT);
}

int test_table_no_leading_pipe() {
  return test_xml(
    "foo | bar |\n"
    "--- | --- |\n"
    "baz | bim |",
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
    "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
    "  <paragraph>\n"
    "    <text xml:space=\"preserve\">foo | bar |</text>\n"
    "    <softbreak />\n"
    "    <text xml:space=\"preserve\">--- | --- |</text>\n"
    "    <softbreak />\n"
    "    <text xml:space=\"preserve\">baz | bim |</text>\n"
    "  </paragraph>\n"
    "</document>\n",
    CMARK_OPT_DEFAULT);
}

int test_table_mismatched_columns() {
  return test_xml(
    "| a | b | c |\n"
    "| --- | --- |\n"
    "| d | e |",
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
    "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
    "  <paragraph>\n"
    "    <text xml:space=\"preserve\">| a | b | c |</text>\n"
    "    <softbreak />\n"
    "    <text xml:space=\"preserve\">| --- | --- |</text>\n"
    "    <softbreak />\n"
    "    <text xml:space=\"preserve\">| d | e |</text>\n"
    "  </paragraph>\n"
    "</document>\n",
    CMARK_OPT_DEFAULT);
}

int test_table_with_inline_markdown() {
  return test_xml(
    "| *em* | **strong** |\n"
    "| --- | --- |\n"
    "| `code` | [link](url) |",
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
    "<document sourcepos=\"1:1-3:24\" xmlns=\"http://commonmark.org/xml/1.0\">\n"
    "  <table sourcepos=\"1:1-3:24\" columns=\"2\">\n"
    "    <table_row sourcepos=\"1:1-1:21\" type=\"header\">\n"
    "      <table_cell sourcepos=\"1:2-1:7\" align=\"none\">\n"
    "        <text sourcepos=\"1:2-1:2\" xml:space=\"preserve\"> </text>\n"
    "        <emph sourcepos=\"1:3-1:6\">\n"
    "          <text sourcepos=\"1:4-1:5\" xml:space=\"preserve\">em</text>\n"
    "        </emph>\n"
    "      </table_cell>\n"
    "      <table_cell sourcepos=\"1:9-1:20\" align=\"none\">\n"
    "        <text sourcepos=\"1:9-1:9\" xml:space=\"preserve\"> </text>\n"
    "        <strong sourcepos=\"1:10-1:19\">\n"
    "          <text sourcepos=\"1:12-1:17\" xml:space=\"preserve\">strong</text>\n"
    "        </strong>\n"
    "      </table_cell>\n"
    "    </table_row>\n"
    "    <table_row sourcepos=\"2:1-2:13\" type=\"delimiter\">\n"
    "      <table_cell sourcepos=\"2:2-2:6\" align=\"none\" />\n"
    "      <table_cell sourcepos=\"2:8-2:12\" align=\"none\" />\n"
    "    </table_row>\n"
    "    <table_row sourcepos=\"3:1-3:24\" type=\"data\">\n"
    "      <table_cell sourcepos=\"3:2-3:9\" align=\"none\">\n"
    "        <text sourcepos=\"3:2-3:2\" xml:space=\"preserve\"> </text>\n"
    "        <code sourcepos=\"3:3-3:8\" xml:space=\"preserve\">code</code>\n"
    "      </table_cell>\n"
    "      <table_cell sourcepos=\"3:11-3:23\" align=\"none\">\n"
    "        <text sourcepos=\"3:11-3:11\" xml:space=\"preserve\"> </text>\n"
    "        <link sourcepos=\"3:12-3:22\" destination=\"url\">\n"
    "          <text sourcepos=\"3:13-3:16\" xml:space=\"preserve\">link</text>\n"
    "        </link>\n"
    "      </table_cell>\n"
    "    </table_row>\n"
    "  </table>\n"
    "</document>\n",
    CMARK_OPT_SOURCEPOS);
}

int main() {
  CASE(test_table_basic);
  CASE(test_table_alignments);
  CASE(test_table_empty_cells);
  CASE(test_table_escaped_pipes);
  CASE(test_table_invalid_no_header);
  CASE(test_table_invalid_delimiter);
  CASE(test_table_no_leading_pipe);
  CASE(test_table_mismatched_columns);
  CASE(test_table_with_inline_markdown);
  return 0;
}
