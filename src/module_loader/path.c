#include <dirent.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <uv.h>

#include "rtsyn/internal/module_loader.h"

#if !defined(PATH_MAX)
#define PATH_MAX 4096
#endif

static bool
rtsyn_module_loader_is_regular_file(const char *path)
{
    struct stat statbuf = {0};
    return path && stat(path, &statbuf) == 0 && S_ISREG(statbuf.st_mode);
}

static bool
rtsyn_module_loader_is_directory(const char *path)
{
    struct stat statbuf = {0};
    return path && stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode);
}

static const char *
rtsyn_module_loader_basename(const char *path)
{
    const char *slash = path ? strrchr(path, '/') : nullptr;
    return slash ? slash + 1 : path;
}

static bool
rtsyn_module_loader_has_shared_library_extension(const char *path)
{
    const char *name = rtsyn_module_loader_basename(path);
    const char *dot = name ? strrchr(name, '.') : nullptr;
    return dot
           && (strcmp(dot, ".so") == 0 || strcmp(dot, ".dylib") == 0 || strcmp(dot, ".dll") == 0);
}

static bool
rtsyn_module_loader_copy_path(const char *path, char *out_path, size_t out_path_size)
{
    if (!path || !out_path || out_path_size == 0)
    {
        return false;
    }

    int written = snprintf(out_path, out_path_size, "%s", path);
    return written >= 0 && (size_t)written < out_path_size;
}

static bool
rtsyn_module_loader_join_path(char *out_path, size_t out_path_size, const char *left,
                              const char *right)
{
    if (!left || !right || !out_path || out_path_size == 0)
    {
        return false;
    }

    int written = snprintf(out_path, out_path_size, "%s/%s", left, right);
    return written >= 0 && (size_t)written < out_path_size;
}

static bool
rtsyn_module_loader_parent_dir(const char *path, char *out_path, size_t out_path_size)
{
    char buffer[PATH_MAX] = {0};
    if (!rtsyn_module_loader_copy_path(path, buffer, sizeof(buffer)))
    {
        return false;
    }

    char *slash = strrchr(buffer, '/');
    if (!slash)
    {
        return rtsyn_module_loader_copy_path(".", out_path, out_path_size);
    }
    if (slash == buffer)
    {
        slash[1] = '\0';
    } else
    {
        *slash = '\0';
    }
    return rtsyn_module_loader_copy_path(buffer, out_path, out_path_size);
}

static bool
rtsyn_module_loader_realpath(const char *path, char *out_path, size_t out_path_size)
{
    if (!path || !out_path || out_path_size == 0)
    {
        return false;
    }

    uv_fs_t request = {0};
    int result = uv_fs_realpath(nullptr, &request, path, nullptr);
    if (result < 0 || !request.ptr)
    {
        uv_fs_req_cleanup(&request);
        return false;
    }

    bool copied = rtsyn_module_loader_copy_path((const char *)request.ptr, out_path, out_path_size);
    uv_fs_req_cleanup(&request);
    return copied;
}

static bool
rtsyn_module_loader_module_root(const char *path, char *out_path, size_t out_path_size)
{
    char resolved[PATH_MAX] = {0};
    if (!path || path[0] == '\0' || !rtsyn_module_loader_realpath(path, resolved, sizeof(resolved)))
    {
        return false;
    }

    if (rtsyn_module_loader_is_directory(resolved))
    {
        char xmake_path[PATH_MAX] = {0};
        if (!rtsyn_module_loader_join_path(xmake_path, sizeof(xmake_path), resolved, "xmake.lua")
            || !rtsyn_module_loader_is_regular_file(xmake_path))
        {
            return false;
        }
        return rtsyn_module_loader_copy_path(resolved, out_path, out_path_size);
    }

    if (rtsyn_module_loader_is_regular_file(resolved)
        && strcmp(rtsyn_module_loader_basename(resolved), "xmake.lua") == 0)
    {
        return rtsyn_module_loader_parent_dir(resolved, out_path, out_path_size);
    }

    return false;
}

static bool
rtsyn_module_loader_path_contains_package_cache(const char *path)
{
    return path && strstr(path, "/.packages/") != nullptr;
}

static const char *
rtsyn_module_loader_xmake_binary(void)
{
    const char *xmake = getenv("RTSYN_XMAKE_BIN");
    return xmake && xmake[0] != '\0' ? xmake : "xmake";
}

static bool
rtsyn_module_loader_run_xmake(const char *module_root, char *const argv[])
{
    pid_t pid = fork();
    if (pid < 0)
    {
        return false;
    }
    if (pid == 0)
    {
        if (chdir(module_root) != 0)
        {
            _exit(127);
        }

        execvp(argv[0], argv);
        _exit(127);
    }

    int status = 0;
    if (waitpid(pid, &status, 0) < 0)
    {
        return false;
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

static bool
rtsyn_module_loader_build_module(const char *module_root)
{
    char *const configure_args[] = {
        (char *)rtsyn_module_loader_xmake_binary(),
        (char *)"f",
        (char *)"-y",
        (char *)"--mode=release",
        (char *)"--tests=false",
        nullptr,
    };
    char *const build_args[] = {
        (char *)rtsyn_module_loader_xmake_binary(),
        (char *)"build",
        (char *)"-y",
        nullptr,
    };
    return rtsyn_module_loader_run_xmake(module_root, configure_args)
           && rtsyn_module_loader_run_xmake(module_root, build_args);
}

static bool
rtsyn_module_loader_shared_library_is_preferred(const char *path, const char *preferred_stem)
{
    const char *name = rtsyn_module_loader_basename(path);
    if (!name || !preferred_stem || preferred_stem[0] == '\0')
    {
        return false;
    }

    char candidate[PATH_MAX] = {0};
    if (snprintf(candidate, sizeof(candidate), "lib%s.so", preferred_stem) >= 0
        && strcmp(name, candidate) == 0)
    {
        return true;
    }
    if (snprintf(candidate, sizeof(candidate), "lib%s.dylib", preferred_stem) >= 0
        && strcmp(name, candidate) == 0)
    {
        return true;
    }
    if (snprintf(candidate, sizeof(candidate), "%s.dll", preferred_stem) >= 0
        && strcmp(name, candidate) == 0)
    {
        return true;
    }
    return strcmp(name, preferred_stem) == 0;
}

static bool
rtsyn_module_loader_shared_library_is_better(const char *candidate, const char *selected,
                                             const char *preferred_stem)
{
    if (!selected || selected[0] == '\0')
    {
        return true;
    }

    const bool candidate_preferred =
        rtsyn_module_loader_shared_library_is_preferred(candidate, preferred_stem);
    const bool selected_preferred =
        rtsyn_module_loader_shared_library_is_preferred(selected, preferred_stem);
    if (candidate_preferred != selected_preferred)
    {
        return candidate_preferred;
    }

    return strcmp(candidate, selected) < 0;
}

static bool
rtsyn_module_loader_find_shared_library(const char *root, uint32_t depth,
                                        const char *preferred_stem, char *out_path,
                                        size_t out_path_size)
{
    if (depth == 0 || !root)
    {
        return false;
    }

    DIR *dir = opendir(root);
    if (!dir)
    {
        return false;
    }

    bool found = out_path && out_path[0] != '\0';
    struct dirent *entry = nullptr;
    while ((entry = readdir(dir)) != nullptr)
    {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
        {
            continue;
        }

        char child[PATH_MAX] = {0};
        if (!rtsyn_module_loader_join_path(child, sizeof(child), root, entry->d_name))
        {
            continue;
        }

        if (rtsyn_module_loader_is_regular_file(child)
            && rtsyn_module_loader_has_shared_library_extension(child))
        {
            if (rtsyn_module_loader_shared_library_is_better(child, out_path, preferred_stem))
            {
                found = rtsyn_module_loader_copy_path(child, out_path, out_path_size);
            }
        } else if (rtsyn_module_loader_is_directory(child))
        {
            if (rtsyn_module_loader_path_contains_package_cache(child))
            {
                continue;
            }
            bool child_found =
                rtsyn_module_loader_find_shared_library(child, depth - 1, preferred_stem, out_path,
                                                        out_path_size);
            found = found || child_found;
        }
    }

    closedir(dir);
    return found;
}

bool
rtsyn_module_loader_path_is_valid(const char *path)
{
    char resolved[PATH_MAX] = {0};
    if (!path || path[0] == '\0')
    {
        return false;
    }

    if (rtsyn_module_loader_realpath(path, resolved, sizeof(resolved))
        && rtsyn_module_loader_is_regular_file(resolved)
        && rtsyn_module_loader_has_shared_library_extension(resolved))
    {
        return true;
    }

    return rtsyn_module_loader_module_root(path, resolved, sizeof(resolved));
}

bool
rtsyn_module_loader_resolve_path(const char *path, char *out_path, size_t out_path_size)
{
    if (!path || path[0] == '\0' || !out_path || out_path_size == 0)
    {
        return false;
    }

    char resolved[PATH_MAX] = {0};
    if (rtsyn_module_loader_realpath(path, resolved, sizeof(resolved))
        && rtsyn_module_loader_is_regular_file(resolved)
        && rtsyn_module_loader_has_shared_library_extension(resolved))
    {
        return rtsyn_module_loader_copy_path(resolved, out_path, out_path_size);
    }

    char module_root[PATH_MAX] = {0};
    if (!rtsyn_module_loader_module_root(path, module_root, sizeof(module_root)))
    {
        return false;
    }

    if (!rtsyn_module_loader_build_module(module_root))
    {
        return false;
    }

    char build_dir[PATH_MAX] = {0};
    if (!rtsyn_module_loader_join_path(build_dir, sizeof(build_dir), module_root, "build"))
    {
        return false;
    }

    const char *preferred_stem = rtsyn_module_loader_basename(module_root);

    out_path[0] = '\0';
    return rtsyn_module_loader_find_shared_library(build_dir, 8, preferred_stem, out_path,
                                                   out_path_size);
}
