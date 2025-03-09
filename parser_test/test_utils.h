#ifndef CMARK_TEST_UTILS_H
#define CMARK_TEST_UTILS_H

/**
 * Compare markdown input with expected XML output
 *
 * @param markdown The input markdown text
 * @param expected_xml The expected XML output
 * @param options Parsing and rendering options
 * @return 1 if the actual output matches expected output, 0 otherwise
 */
int test_xml(const char *markdown, const char *expected_xml, int options);

#define CASE(func) do { if ((func)() <= 0) return -1; } while (0)

#endif /* CMARK_TEST_UTILS_H */
