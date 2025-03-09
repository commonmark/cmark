#include "test_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cmark.h>

int test_xml(const char *markdown, const char *expected_xml, int options) {
    cmark_node *doc = cmark_parse_document(markdown, strlen(markdown), options);
    char *actual_xml = cmark_render_xml(doc, options);

    int ret = 1;
    if (strlen(actual_xml) != strlen(expected_xml) ||
        strcmp(actual_xml, expected_xml) != 0) {
        fprintf(stderr,
            "Test failed!\nExpected:\n%s\nActual:\n%s\n",
            expected_xml, actual_xml);
        ret = 0;
    }

    free(actual_xml);
    cmark_node_free(doc);
    return ret;
}
