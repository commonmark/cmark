#include "cmark.h"
#include "test_utils.h"

static int test_basic_blockquote() {
    const char *markdown = "> This is a blockquote.\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                          "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                          "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                          "  <block_quote>\n"
                          "    <paragraph>\n"
                          "      <text xml:space=\"preserve\">This is a blockquote.</text>\n"
                          "    </paragraph>\n"
                          "  </block_quote>\n"
                          "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

static int test_blockquote_with_fenced_code() {
    const char *markdown = "> ```\n> function example() {\n>   return true;\n> }\n> ```\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                          "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                          "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                          "  <block_quote>\n"
                          "    <code_block xml:space=\"preserve\">function example() {\n  return true;\n}\n</code_block>\n"
                          "  </block_quote>\n"
                          "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

static int test_blockquote_with_fenced_code_source() {
    const char *markdown = "> ```\n> function example() {\n>   return true;\n> }\n> ```\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                          "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                          "<document sourcepos=\"1:1-5:5\" xmlns=\"http://commonmark.org/xml/1.0\">\n"
                          "  <block_quote sourcepos=\"1:1-5:5\">\n"
                          "    <code_block sourcepos=\"1:3-5:5\" xml:space=\"preserve\">function example() {\n  return true;\n}\n</code_block>\n"
                          "  </block_quote>\n"
                          "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_SOURCEPOS);
}

static int test_blockquote_with_language_specific_code() {
    const char *markdown = "> ```javascript\n> function example() {\n>   return true;\n> }\n> ```\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                          "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                          "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                          "  <block_quote>\n"
                          "    <code_block info=\"javascript\" xml:space=\"preserve\">function example() {\n  return true;\n}\n</code_block>\n"
                          "  </block_quote>\n"
                          "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

static int test_blockquote_with_text_and_code() {
    const char *markdown = "> Here is some text.\n>\n> ```\n> function example() {\n>   return true;\n> }\n> ```\n>\n> And more text after.\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                          "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                          "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                          "  <block_quote>\n"
                          "    <paragraph>\n"
                          "      <text xml:space=\"preserve\">Here is some text.</text>\n"
                          "    </paragraph>\n"
                          "    <code_block xml:space=\"preserve\">function example() {\n  return true;\n}\n</code_block>\n"
                          "    <paragraph>\n"
                          "      <text xml:space=\"preserve\">And more text after.</text>\n"
                          "    </paragraph>\n"
                          "  </block_quote>\n"
                          "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

static int test_nested_blockquote_with_code() {
    const char *markdown = "> Outer quote\n>\n> > Inner quote\n> >\n> > ```\n> > nested code\n> > ```\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                          "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                          "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                          "  <block_quote>\n"
                          "    <paragraph>\n"
                          "      <text xml:space=\"preserve\">Outer quote</text>\n"
                          "    </paragraph>\n"
                          "    <block_quote>\n"
                          "      <paragraph>\n"
                          "        <text xml:space=\"preserve\">Inner quote</text>\n"
                          "      </paragraph>\n"
                          "      <code_block xml:space=\"preserve\">nested code\n</code_block>\n"
                          "    </block_quote>\n"
                          "  </block_quote>\n"
                          "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

static int test_blockquote_with_tildes() {
    const char *markdown = "> ~~~\n> function example() {\n>   return true;\n> }\n> ~~~\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                          "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                          "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                          "  <block_quote>\n"
                          "    <code_block xml:space=\"preserve\">function example() {\n  return true;\n}\n</code_block>\n"
                          "  </block_quote>\n"
                          "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

static int test_loose_blockquote_markers() {
    const char *markdown = ">```\n>code\n>```\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                          "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                          "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                          "  <block_quote>\n"
                          "    <code_block xml:space=\"preserve\">code\n</code_block>\n"
                          "  </block_quote>\n"
                          "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

static int test_code_without_blockquote_markers() {
    const char *markdown = "> ```\ncode\n```\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                          "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                          "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                          "  <block_quote>\n"
                          "    <code_block xml:space=\"preserve\"></code_block>\n"
                          "  </block_quote>\n"
                          "  <paragraph>\n"
                          "    <text xml:space=\"preserve\">code</text>\n"
                          "  </paragraph>\n"
                          "  <code_block xml:space=\"preserve\"></code_block>\n"
                          "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

int main() {
    CASE(test_basic_blockquote);
    CASE(test_blockquote_with_fenced_code);
    CASE(test_blockquote_with_fenced_code_source);
    CASE(test_blockquote_with_language_specific_code);
    CASE(test_blockquote_with_text_and_code);
    CASE(test_nested_blockquote_with_code);
    CASE(test_blockquote_with_tildes);
    CASE(test_loose_blockquote_markers);
    CASE(test_code_without_blockquote_markers);
    return 0;
}
