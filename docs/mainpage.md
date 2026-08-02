# RTSyn Module Loader

RTSyn Module Loader opens a dynamic module and retrieves its RTSyn ABI node descriptor.

## Public API

The public API is declared in:

- `include/rtsyn/module_loader.h`: module loading, function lookup, and unloading.

## Basic Usage

```c
#include <rtsyn/module_loader.h>

int main(void) {
  rtsyn_module_loader_t *loader = rtsyn_module_loader_create("./module.so");

  if (loader == NULL) {
    return 1;
  }

  const rtsyn_abi_node_descriptor_t *descriptor =
      rtsyn_module_loader_get_descriptor(loader);
  int result = descriptor == NULL;
  rtsyn_module_loader_destroy(loader);
  return result;
}
```

## Generating Documentation

Run:

```sh
xmake doxygen
```

Then open `build/html/index.html`.
