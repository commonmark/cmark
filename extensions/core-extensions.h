#ifndef CORE_EXTENSIONS_H
#define CORE_EXTENSIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <cmark_extension_api.h>

int core_extensions_registration(cmark_plugin *plugin);

#ifdef __cplusplus
}
#endif

#endif
