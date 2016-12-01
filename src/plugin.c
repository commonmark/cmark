#include <stdlib.h>

#include "plugin.h"

int cmark_plugin_register_syntax_extension(cmark_plugin    * plugin,
                                        cmark_syntax_extension * extension) {
  plugin->syntax_extensions = cmark_llist_append(plugin->syntax_extensions, extension);
  return 1;
}

cmark_plugin *
cmark_plugin_new(void) {
  cmark_plugin *res = malloc(sizeof(cmark_plugin));

  res->syntax_extensions = NULL;

  return res;
}

void
cmark_plugin_free(cmark_plugin *plugin) {
  cmark_llist_free_full(plugin->syntax_extensions,
                        (cmark_free_func) cmark_syntax_extension_free);
  free(plugin);
}

cmark_llist *
cmark_plugin_steal_syntax_extensions(cmark_plugin *plugin) {
  cmark_llist *res = plugin->syntax_extensions;

  plugin->syntax_extensions = NULL;
  return res;
}
