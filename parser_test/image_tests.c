#include "cmark.h"
#include "test_utils.h"

static int test_basic_image() {
    const char *markdown = "![Alt text](image.png)\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                           "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                           "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                           "  <paragraph>\n"
                           "    <image destination=\"image.png\">\n"
                           "      <text xml:space=\"preserve\">Alt text</text>\n"
                           "    </image>\n"
                           "  </paragraph>\n"
                           "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

static int test_image_with_title() {
    const char *markdown = "![Alt text](image.png \"Image title\")\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                           "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                           "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                           "  <paragraph>\n"
                           "    <image destination=\"image.png\" title=\"Image title\">\n"
                           "      <text xml:space=\"preserve\">Alt text</text>\n"
                           "    </image>\n"
                           "  </paragraph>\n"
                           "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

static int test_image_with_width_and_height() {
    const char *markdown = "![Alt text](image.png =800x600)\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                           "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                           "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                           "  <paragraph>\n"
                           "    <image destination=\"image.png\" width=\"800\" height=\"600\">\n"
                           "      <text xml:space=\"preserve\">Alt text</text>\n"
                           "    </image>\n"
                           "  </paragraph>\n"
                           "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

static int test_image_with_width_only() {
    const char *markdown = "![Alt text](image.png =800x)\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                           "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                           "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                           "  <paragraph>\n"
                           "    <image destination=\"image.png\" width=\"800\">\n"
                           "      <text xml:space=\"preserve\">Alt text</text>\n"
                           "    </image>\n"
                           "  </paragraph>\n"
                           "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

static int test_image_with_height_only() {
    const char *markdown = "![Alt text](image.png =x600)\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                           "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                           "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                           "  <paragraph>\n"
                           "    <image destination=\"image.png\" height=\"600\">\n"
                           "      <text xml:space=\"preserve\">Alt text</text>\n"
                           "    </image>\n"
                           "  </paragraph>\n"
                           "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

static int test_image_with_title_and_dimensions() {
    const char *markdown = "![Alt text](image.png \"Image title\" =800x600)\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                           "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                           "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                           "  <paragraph>\n"
                           "    <image destination=\"image.png\" title=\"Image title\" width=\"800\" height=\"600\">\n"
                           "      <text xml:space=\"preserve\">Alt text</text>\n"
                           "    </image>\n"
                           "  </paragraph>\n"
                           "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

static int test_image_with_title_and_width_only() {
    const char *markdown = "![Alt text](image.png \"Image title\" =800x)\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                           "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                           "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                           "  <paragraph>\n"
                           "    <image destination=\"image.png\" title=\"Image title\" width=\"800\">\n"
                           "      <text xml:space=\"preserve\">Alt text</text>\n"
                           "    </image>\n"
                           "  </paragraph>\n"
                           "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

static int test_image_with_title_and_height_only() {
    const char *markdown = "![Alt text](image.png \"Image title\" =x600)\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                           "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                           "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                           "  <paragraph>\n"
                           "    <image destination=\"image.png\" title=\"Image title\" height=\"600\">\n"
                           "      <text xml:space=\"preserve\">Alt text</text>\n"
                           "    </image>\n"
                           "  </paragraph>\n"
                           "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

static int test_image_reference() {
    const char *markdown = "![Alt text][ref]\n\n[ref]: image.png\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                           "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                           "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                           "  <paragraph>\n"
                           "    <image destination=\"image.png\">\n"
                           "      <text xml:space=\"preserve\">Alt text</text>\n"
                           "    </image>\n"
                           "  </paragraph>\n"
                           "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

static int test_image_with_complex_alt_text() {
    const char *markdown = "![Alt *text* with **formatting**](image.png =800x600)\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                           "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                           "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                           "  <paragraph>\n"
                           "    <image destination=\"image.png\" width=\"800\" height=\"600\">\n"
                           "      <text xml:space=\"preserve\">Alt </text>\n"
                           "      <emph>\n"
                           "        <text xml:space=\"preserve\">text</text>\n"
                           "      </emph>\n"
                           "      <text xml:space=\"preserve\"> with </text>\n"
                           "      <strong>\n"
                           "        <text xml:space=\"preserve\">formatting</text>\n"
                           "      </strong>\n"
                           "    </image>\n"
                           "  </paragraph>\n"
                           "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

static int test_image_invalid_size_syntax() {
    // Invalid size syntax should be ignored
    const char *markdown = "![Alt text](image.png =invalid)\n";
    const char *expected = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                           "<!DOCTYPE document SYSTEM \"CommonMark.dtd\">\n"
                           "<document xmlns=\"http://commonmark.org/xml/1.0\">\n"
                           "  <paragraph>\n"
                           "    <text xml:space=\"preserve\">![Alt text](image.png =invalid)</text>\n"
                           "  </paragraph>\n"
                           "</document>\n";
    return test_xml(markdown, expected, CMARK_OPT_DEFAULT);
}

int main() {
    CASE(test_basic_image);
    CASE(test_image_with_title);
    CASE(test_image_with_width_and_height);
    CASE(test_image_with_width_only);
    CASE(test_image_with_height_only);
    CASE(test_image_with_title_and_dimensions);
    CASE(test_image_with_title_and_width_only);
    CASE(test_image_with_title_and_height_only);
    CASE(test_image_reference);
    CASE(test_image_with_complex_alt_text);
    CASE(test_image_invalid_size_syntax);
    return 0;
}
