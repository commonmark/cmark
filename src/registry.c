#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "cmark.h"
#include "syntax_extension.h"
#include "registry.h"
#include "plugin.h"


static cmark_llist *syntax_extensions = NULL;

void cmark_register_plugin(cmark_plugin_init_func reg_fn) {
  cmark_plugin *plugin = cmark_plugin_new();

  if (!reg_fn(plugin)) {
    cmark_plugin_free(plugin);
    return;
  }

  cmark_llist *syntax_extensions_list = cmark_plugin_steal_syntax_extensions(plugin),
              *it;

  for (it = syntax_extensions_list; it; it = it->next) {
    syntax_extensions = cmark_llist_append(syntax_extensions, it->data);
  }

  cmark_llist_free(syntax_extensions_list);
  cmark_plugin_free(plugin);
}

void cmark_release_plugins(void) {
  if (syntax_extensions) {
    cmark_llist_free_full(syntax_extensions,
        (cmark_free_func) cmark_syntax_extension_free);
    syntax_extensions = NULL;
  }
}

cmark_llist *cmark_list_syntax_extensions(void) {
  cmark_llist *it;
  cmark_llist *res = NULL;

  for (it = syntax_extensions; it; it = it->next) {
    res = cmark_llist_append(res, it->data);
  }
  return res;
}

cmark_syntax_extension *cmark_find_syntax_extension(const char *name) {
  cmark_llist *tmp;

  for (tmp = syntax_extensions; tmp; tmp = tmp->next) {
    cmark_syntax_extension *ext = (cmark_syntax_extension *) tmp->data;
    if (!strcmp(ext->name, name))
      return ext;
  }
  return NULL;
}
