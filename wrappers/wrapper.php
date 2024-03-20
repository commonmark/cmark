<?php

$ffi = FFI::cdef(
    'char *cmark_markdown_to_html(const char *text, size_t len, int options);',
    'libcmark.so'
);
$markdown = <<<'md'
# First level title

## Second level title

Paragraph

md;

$html = FFI::string($ffi->cmark_markdown_to_html($markdown, strlen($markdown), 0));

echo $html . PHP_EOL;
