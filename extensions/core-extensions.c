#include <pthread.h>
#include <stdatomic.h>

#include "cmark-gfm-core-extensions.h"
#include "autolink.h"
#include "mutex.h"
#include "strikethrough.h"
#include "table.h"
#include "tagfilter.h"
#include "tasklist.h"
#include "registry.h"
#include "plugin.h"

static pthread_mutex_t extensions_lock;
static atomic_int extensions_latch = 0;

static int core_extensions_registration(cmark_plugin *plugin) {
  initialize_mutex_once(&extensions_lock, &extensions_latch);
  pthread_mutex_lock(&extensions_lock);
  cmark_plugin_register_syntax_extension(plugin, create_table_extension());
  cmark_plugin_register_syntax_extension(plugin,
                                         create_strikethrough_extension());
  cmark_plugin_register_syntax_extension(plugin, create_autolink_extension());
  cmark_plugin_register_syntax_extension(plugin, create_tagfilter_extension());
  cmark_plugin_register_syntax_extension(plugin, create_tasklist_extension());
  pthread_mutex_unlock(&extensions_lock);
  return 1;
}

pthread_mutex_t registered_lock;
static atomic_int registered_latch = 0;
static _Atomic int registered = 0;

void cmark_gfm_core_extensions_ensure_registered(void) {
  initialize_mutex_once(&registered_lock, &registered_latch);
  pthread_mutex_lock(&registered_lock);
  if (!registered) {
    cmark_register_plugin(core_extensions_registration);
    registered = 1;
  }
  pthread_mutex_unlock(&registered_lock);
}
