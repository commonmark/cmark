#include <stdio.h>
#include <string.h>

#include "core-extensions.h"
#include "table.h"
#include "strikethrough.h"

int core_extensions_registration(cmark_plugin *plugin) {
  cmark_plugin_register_syntax_extension(plugin, create_table_extension());
  cmark_plugin_register_syntax_extension(plugin, create_strikethrough_extension());
  return 1;
}
