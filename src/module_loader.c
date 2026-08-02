#include <stdlib.h>
#include <sys/param.h>
#include <uv.h>

#include "rtsyn/internal/module_loader.h"

#if !defined(MAXPATHLEN)
#define MAXPATHLEN 4096
#endif

rtsyn_module_loader_t *
rtsyn_module_loader_create(const char *path)
{
    char resolved_path[MAXPATHLEN] = {0};
    if (!rtsyn_module_loader_resolve_path(path, resolved_path, sizeof(resolved_path)))
    {
        return nullptr;
    }

    rtsyn_module_loader_t *loader = malloc(sizeof(rtsyn_module_loader_t));
    if (!loader)
    {
        return nullptr;
    }

    if (uv_dlopen(resolved_path, &loader->library) != 0)
    {
        free(loader);
        return nullptr;
    }

    rtsyn_module_loader_entry_point_t entry_point = nullptr;
    if (uv_dlsym(&loader->library, RTSYN_MODULE_LOADER_ENTRY_POINT, (void **)&entry_point) != 0)
    {
        uv_dlclose(&loader->library);
        free(loader);
        return nullptr;
    }

    loader->descriptor = entry_point();
    if (!rtsyn_module_loader_descriptor_is_valid(loader->descriptor))
    {
        uv_dlclose(&loader->library);
        free(loader);
        return nullptr;
    }

    return loader;
}

const rtsyn_abi_node_descriptor_t *
rtsyn_module_loader_get_descriptor(rtsyn_module_loader_t *loader)
{
    return loader ? loader->descriptor : nullptr;
}

void
rtsyn_module_loader_destroy(rtsyn_module_loader_t *loader)
{
    if (!loader)
    {
        return;
    }

    uv_dlclose(&loader->library);
    free(loader);
}
