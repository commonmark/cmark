#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CMARK_NO_SHORT_NAMES
#include "cmark.h"
#include "node.h"

#include "harness.h"
#include "cplusplus.h"

#define UTF8_REPL "\xEF\xBF\xBD"

static const cmark_node_type node_types[] = {
    CMARK_NODE_DOCUMENT,  CMARK_NODE_BLOCK_QUOTE, CMARK_NODE_LIST,
    CMARK_NODE_ITEM,      CMARK_NODE_CODE_BLOCK,  CMARK_NODE_HTML_BLOCK,
    CMARK_NODE_PARAGRAPH, CMARK_NODE_HEADING,     CMARK_NODE_THEMATIC_BREAK,
    CMARK_NODE_TEXT,      CMARK_NODE_SOFTBREAK,   CMARK_NODE_LINEBREAK,
    CMARK_NODE_CODE,      CMARK_NODE_HTML_INLINE, CMARK_NODE_EMPH,
    CMARK_NODE_STRONG,    CMARK_NODE_LINK,        CMARK_NODE_IMAGE};
static const int num_node_types = sizeof(node_types) / sizeof(*node_types);

static void test_content(test_batch_runner *runner, cmark_node_type type,
                         int allowed_content);

static void test_continuation_byte(test_batch_runner *runner, const char *utf8);

static void version(test_batch_runner *runner) {
  INT_EQ(runner, cmark_version(), CMARK_VERSION, "cmark_version");
  STR_EQ(runner, cmark_version_string(), CMARK_VERSION_STRING,
         "cmark_version_string");
}

static void accessors(test_batch_runner *runner) {
  static const char markdown[] = "## Header\n"
                                 "\n"
                                 "* Item 1\n"
                                 "* Item 2\n"
                                 "\n"
                                 "2. Item 1\n"
                                 "\n"
                                 "3. Item 2\n"
                                 "\n"
                                 "``` lang\n"
                                 "fenced\n"
                                 "```\n"
                                 "    code\n"
                                 "\n"
                                 "<div>html</div>\n"
                                 "\n"
                                 "[link](url 'title')\n";

  cmark_node *doc =
      cmark_parse_document(markdown, sizeof(markdown) - 1, CMARK_OPT_DEFAULT);

  // Getters

  cmark_node *heading = cmark_node_first_child(doc);
  INT_EQ(runner, cmark_node_get_heading_level(heading), 2, "get_heading_level");

  cmark_node *bullet_list = cmark_node_next(heading);
  INT_EQ(runner, cmark_node_get_list_type(bullet_list), CMARK_BULLET_LIST,
         "get_list_type bullet");
  INT_EQ(runner, cmark_node_get_list_tight(bullet_list), 1,
         "get_list_tight tight");

  cmark_node *ordered_list = cmark_node_next(bullet_list);
  INT_EQ(runner, cmark_node_get_list_type(ordered_list), CMARK_ORDERED_LIST,
         "get_list_type ordered");
  INT_EQ(runner, cmark_node_get_list_delim(ordered_list), CMARK_PERIOD_DELIM,
         "get_list_delim ordered");
  INT_EQ(runner, cmark_node_get_list_start(ordered_list), 2, "get_list_start");
  INT_EQ(runner, cmark_node_get_list_tight(ordered_list), 0,
         "get_list_tight loose");
  cmark_node *fenced = cmark_node_next(ordered_list);
  STR_EQ(runner, cmark_node_get_literal(fenced), "fenced\n",
         "get_literal fenced code");
  STR_EQ(runner, cmark_node_get_fence_info(fenced), "lang", "get_fence_info");

  cmark_node *code = cmark_node_next(fenced);
  STR_EQ(runner, cmark_node_get_literal(code), "code\n",
         "get_literal indented code");

  cmark_node *html = cmark_node_next(code);
  STR_EQ(runner, cmark_node_get_literal(html), "<div>html</div>\n",
         "get_literal html");

  cmark_node *paragraph = cmark_node_next(html);
  INT_EQ(runner, cmark_node_get_start_line(paragraph), 17, "get_start_line");
  INT_EQ(runner, cmark_node_get_start_column(paragraph), 1, "get_start_column");
  INT_EQ(runner, cmark_node_get_end_line(paragraph), 17, "get_end_line");

  cmark_node *link = cmark_node_first_child(paragraph);
  STR_EQ(runner, cmark_node_get_url(link), "url", "get_url");
  STR_EQ(runner, cmark_node_get_title(link), "title", "get_title");

  cmark_node *string = cmark_node_first_child(link);
  STR_EQ(runner, cmark_node_get_literal(string), "link", "get_literal string");

  // Setters

  OK(runner, cmark_node_set_heading_level(heading, 3), "set_heading_level");

  OK(runner, cmark_node_set_list_type(bullet_list, CMARK_ORDERED_LIST),
     "set_list_type ordered");
  OK(runner, cmark_node_set_list_delim(bullet_list, CMARK_PAREN_DELIM),
     "set_list_delim paren");
  OK(runner, cmark_node_set_list_start(bullet_list, 3), "set_list_start");
  OK(runner, cmark_node_set_list_tight(bullet_list, 0), "set_list_tight loose");

  OK(runner, cmark_node_set_list_type(ordered_list, CMARK_BULLET_LIST),
     "set_list_type bullet");
  OK(runner, cmark_node_set_list_tight(ordered_list, 1),
     "set_list_tight tight");

  OK(runner, cmark_node_set_literal(code, "CODE\n"),
     "set_literal indented code");

  OK(runner, cmark_node_set_literal(fenced, "FENCED\n"),
     "set_literal fenced code");
  OK(runner, cmark_node_set_fence_info(fenced, "LANG"), "set_fence_info");

  OK(runner, cmark_node_set_literal(html, "<div>HTML</div>\n"),
     "set_literal html");

  OK(runner, cmark_node_set_url(link, "URL"), "set_url");
  OK(runner, cmark_node_set_title(link, "TITLE"), "set_title");

  OK(runner, cmark_node_set_literal(string, "prefix-LINK"),
     "set_literal string");

  // Set literal to suffix of itself (issue #139).
  const char *literal = cmark_node_get_literal(string);
  OK(runner, cmark_node_set_literal(string, literal + sizeof("prefix")),
     "set_literal suffix");

  // Getter errors

  INT_EQ(runner, cmark_node_get_heading_level(bullet_list), 0,
         "get_heading_level error");
  INT_EQ(runner, cmark_node_get_list_type(heading), CMARK_NO_LIST,
         "get_list_type error");
  INT_EQ(runner, cmark_node_get_list_start(code), 0, "get_list_start error");
  INT_EQ(runner, cmark_node_get_list_tight(fenced), 0, "get_list_tight error");
  OK(runner, cmark_node_get_literal(ordered_list) == NULL, "get_literal error");
  OK(runner, cmark_node_get_fence_info(paragraph) == NULL,
     "get_fence_info error");
  OK(runner, cmark_node_get_url(html) == NULL, "get_url error");
  OK(runner, cmark_node_get_title(heading) == NULL, "get_title error");

  // Setter errors

  OK(runner, !cmark_node_set_heading_level(bullet_list, 3),
     "set_heading_level error");
  OK(runner, !cmark_node_set_list_type(heading, CMARK_ORDERED_LIST),
     "set_list_type error");
  OK(runner, !cmark_node_set_list_start(code, 3), "set_list_start error");
  OK(runner, !cmark_node_set_list_tight(fenced, 0), "set_list_tight error");
  OK(runner, !cmark_node_set_literal(ordered_list, "content\n"),
     "set_literal error");
  OK(runner, !cmark_node_set_fence_info(paragraph, "lang"),
     "set_fence_info error");
  OK(runner, !cmark_node_set_url(html, "url"), "set_url error");
  OK(runner, !cmark_node_set_title(heading, "title"), "set_title error");

  OK(runner, !cmark_node_set_heading_level(heading, 0),
     "set_heading_level too small");
  OK(runner, !cmark_node_set_heading_level(heading, 7),
     "set_heading_level too large");
  OK(runner, !cmark_node_set_list_type(bullet_list, CMARK_NO_LIST),
     "set_list_type invalid");
  OK(runner, !cmark_node_set_list_start(bullet_list, -1),
     "set_list_start negative");

  cmark_node_free(doc);
}

static void free_parent(test_batch_runner *runner) {
  static const char markdown[] = "text\n";

  cmark_node *doc =
      cmark_parse_document(markdown, sizeof(markdown) - 1, CMARK_OPT_DEFAULT);

  cmark_node *para = cmark_node_first_child(doc);
  cmark_node *text = cmark_node_first_child(para);
  cmark_node_unlink(text);
  cmark_node_free(doc);
  STR_EQ(runner, cmark_node_get_literal(text), "text",
         "inline content after freeing parent block");
  cmark_node_free(text);
}

static void node_check(test_batch_runner *runner) {
  // Construct an incomplete tree.
  cmark_node *doc = cmark_node_new(CMARK_NODE_DOCUMENT);
  cmark_node *p1 = cmark_node_new(CMARK_NODE_PARAGRAPH);
  cmark_node *p2 = cmark_node_new(CMARK_NODE_PARAGRAPH);
  doc->first_child = p1;
  p1->next = p2;

  INT_EQ(runner, cmark_node_check(doc, NULL), 4, "node_check works");
  INT_EQ(runner, cmark_node_check(doc, NULL), 0, "node_check fixes tree");

  cmark_node_free(doc);
}

static void iterator(test_batch_runner *runner) {
  cmark_node *doc = cmark_parse_document("> a *b*\n\nc", 10, CMARK_OPT_DEFAULT);
  int parnodes = 0;
  cmark_event_type ev_type;
  cmark_iter *iter = cmark_iter_new(doc);
  cmark_node *cur;

  while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
    cur = cmark_iter_get_node(iter);
    if (cur->type == CMARK_NODE_PARAGRAPH && ev_type == CMARK_EVENT_ENTER) {
      parnodes += 1;
    }
  }
  INT_EQ(runner, parnodes, 2, "iterate correctly counts paragraphs");

  cmark_iter_free(iter);
  cmark_node_free(doc);
}

static void iterator_delete(test_batch_runner *runner) {
  static const char md[] = "a *b* c\n"
                           "\n"
                           "* item1\n"
                           "* item2\n"
                           "\n"
                           "a `b` c\n"
                           "\n"
                           "* item1\n"
                           "* item2\n";
  cmark_node *doc = cmark_parse_document(md, sizeof(md) - 1, CMARK_OPT_DEFAULT);
  cmark_iter *iter = cmark_iter_new(doc);
  cmark_event_type ev_type;

  while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
    cmark_node *node = cmark_iter_get_node(iter);
    // Delete list, emph, and code nodes.
    if ((ev_type == CMARK_EVENT_EXIT && node->type == CMARK_NODE_LIST) ||
        (ev_type == CMARK_EVENT_EXIT && node->type == CMARK_NODE_EMPH) ||
        (ev_type == CMARK_EVENT_ENTER && node->type == CMARK_NODE_CODE)) {
      cmark_node_free(node);
    }
  }

  cmark_iter_free(iter);
  cmark_node_free(doc);
}

static void create_tree(test_batch_runner *runner) {
  cmark_node *doc = cmark_node_new(CMARK_NODE_DOCUMENT);

  cmark_node *p = cmark_node_new(CMARK_NODE_PARAGRAPH);
  OK(runner, !cmark_node_insert_before(doc, p), "insert before root fails");
  OK(runner, !cmark_node_insert_after(doc, p), "insert after root fails");
  OK(runner, cmark_node_append_child(doc, p), "append1");
  INT_EQ(runner, cmark_node_check(doc, NULL), 0, "append1 consistent");
  OK(runner, cmark_node_parent(p) == doc, "node_parent");

  cmark_node *emph = cmark_node_new(CMARK_NODE_EMPH);
  OK(runner, cmark_node_prepend_child(p, emph), "prepend1");
  INT_EQ(runner, cmark_node_check(doc, NULL), 0, "prepend1 consistent");

  cmark_node *str1 = cmark_node_new(CMARK_NODE_TEXT);
  cmark_node_set_literal(str1, "Hello, ");
  OK(runner, cmark_node_prepend_child(p, str1), "prepend2");
  INT_EQ(runner, cmark_node_check(doc, NULL), 0, "prepend2 consistent");

  cmark_node *str3 = cmark_node_new(CMARK_NODE_TEXT);
  cmark_node_set_literal(str3, "!");
  OK(runner, cmark_node_append_child(p, str3), "append2");
  INT_EQ(runner, cmark_node_check(doc, NULL), 0, "append2 consistent");

  cmark_node *str2 = cmark_node_new(CMARK_NODE_TEXT);
  cmark_node_set_literal(str2, "world");
  OK(runner, cmark_node_append_child(emph, str2), "append3");
  INT_EQ(runner, cmark_node_check(doc, NULL), 0, "append3 consistent");

  OK(runner, cmark_node_insert_before(str1, str3), "ins before1");
  INT_EQ(runner, cmark_node_check(doc, NULL), 0, "ins before1 consistent");
  // 31e
  OK(runner, cmark_node_first_child(p) == str3, "ins before1 works");

  OK(runner, cmark_node_insert_before(str1, emph), "ins before2");
  INT_EQ(runner, cmark_node_check(doc, NULL), 0, "ins before2 consistent");
  // 3e1
  OK(runner, cmark_node_last_child(p) == str1, "ins before2 works");

  OK(runner, cmark_node_insert_after(str1, str3), "ins after1");
  INT_EQ(runner, cmark_node_check(doc, NULL), 0, "ins after1 consistent");
  // e13
  OK(runner, cmark_node_next(str1) == str3, "ins after1 works");

  OK(runner, cmark_node_insert_after(str1, emph), "ins after2");
  INT_EQ(runner, cmark_node_check(doc, NULL), 0, "ins after2 consistent");
  // 1e3
  OK(runner, cmark_node_previous(emph) == str1, "ins after2 works");

  cmark_node *str4 = cmark_node_new(CMARK_NODE_TEXT);
  cmark_node_set_literal(str4, "brzz");
  OK(runner, cmark_node_replace(str1, str4), "replace");
  // The replaced node is not freed
  cmark_node_free(str1);

  INT_EQ(runner, cmark_node_check(doc, NULL), 0, "replace consistent");
  OK(runner, cmark_node_previous(emph) == str4, "replace works");
  INT_EQ(runner, cmark_node_replace(p, str4), 0, "replace str for p fails");

  cmark_node_unlink(emph);

  cmark_node_free(doc);
  cmark_node_free(emph);
}

static void custom_nodes(test_batch_runner *runner) {
  cmark_node *doc = cmark_node_new(CMARK_NODE_DOCUMENT);
  cmark_node *p = cmark_node_new(CMARK_NODE_PARAGRAPH);
  cmark_node_append_child(doc, p);
  cmark_node *ci = cmark_node_new(CMARK_NODE_CUSTOM_INLINE);
  cmark_node *str1 = cmark_node_new(CMARK_NODE_TEXT);
  cmark_node_set_literal(str1, "Hello");
  OK(runner, cmark_node_append_child(ci, str1), "append1");
  OK(runner, cmark_node_set_on_enter(ci, "<ON ENTER|"), "set_on_enter");
  OK(runner, cmark_node_set_on_exit(ci, "|ON EXIT>"), "set_on_exit");
  STR_EQ(runner, cmark_node_get_on_enter(ci), "<ON ENTER|", "get_on_enter");
  STR_EQ(runner, cmark_node_get_on_exit(ci), "|ON EXIT>", "get_on_exit");
  cmark_node_append_child(p, ci);
  cmark_node *cb = cmark_node_new(CMARK_NODE_CUSTOM_BLOCK);
  cmark_node_set_on_enter(cb, "<on enter|");
  // leave on_exit unset
  STR_EQ(runner, cmark_node_get_on_exit(cb), "", "get_on_exit (empty)");
  cmark_node_append_child(doc, cb);

  cmark_node_free(doc);
}

void hierarchy(test_batch_runner *runner) {
  cmark_node *bquote1 = cmark_node_new(CMARK_NODE_BLOCK_QUOTE);
  cmark_node *bquote2 = cmark_node_new(CMARK_NODE_BLOCK_QUOTE);
  cmark_node *bquote3 = cmark_node_new(CMARK_NODE_BLOCK_QUOTE);

  OK(runner, cmark_node_append_child(bquote1, bquote2), "append bquote2");
  OK(runner, cmark_node_append_child(bquote2, bquote3), "append bquote3");
  OK(runner, !cmark_node_append_child(bquote3, bquote3),
     "adding a node as child of itself fails");
  OK(runner, !cmark_node_append_child(bquote3, bquote1),
     "adding a parent as child fails");

  cmark_node_free(bquote1);

  int max_node_type = CMARK_NODE_LAST_BLOCK > CMARK_NODE_LAST_INLINE
                          ? CMARK_NODE_LAST_BLOCK
                          : CMARK_NODE_LAST_INLINE;
  OK(runner, max_node_type < 32, "all node types < 32");

  int list_item_flag = 1 << CMARK_NODE_ITEM;
  int top_level_blocks =
      (1 << CMARK_NODE_BLOCK_QUOTE) | (1 << CMARK_NODE_LIST) |
      (1 << CMARK_NODE_CODE_BLOCK) | (1 << CMARK_NODE_HTML_BLOCK) |
      (1 << CMARK_NODE_PARAGRAPH) | (1 << CMARK_NODE_HEADING) |
      (1 << CMARK_NODE_THEMATIC_BREAK);
  int all_inlines = (1 << CMARK_NODE_TEXT) | (1 << CMARK_NODE_SOFTBREAK) |
                    (1 << CMARK_NODE_LINEBREAK) | (1 << CMARK_NODE_CODE) |
                    (1 << CMARK_NODE_HTML_INLINE) | (1 << CMARK_NODE_EMPH) |
                    (1 << CMARK_NODE_STRONG) | (1 << CMARK_NODE_LINK) |
                    (1 << CMARK_NODE_IMAGE);

  test_content(runner, CMARK_NODE_DOCUMENT, top_level_blocks);
  test_content(runner, CMARK_NODE_BLOCK_QUOTE, top_level_blocks);
  test_content(runner, CMARK_NODE_LIST, list_item_flag);
  test_content(runner, CMARK_NODE_ITEM, top_level_blocks);
  test_content(runner, CMARK_NODE_CODE_BLOCK, 0);
  test_content(runner, CMARK_NODE_HTML_BLOCK, 0);
  test_content(runner, CMARK_NODE_PARAGRAPH, all_inlines);
  test_content(runner, CMARK_NODE_HEADING, all_inlines);
  test_content(runner, CMARK_NODE_THEMATIC_BREAK, 0);
  test_content(runner, CMARK_NODE_TEXT, 0);
  test_content(runner, CMARK_NODE_SOFTBREAK, 0);
  test_content(runner, CMARK_NODE_LINEBREAK, 0);
  test_content(runner, CMARK_NODE_CODE, 0);
  test_content(runner, CMARK_NODE_HTML_INLINE, 0);
  test_content(runner, CMARK_NODE_EMPH, all_inlines);
  test_content(runner, CMARK_NODE_STRONG, all_inlines);
  test_content(runner, CMARK_NODE_LINK, all_inlines);
  test_content(runner, CMARK_NODE_IMAGE, all_inlines);
}

static void test_content(test_batch_runner *runner, cmark_node_type type,
                         int allowed_content) {
  cmark_node *node = cmark_node_new(type);

  for (int i = 0; i < num_node_types; ++i) {
    cmark_node_type child_type = node_types[i];
    cmark_node *child = cmark_node_new(child_type);

    int got = cmark_node_append_child(node, child);
    int expected = (allowed_content >> child_type) & 1;

    INT_EQ(runner, got, expected, "add %d as child of %d", child_type, type);

    cmark_node_free(child);
  }

  cmark_node_free(node);
}

static void render_commonmark(test_batch_runner *runner) {
  char *commonmark;

  static const char markdown[] = "> \\- foo *bar* \\*bar\\*\n"
                                 "\n"
                                 "- Lorem ipsum dolor sit amet,\n"
                                 "  consectetur adipiscing elit,\n"
                                 "- sed do eiusmod tempor incididunt\n"
                                 "  ut labore et dolore magna aliqua.\n";
  cmark_node *doc =
      cmark_parse_document(markdown, sizeof(markdown) - 1, CMARK_OPT_DEFAULT);

  commonmark = cmark_render_commonmark(doc, CMARK_OPT_DEFAULT, 26);
  STR_EQ(runner, commonmark, "> \\- foo *bar* \\*bar\\*\n"
                             "\n"
                             "  - Lorem ipsum dolor sit\n"
                             "    amet, consectetur\n"
                             "    adipiscing elit,\n"
                             "  - sed do eiusmod tempor\n"
                             "    incididunt ut labore\n"
                             "    et dolore magna\n"
                             "    aliqua.\n",
         "render document with wrapping");
  free(commonmark);
  commonmark = cmark_render_commonmark(doc, CMARK_OPT_DEFAULT, 0);
  STR_EQ(runner, commonmark, "> \\- foo *bar* \\*bar\\*\n"
                             "\n"
                             "  - Lorem ipsum dolor sit amet,\n"
                             "    consectetur adipiscing elit,\n"
                             "  - sed do eiusmod tempor incididunt\n"
                             "    ut labore et dolore magna aliqua.\n",
         "render document without wrapping");
  free(commonmark);

  cmark_node *text = cmark_node_new(CMARK_NODE_TEXT);
  cmark_node_set_literal(text, "Hi");
  commonmark = cmark_render_commonmark(text, CMARK_OPT_DEFAULT, 0);
  STR_EQ(runner, commonmark, "Hi\n", "render single inline node");
  free(commonmark);

  cmark_node_free(text);
  cmark_node_free(doc);
}

static void utf8(test_batch_runner *runner) {
  // Invalid continuation bytes
  test_continuation_byte(runner, "\xC2\x80");
  test_continuation_byte(runner, "\xE0\xA0\x80");
  test_continuation_byte(runner, "\xF0\x90\x80\x80");
}

static void test_continuation_byte(test_batch_runner *runner,
                                   const char *utf8) {
  size_t len = strlen(utf8);

  for (size_t pos = 1; pos < len; ++pos) {
    char buf[20];
    sprintf(buf, "((((%s))))", utf8);
    buf[4 + pos] = '\x20';

    char expected[50];
    strcpy(expected, "<p>((((" UTF8_REPL "\x20");
    for (size_t i = pos + 1; i < len; ++i) {
      strcat(expected, UTF8_REPL);
    }
    strcat(expected, "))))</p>\n");
  }
}

static void test_feed_across_line_ending(test_batch_runner *runner) {
  // See #117
  cmark_parser *parser = cmark_parser_new(CMARK_OPT_DEFAULT);
  cmark_parser_feed(parser, "line1\r", 6);
  cmark_parser_feed(parser, "\nline2\r\n", 8);
  cmark_node *document = cmark_parser_finish(parser);
  OK(runner, document->first_child->next == NULL, "document has one paragraph");
  cmark_parser_free(parser);
  cmark_node_free(document);
}

static void sub_document(test_batch_runner *runner) {
  cmark_node *doc = cmark_node_new(CMARK_NODE_DOCUMENT);
  cmark_node *list = cmark_node_new(CMARK_NODE_LIST);
  OK(runner, cmark_node_append_child(doc, list), "list");

  {
    cmark_node *item = cmark_node_new(CMARK_NODE_ITEM);
    OK(runner, cmark_node_append_child(list, item), "append_0");
    static const char markdown[] =
      "Hello &ldquo; <http://www.google.com>\n";
    cmark_parser *parser = cmark_parser_new_with_mem_into_root(
        CMARK_OPT_DEFAULT,
        cmark_get_default_mem_allocator(),
        item);
    cmark_parser_feed(parser, markdown, sizeof(markdown) - 1);
    OK(runner, cmark_parser_finish(parser) != NULL, "parser_finish_0");
    cmark_parser_free(parser);
  }

  {
    cmark_node *item = cmark_node_new(CMARK_NODE_ITEM);
    OK(runner, cmark_node_append_child(list, item), "append_0");
    static const char markdown[] =
      "Bye &ldquo; <http://www.geocities.com>\n";
    cmark_parser *parser = cmark_parser_new_with_mem_into_root(
        CMARK_OPT_DEFAULT,
        cmark_get_default_mem_allocator(),
        item);
    cmark_parser_feed(parser, markdown, sizeof(markdown) - 1);
    OK(runner, cmark_parser_finish(parser) != NULL, "parser_finish_0");
    cmark_parser_free(parser);
  }

  char *cmark = cmark_render_commonmark(doc, CMARK_OPT_DEFAULT, 0);
  STR_EQ(runner, cmark, "  - Hello “ <http://www.google.com>\n"
                        "\n"
                        "  - Bye “ <http://www.geocities.com>\n",
         "nested document CommonMark is as expected");
  free(cmark);

  cmark_node_free(doc);
}

int main(void) {
  int retval;
  test_batch_runner *runner = test_batch_runner_new();

  version(runner);
  accessors(runner);
  free_parent(runner);
  node_check(runner);
  iterator(runner);
  iterator_delete(runner);
  create_tree(runner);
  custom_nodes(runner);
  hierarchy(runner);
  render_commonmark(runner);
  utf8(runner);
  test_cplusplus(runner);
  test_feed_across_line_ending(runner);
  sub_document(runner);

  test_print_summary(runner);
  retval = test_ok(runner) ? 0 : 1;
  free(runner);

  return retval;
}
