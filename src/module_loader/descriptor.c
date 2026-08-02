#include "rtsyn/internal/module_loader.h"

bool
rtsyn_module_loader_descriptor_is_valid(const rtsyn_abi_node_descriptor_t *descriptor)
{
    if (!descriptor || !descriptor->name || descriptor->name[0] == '\0'
        || (descriptor->port_count > 0 && !descriptor->ports) || !descriptor->callbacks.create
        || !descriptor->callbacks.start || !descriptor->callbacks.process
        || !descriptor->callbacks.stop || !descriptor->callbacks.destroy)
    {
        return false;
    }

    for (uint32_t index = 0; index < descriptor->port_count; index++)
    {
        if (!rtsyn_abi_port_descriptor_is_valid(&descriptor->ports[index]))
        {
            return false;
        }
    }

    return true;
}
