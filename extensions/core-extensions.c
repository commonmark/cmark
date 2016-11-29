#include "core-extensions.h"
#include "strikethrough.h"
#include "table.h"

int core_extensions_registration(cmark_plugin *plugin) {
  cmark_plugin_register_syntax_extension(plugin, create_table_extension());
  cmark_plugin_register_syntax_extension(plugin,
                                         create_strikethrough_extension());
  return 1;
}
