/**
 * @file rtsyn/internal/module_loader.h
 * @author Sergio Hidalgo (sergiohg.dev@gmail.com)
 * @brief Internal definitions for the RTSyn module loader.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 * @copyright Copyright (c) Sergio Hidalgo 2026
 */
#ifndef RTSYN_INTERNAL_MODULE_LOADER_H
#define RTSYN_INTERNAL_MODULE_LOADER_H
#include <uv.h>

#include "rtsyn/module_loader.h"

#if defined(_WIN32)
#define _S_IFMT  S_IFMT
#define _S_IFREG S_IFREG
#endif

struct rtsyn_module_loader_s {
    uv_lib_t library;
    const rtsyn_abi_node_descriptor_t *descriptor;
};

typedef const rtsyn_abi_node_descriptor_t *(RTSYN_ABI_CALL *rtsyn_module_loader_entry_point_t)(
    void);

bool
rtsyn_module_loader_path_is_valid(const char *path);

bool
rtsyn_module_loader_resolve_path(const char *path, char *out_path, size_t out_path_size);

bool
rtsyn_module_loader_descriptor_is_valid(const rtsyn_abi_node_descriptor_t *descriptor);

#endif /* RTSYN_INTERNAL_MODULE_LOADER_H */
