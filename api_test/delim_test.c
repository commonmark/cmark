#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cmark.h"
#include "cmark_delim.h"
#include "harness.h"
#include "delim_test.h"

static void test_emph_asterisk(test_batch_runner *runner) {
    const char *md = "*italic*";
    cmark_node *doc = cmark_parse_document_for_delimiters(md, strlen(md), CMARK_OPT_DEFAULT);
    cmark_node *para = cmark_node_first_child(doc);
    cmark_node *emph = cmark_node_first_child(para);

    int delim_char = 0, delim_len = 0;
    int ok = cmark_node_get_delimiter_info(emph, md, strlen(md), CMARK_OPT_SOURCEPOS, &delim_char, &delim_len);

    OK(runner, ok == 1, "emph asterisk: get_delimiter_info returns 1");
    INT_EQ(runner, delim_char, '*', "emph asterisk: delim_char is *");
    INT_EQ(runner, delim_len, 1, "emph asterisk: delim_len is 1");

    cmark_node_free(doc);
}

static void test_emph_underscore(test_batch_runner *runner) {
    const char *md = "_italic_";
    cmark_node *doc = cmark_parse_document_for_delimiters(md, strlen(md), CMARK_OPT_DEFAULT);
    cmark_node *para = cmark_node_first_child(doc);
    cmark_node *emph = cmark_node_first_child(para);

    int delim_char = 0, delim_len = 0;
    int ok = cmark_node_get_delimiter_info(emph, md, strlen(md), CMARK_OPT_SOURCEPOS, &delim_char, &delim_len);

    OK(runner, ok == 1, "emph underscore: get_delimiter_info returns 1");
    INT_EQ(runner, delim_char, '_', "emph underscore: delim_char is _");
    INT_EQ(runner, delim_len, 1, "emph underscore: delim_len is 1");

    cmark_node_free(doc);
}

static void test_strong_asterisk(test_batch_runner *runner) {
    const char *md = "**bold**";
    cmark_node *doc = cmark_parse_document_for_delimiters(md, strlen(md), CMARK_OPT_DEFAULT);
    cmark_node *para = cmark_node_first_child(doc);
    cmark_node *strong = cmark_node_first_child(para);

    int delim_char = 0, delim_len = 0;
    int ok = cmark_node_get_delimiter_info(strong, md, strlen(md), CMARK_OPT_SOURCEPOS, &delim_char, &delim_len);

    OK(runner, ok == 1, "strong asterisk: get_delimiter_info returns 1");
    INT_EQ(runner, delim_char, '*', "strong asterisk: delim_char is *");
    INT_EQ(runner, delim_len, 2, "strong asterisk: delim_len is 2");

    cmark_node_free(doc);
}

static void test_strong_underscore(test_batch_runner *runner) {
    const char *md = "__bold__";
    cmark_node *doc = cmark_parse_document_for_delimiters(md, strlen(md), CMARK_OPT_DEFAULT);
    cmark_node *para = cmark_node_first_child(doc);
    cmark_node *strong = cmark_node_first_child(para);

    int delim_char = 0, delim_len = 0;
    int ok = cmark_node_get_delimiter_info(strong, md, strlen(md), CMARK_OPT_SOURCEPOS, &delim_char, &delim_len);

    OK(runner, ok == 1, "strong underscore: get_delimiter_info returns 1");
    INT_EQ(runner, delim_char, '_', "strong underscore: delim_char is _");
    INT_EQ(runner, delim_len, 2, "strong underscore: delim_len is 2");

    cmark_node_free(doc);
}

static void test_code_single_backtick(test_batch_runner *runner) {
    const char *md = "`code`";
    cmark_node *doc = cmark_parse_document_for_delimiters(md, strlen(md), CMARK_OPT_DEFAULT);
    cmark_node *para = cmark_node_first_child(doc);
    cmark_node *code = cmark_node_first_child(para);

    int delim_char = 0, delim_len = 0;
    int ok = cmark_node_get_delimiter_info(code, md, strlen(md), CMARK_OPT_SOURCEPOS, &delim_char, &delim_len);

    OK(runner, ok == 1, "code single: get_delimiter_info returns 1");
    INT_EQ(runner, delim_char, '`', "code single: delim_char is `");
    INT_EQ(runner, delim_len, 1, "code single: delim_len is 1");

    cmark_node_free(doc);
}

static void test_code_double_backtick(test_batch_runner *runner) {
    const char *md = "``code with ` inside``";
    cmark_node *doc = cmark_parse_document_for_delimiters(md, strlen(md), CMARK_OPT_DEFAULT);
    cmark_node *para = cmark_node_first_child(doc);
    cmark_node *code = cmark_node_first_child(para);

    int delim_char = 0, delim_len = 0;
    int ok = cmark_node_get_delimiter_info(code, md, strlen(md), CMARK_OPT_SOURCEPOS, &delim_char, &delim_len);

    OK(runner, ok == 1, "code double: get_delimiter_info returns 1");
    INT_EQ(runner, delim_char, '`', "code double: delim_char is `");
    INT_EQ(runner, delim_len, 2, "code double: delim_len is 2");

    cmark_node_free(doc);
}

static void test_delim_length_convenience(test_batch_runner *runner) {
    // Test cmark_node_get_delim_length convenience function
    cmark_node *emph = cmark_node_new(CMARK_NODE_EMPH);
    cmark_node *strong = cmark_node_new(CMARK_NODE_STRONG);
    cmark_node *code = cmark_node_new(CMARK_NODE_CODE);
    cmark_node *text = cmark_node_new(CMARK_NODE_TEXT);

    INT_EQ(runner, cmark_node_get_delim_length(emph), 1, "delim_length emph is 1");
    INT_EQ(runner, cmark_node_get_delim_length(strong), 2, "delim_length strong is 2");
    INT_EQ(runner, cmark_node_get_delim_length(code), 0, "delim_length code is 0 (needs source)");
    INT_EQ(runner, cmark_node_get_delim_length(text), 0, "delim_length text is 0");
    INT_EQ(runner, cmark_node_get_delim_length(NULL), 0, "delim_length NULL is 0");

    cmark_node_free(emph);
    cmark_node_free(strong);
    cmark_node_free(code);
    cmark_node_free(text);
}

static void test_delim_char_convenience(test_batch_runner *runner) {
    // Test cmark_node_get_delim_char convenience function
    const char *md = "*italic* and `code`";
    cmark_node *doc = cmark_parse_document_for_delimiters(md, strlen(md), CMARK_OPT_DEFAULT);
    cmark_node *para = cmark_node_first_child(doc);
    cmark_node *emph = cmark_node_first_child(para);
    cmark_node *code = cmark_node_last_child(para);

    INT_EQ(runner, cmark_node_get_delim_char(emph, md, strlen(md), CMARK_OPT_SOURCEPOS), '*',
           "delim_char convenience for emph");
    INT_EQ(runner, cmark_node_get_delim_char(code, md, strlen(md), CMARK_OPT_SOURCEPOS), '`',
           "delim_char convenience for code");

    cmark_node_free(doc);
}

static void test_null_handling(test_batch_runner *runner) {
    // Test NULL node handling
    int delim_char = 0, delim_len = 0;
    int ok = cmark_node_get_delimiter_info(NULL, "test", 4, CMARK_OPT_SOURCEPOS, &delim_char, &delim_len);
    OK(runner, ok == 0, "get_delimiter_info returns 0 for NULL node");

    // Test NULL source handling
    cmark_node *emph = cmark_node_new(CMARK_NODE_EMPH);
    ok = cmark_node_get_delimiter_info(emph, NULL, 0, CMARK_OPT_SOURCEPOS, &delim_char, &delim_len);
    OK(runner, ok == 0, "get_delimiter_info returns 0 for NULL source");
    cmark_node_free(emph);

    // Test non-delimiter node type
    cmark_node *text = cmark_node_new(CMARK_NODE_TEXT);
    ok = cmark_node_get_delimiter_info(text, "test", 4, CMARK_OPT_SOURCEPOS, &delim_char, &delim_len);
    OK(runner, ok == 0, "get_delimiter_info returns 0 for TEXT node");
    cmark_node_free(text);

    // Test missing CMARK_OPT_SOURCEPOS
    const char *md = "*italic*";
    cmark_node *doc = cmark_parse_document_for_delimiters(md, strlen(md), CMARK_OPT_DEFAULT);
    cmark_node *para = cmark_node_first_child(doc);
    cmark_node *emph_node = cmark_node_first_child(para);
    ok = cmark_node_get_delimiter_info(emph_node, md, strlen(md), CMARK_OPT_DEFAULT, &delim_char, &delim_len);
    OK(runner, ok == 0, "get_delimiter_info returns 0 without CMARK_OPT_SOURCEPOS");
    cmark_node_free(doc);
}

static void test_nested_emphasis(test_batch_runner *runner) {
    // Test nested emphasis: ***bold italic***
    const char *md = "***bold italic***";
    cmark_node *doc = cmark_parse_document_for_delimiters(md, strlen(md), CMARK_OPT_DEFAULT);
    cmark_node *para = cmark_node_first_child(doc);
    // Structure is: para -> emph -> strong -> text
    cmark_node *emph = cmark_node_first_child(para);
    cmark_node *strong = cmark_node_first_child(emph);

    int delim_char = 0, delim_len = 0;

    // Check outer emph
    OK(runner, cmark_node_get_type(emph) == CMARK_NODE_EMPH, "nested: outer is emph");
    cmark_node_get_delimiter_info(emph, md, strlen(md), CMARK_OPT_SOURCEPOS, &delim_char, &delim_len);
    INT_EQ(runner, delim_char, '*', "nested: emph delim_char is *");
    INT_EQ(runner, delim_len, 1, "nested: emph delim_len is 1");

    // Check inner strong
    OK(runner, cmark_node_get_type(strong) == CMARK_NODE_STRONG, "nested: inner is strong");
    cmark_node_get_delimiter_info(strong, md, strlen(md), CMARK_OPT_SOURCEPOS, &delim_char, &delim_len);
    INT_EQ(runner, delim_char, '*', "nested: strong delim_char is *");
    INT_EQ(runner, delim_len, 2, "nested: strong delim_len is 2");

    cmark_node_free(doc);
}

static void test_multiline(test_batch_runner *runner) {
    // Test emphasis spanning multiple lines
    const char *md = "line 1\n*italic\ntext*\nline 3";
    cmark_node *doc = cmark_parse_document_for_delimiters(md, strlen(md), CMARK_OPT_DEFAULT);
    cmark_node *para = cmark_node_first_child(doc);

    // Find the emph node
    cmark_node *node = cmark_node_first_child(para);
    while (node && cmark_node_get_type(node) != CMARK_NODE_EMPH) {
        node = cmark_node_next(node);
    }

    OK(runner, node != NULL, "multiline: found emph node");
    if (node) {
        int delim_char = 0, delim_len = 0;
        int ok = cmark_node_get_delimiter_info(node, md, strlen(md), CMARK_OPT_SOURCEPOS, &delim_char, &delim_len);
        OK(runner, ok == 1, "multiline: get_delimiter_info returns 1");
        INT_EQ(runner, delim_char, '*', "multiline: delim_char is *");
        INT_EQ(runner, delim_len, 1, "multiline: delim_len is 1");
    }

    cmark_node_free(doc);
}

static void test_multiline_code_then_emph(test_batch_runner *runner) {
    // Test the edge case: multi-line code span followed by emphasis
    // This requires CMARK_OPT_SOURCEPOS to track positions correctly
    const char *md = "`multi\nline` *emph*";
    cmark_node *doc = cmark_parse_document_for_delimiters(md, strlen(md), CMARK_OPT_DEFAULT);
    cmark_node *para = cmark_node_first_child(doc);

    // Find the code node (first child)
    cmark_node *code = cmark_node_first_child(para);
    OK(runner, cmark_node_get_type(code) == CMARK_NODE_CODE, "multiline_code_emph: first is code");

    int delim_char = 0, delim_len = 0;
    int ok = cmark_node_get_delimiter_info(code, md, strlen(md), CMARK_OPT_SOURCEPOS, &delim_char, &delim_len);
    OK(runner, ok == 1, "multiline_code_emph: code get_delimiter_info returns 1");
    INT_EQ(runner, delim_char, '`', "multiline_code_emph: code delim_char is `");
    INT_EQ(runner, delim_len, 1, "multiline_code_emph: code delim_len is 1");

    // Find the emph node (after text node with space)
    cmark_node *node = cmark_node_next(code);
    while (node && cmark_node_get_type(node) != CMARK_NODE_EMPH) {
        node = cmark_node_next(node);
    }

    OK(runner, node != NULL, "multiline_code_emph: found emph node");
    if (node) {
        ok = cmark_node_get_delimiter_info(node, md, strlen(md), CMARK_OPT_SOURCEPOS, &delim_char, &delim_len);
        OK(runner, ok == 1, "multiline_code_emph: emph get_delimiter_info returns 1");
        INT_EQ(runner, delim_char, '*', "multiline_code_emph: emph delim_char is *");
        INT_EQ(runner, delim_len, 1, "multiline_code_emph: emph delim_len is 1");
    }

    cmark_node_free(doc);
}

static void test_cr_line_endings(test_batch_runner *runner) {
    // Test CR-only line endings (old Mac style)
    const char *md = "line1\r*emph*";
    cmark_node *doc = cmark_parse_document_for_delimiters(md, strlen(md), CMARK_OPT_DEFAULT);
    cmark_node *para = cmark_node_first_child(doc);

    // Find emph node
    cmark_node *node = cmark_node_first_child(para);
    while (node && cmark_node_get_type(node) != CMARK_NODE_EMPH) {
        node = cmark_node_next(node);
    }

    OK(runner, node != NULL, "cr_endings: found emph node");
    if (node) {
        int delim_char = 0, delim_len = 0;
        int ok = cmark_node_get_delimiter_info(node, md, strlen(md), CMARK_OPT_SOURCEPOS, &delim_char, &delim_len);
        OK(runner, ok == 1, "cr_endings: get_delimiter_info returns 1");
        INT_EQ(runner, delim_char, '*', "cr_endings: delim_char is *");
        INT_EQ(runner, delim_len, 1, "cr_endings: delim_len is 1");
    }

    cmark_node_free(doc);
}

static void test_crlf_line_endings(test_batch_runner *runner) {
    // Test CRLF line endings (Windows style)
    const char *md = "line1\r\n*emph*";
    cmark_node *doc = cmark_parse_document_for_delimiters(md, strlen(md), CMARK_OPT_DEFAULT);
    cmark_node *para = cmark_node_first_child(doc);

    // Find emph node
    cmark_node *node = cmark_node_first_child(para);
    while (node && cmark_node_get_type(node) != CMARK_NODE_EMPH) {
        node = cmark_node_next(node);
    }

    OK(runner, node != NULL, "crlf_endings: found emph node");
    if (node) {
        int delim_char = 0, delim_len = 0;
        int ok = cmark_node_get_delimiter_info(node, md, strlen(md), CMARK_OPT_SOURCEPOS, &delim_char, &delim_len);
        OK(runner, ok == 1, "crlf_endings: get_delimiter_info returns 1");
        INT_EQ(runner, delim_char, '*', "crlf_endings: delim_char is *");
        INT_EQ(runner, delim_len, 1, "crlf_endings: delim_len is 1");
    }

    cmark_node_free(doc);
}

void test_delimiters(test_batch_runner *runner) {
    test_emph_asterisk(runner);
    test_emph_underscore(runner);
    test_strong_asterisk(runner);
    test_strong_underscore(runner);
    test_code_single_backtick(runner);
    test_code_double_backtick(runner);
    test_delim_length_convenience(runner);
    test_delim_char_convenience(runner);
    test_null_handling(runner);
    test_nested_emphasis(runner);
    test_multiline(runner);
    test_multiline_code_then_emph(runner);
    test_cr_line_endings(runner);
    test_crlf_line_endings(runner);
}
