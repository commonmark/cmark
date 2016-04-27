#include <dirent.h>
#include <dlfcn.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "cmark.h"
#include "syntax_extension.h"
#include "registry.h"
#include "plugin.h"


static cmark_llist *syntax_extensions = NULL;
static cmark_llist *plugin_handles = NULL;

static cmark_plugin *scan_file(char* filename) {
  char* last_slash = strrchr(filename, '/');
  char* name_start = last_slash ? last_slash + 1 : filename;
  char* last_dot = strrchr(filename, '.');
  cmark_plugin *plugin = NULL;
  char *init_func_name = NULL;
  int i;
  void *libhandle;
  char *libname = NULL;

  if (!last_dot || strcmp(last_dot, ".so"))
      goto done;

  libname = malloc(sizeof(char) * (strlen(EXTENSION_DIR) + strlen(filename) + 2));
  snprintf(libname, strlen(EXTENSION_DIR) + strlen(filename) + 2, "%s/%s",
      EXTENSION_DIR, filename);
  libhandle = dlopen(libname, RTLD_NOW);
  free(libname);

  if (!libhandle) {
      printf("Error loading DSO: %s\n", dlerror());
      goto done;
  }

  name_start[last_dot - name_start] = '\0';

  for (i = 0; name_start[i]; i++) {
    if (name_start[i] == '-')
      name_start[i] = '_';
  }

  init_func_name = malloc(sizeof(char) * (strlen(name_start) + 6));

  snprintf(init_func_name, strlen(name_start) + 6, "init_%s", name_start);

  cmark_plugin_init_func initfunc = (cmark_plugin_init_func)
      (intptr_t) dlsym(libhandle, init_func_name);
  free(init_func_name);

  plugin = cmark_plugin_new();

  if (initfunc) {
    if (initfunc(plugin)) {
      plugin_handles = cmark_llist_append(plugin_handles, libhandle);
    } else {
      cmark_plugin_free(plugin);
      printf("Error Initializing plugin %s\n", name_start);
      plugin = NULL;
      dlclose(libhandle);
    }
  } else {
    printf("Error loading init function: %s\n", dlerror());
    dlclose(libhandle);
  }

done:
  return plugin;
}

static void scan_path(char *path) {
  DIR *dir = opendir(path);
  struct dirent* direntry;

  if (!dir)
    return;

  while ((direntry = readdir(dir))) {
    cmark_plugin *plugin = scan_file(direntry->d_name);
    if (plugin) {
      cmark_llist *syntax_extensions_list = cmark_plugin_steal_syntax_extensions(plugin);
      cmark_llist *tmp;

      for (tmp = syntax_extensions_list; tmp; tmp=tmp->next) {
        syntax_extensions = cmark_llist_append(syntax_extensions, tmp->data);
      }

      cmark_llist_free(syntax_extensions_list);
      cmark_plugin_free(plugin);
    }
  }

 closedir(dir);
}

void cmark_discover_plugins(void) {
  cmark_release_plugins();
  scan_path(EXTENSION_DIR);
}

static void
release_plugin_handle(void *libhandle) {
  dlclose(libhandle);
}

void cmark_release_plugins(void) {
  if (syntax_extensions) {
    cmark_llist_free_full(syntax_extensions,
        (cmark_free_func) cmark_syntax_extension_free);
    syntax_extensions = NULL;
  }

  cmark_llist_free_full(plugin_handles, release_plugin_handle);
  plugin_handles = NULL;
}

cmark_llist *cmark_list_syntax_extensions(void) {
  cmark_llist *tmp;
  cmark_llist *res = NULL;

  for (tmp = syntax_extensions; tmp; tmp = tmp->next) {
    res = cmark_llist_append(res, tmp->data);
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
