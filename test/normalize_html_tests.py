import sys
from normalize import normalize_html

def out(str):
    sys.stdout.buffer.write(str.encode('utf-8')) 

def do_test(actual_html, expected_html, result_counts):
    unicode_error = None
    try:
        output = normalize_html(actual_html)
        passed = output == expected_html
    except UnicodeDecodeError as e:
        unicode_error = e
        passed = False

    if passed:
        result_counts['pass'] += 1
    else:
        if unicode_error:
            out("Unicode error: " + str(unicode_error) + '\n')
            out("Expected: " + repr(expected_html) + '\n')
            out("Got:      " + repr(actual_html) + '\n')
        else:
            out("Failed example: " + repr(actual_html) + '\n')
            out("Expected: " + repr(expected_html) + '\n')
            out("Got:      " + repr(output) + '\n')
        result_counts['fail'] += 1


def run_tests():
    tests = [
        {"input": "<p>a  \t b</p>", "output": "<p>a b</p>"},
        {"input": "<p>a  \t\nb</p>", "output": "<p>a b</p>"},
        {"input": "<p>a  b</p>", "output": "<p>a b</p>"},
        {"input": " <p>a  b</p>", "output": "<p>a b</p>"},
        {"input": "<p>a  b</p> ", "output": "<p>a b</p>"},
        {"input": "\n\t<p>\n\t\ta b\t\t</p>\n\t", "output": "<p>a b</p>"},
        {"input": "<i>a  b</i> ", "output": "<i>a b</i> "},
        {"input": "<br />", "output": "<br>"},
        {"input": "<a title=\"bar\" HREF=\"foo\">x</a>", "output": "<a href=\"foo\" title=\"bar\">x</a>"},
        {"input": "&forall;&amp;&gt;&lt;&quot;", "output": "\u2200&amp;&gt;&lt;&quot;"}
    ]

    print("Testing normalize_html:")
    result_counts = {'pass': 0, 'fail': 0}
    for test in tests:
        do_test(test['input'], test['output'], result_counts)

    out("{pass} passed, {fail} failed\n".format(**result_counts))
    exit(result_counts['fail'])


if __name__ == "__main__":
    run_tests()