/**
 * @file rtsyn/module_loader.h
 * @author Sergio Hidalgo (sergiohg.dev@gmail.com)
 * @brief Public API for loading functions exposed by RTSyn modules.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 * @copyright Copyright (c) Sergio Hidalgo 2026
 */
#ifndef RTSYN_MODULE_LOADER_H
#define RTSYN_MODULE_LOADER_H

#include <rtsyn/abi/node.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RTSYN_MODULE_LOADER_ENTRY_POINT "rtsyn_module_get_descriptor"

typedef struct rtsyn_module_loader_s rtsyn_module_loader_t;

/**
 * Open the module at @p path.
 *
 * @p path may point to a built shared library, a module root directory, or the
 * module root xmake.lua. Module roots are built with xmake before opening the
 * generated shared library.
 *
 * @return A loader that owns the open module, or NULL when the path does not
 *         identify a valid RTSyn module.
 */
rtsyn_module_loader_t *
rtsyn_module_loader_create(const char *path);

/**
 * Get the RTSyn ABI node descriptor exposed by the open module.
 *
 * The descriptor and all memory and functions referenced by it remain valid
 * until @p loader is destroyed.
 *
 * @return A valid ABI node descriptor, or NULL when the entry point is missing
 *         or returns an invalid descriptor.
 */
const rtsyn_abi_node_descriptor_t *
rtsyn_module_loader_get_descriptor(rtsyn_module_loader_t *loader);

/** Close the module and release its loader. */
void
rtsyn_module_loader_destroy(rtsyn_module_loader_t *loader);

#ifdef __cplusplus
}
#endif

#endif /* RTSYN_MODULE_LOADER_H */
