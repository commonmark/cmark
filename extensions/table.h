#ifndef TABLE_H
#define TABLE_H

#include "core-extensions.h"


extern cmark_node_type CMARK_NODE_TABLE, CMARK_NODE_TABLE_ROW,
    CMARK_NODE_TABLE_CELL;

cmark_syntax_extension *create_table_extension(void);

#endif
