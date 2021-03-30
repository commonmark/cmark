#include "cmark-gfm-core-extensions.h"
#include "autolink.h"
#include "mutex.h"
#include "strikethrough.h"
#include "table.h"
#include "tagfilter.h"
#include "tasklist.h"
#include "registry.h"
#include "plugin.h"

CMARK_DEFINE_LOCK(extensions);

static int core_extensions_registration(cmark_plugin *plugin) {
  CMARK_INITIALIZE_AND_LOCK(extensions);
  cmark_plugin_register_syntax_extension(plugin, create_table_extension());
  cmark_plugin_register_syntax_extension(plugin,
                                         create_strikethrough_extension());
  cmark_plugin_register_syntax_extension(plugin, create_autolink_extension());
  cmark_plugin_register_syntax_extension(plugin, create_tagfilter_extension());
  cmark_plugin_register_syntax_extension(plugin, create_tasklist_extension());
  CMARK_UNLOCK(extensions);
  return 1;
}

pthread_mutex_t registered_lock;
static atomic_int registered_latch = 0;
static _Atomic int registered = 0;

void cmark_gfm_core_extensions_ensure_registered(void) {
  CMARK_INITIALIZE_AND_LOCK(extensions);
  if (!registered) {
    cmark_register_plugin(core_extensions_registration);
    registered = 1;
  }
  CMARK_UNLOCK(extensions);
}
