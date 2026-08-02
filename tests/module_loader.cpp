#include <cstdlib>
#include <gtest/gtest.h>
#include <string>

#include "rtsyn/module_loader.h"

#ifndef RTSYN_TEST_MODULE_PATH
#error RTSYN_TEST_MODULE_PATH must name the module loader test fixture
#endif

TEST(ModuleLoader, RejectsInvalidPaths)
{
    EXPECT_EQ(rtsyn_module_loader_create(nullptr), nullptr);
    EXPECT_EQ(rtsyn_module_loader_create(""), nullptr);
    EXPECT_EQ(rtsyn_module_loader_create("."), nullptr);
    EXPECT_EQ(rtsyn_module_loader_create("this/module/does/not/exist"), nullptr);
}

TEST(ModuleLoader, LoadsAnAbiNodeDescriptor)
{
    rtsyn_module_loader_t *loader = rtsyn_module_loader_create(RTSYN_TEST_MODULE_PATH);
    ASSERT_NE(loader, nullptr);

    const rtsyn_abi_node_descriptor_t *descriptor = rtsyn_module_loader_get_descriptor(loader);
    ASSERT_NE(descriptor, nullptr);
    EXPECT_STREQ(descriptor->name, "test-plugin");
    EXPECT_EQ(descriptor->port_count, 0U);
    void *instance = nullptr;
    EXPECT_EQ(descriptor->callbacks.create(&instance), RTSYN_ABI_STATUS_OK);

    rtsyn_module_loader_destroy(loader);
}

TEST(ModuleLoader, LoadsAnAbiNodeDescriptorFromXmakeEntrypoint)
{
    const char *workspace = std::getenv("RTSYN_WORKSPACE");
    ASSERT_NE(workspace, nullptr);

    const std::string xmake_path = std::string(workspace) + "/rtsyn-mock/xmake.lua";
    rtsyn_module_loader_t *loader = rtsyn_module_loader_create(xmake_path.c_str());
    ASSERT_NE(loader, nullptr);

    const rtsyn_abi_node_descriptor_t *descriptor = rtsyn_module_loader_get_descriptor(loader);
    ASSERT_NE(descriptor, nullptr);
    EXPECT_STREQ(descriptor->name, "test-plugin");

    rtsyn_module_loader_destroy(loader);
}

TEST(ModuleLoader, HandlesNullLoaders)
{
    EXPECT_EQ(rtsyn_module_loader_get_descriptor(nullptr), nullptr);
    rtsyn_module_loader_destroy(nullptr);
}
