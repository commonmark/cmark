#ifndef CORE_EXTENSIONS_H
#define CORE_EXTENSIONS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <cmark_extension_api.h>
#include "cmarkextensions_export.h"

CMARKEXTENSIONS_EXPORT
int core_extensions_registration(cmark_plugin *plugin);

#ifdef __cplusplus
}
#endif

#endif
