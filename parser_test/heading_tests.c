#include "cmark.h"
#include "test_utils.h"

static int test_basic_heading() {
    const char *markdown = "# Heading 1\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                          "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                          "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                          "  <heading level=\"1\">\n"
                          "    <text xml:space=\"preserve\">Heading 1</text>\n"
                          "  </heading>\n"
                          "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

static int test_multiple_heading_levels() {
    const char *markdown = "# Heading 1\n## Heading 2\n### Heading 3\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                          "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                          "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                          "  <heading level=\"1\">\n"
                          "    <text xml:space=\"preserve\">Heading 1</text>\n"
                          "  </heading>\n"
                          "  <heading level=\"2\">\n"
                          "    <text xml:space=\"preserve\">Heading 2</text>\n"
                          "  </heading>\n"
                          "  <heading level=\"3\">\n"
                          "    <text xml:space=\"preserve\">Heading 3</text>\n"
                          "  </heading>\n"
                          "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

static int test_heading_source_positions() {
    const char *markdown = "# Heading 1\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                          "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                          "<document sourcepos=\"1:1-1:11\" xmlns=\"http://commonmark.org/xml/1.0\">\n"
                          "  <heading sourcepos=\"1:1-1:11\" level=\"1\">\n"
                          "    <text sourcepos=\"1:3-1:11\" xml:space=\"preserve\">Heading 1</text>\n"
                          "  </heading>\n"
                          "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_SOURCEPOS);
}

static int test_heading_with_emphasis() {
    const char *markdown = "# Heading with *emphasis*\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                          "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                          "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                          "  <heading level=\"1\">\n"
                          "    <text xml:space=\"preserve\">Heading with </text>\n"
                          "    <emph>\n"
                          "      <text xml:space=\"preserve\">emphasis</text>\n"
                          "    </emph>\n"
                          "  </heading>\n"
                          "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

static int test_setext_headings() {
    const char *markdown = "Heading 1\n=========\nHeading 2\n---------\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                          "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                          "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                          "  <heading level=\"1\">\n"
                          "    <text xml:space=\"preserve\">Heading 1</text>\n"
                          "  </heading>\n"
                          "  <heading level=\"2\">\n"
                          "    <text xml:space=\"preserve\">Heading 2</text>\n"
                          "  </heading>\n"
                          "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

int main() {
    CASE(test_basic_heading);
    CASE(test_multiple_heading_levels);
    CASE(test_heading_source_positions);
    CASE(test_heading_with_emphasis);
    CASE(test_setext_headings);
    return 0;
}
