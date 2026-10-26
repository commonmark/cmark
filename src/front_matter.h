#ifndef CMARK_FRONT_MATTER_H
#define CMARK_FRONT_MATTER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmark.h"
#include "parser.h"
#include "chunk.h"

// Called from S_process_line in blocks.c for every line when
// CMARK_OPT_FRONT_MATTER is set.  Drives the front matter state machine
// stored directly on the parser (front_matter_scanning / front_matter_buf).
//
// Returns true if the line was consumed by the front matter scanner and
// should not be passed to the normal block parser.
bool cmark_front_matter_process_line(cmark_parser *parser, cmark_chunk *input);

#ifdef __cplusplus
}
#endif

#endif
