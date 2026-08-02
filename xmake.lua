local project_name = "rtsyn-module-loader"
local project_xmake_repo = "rtsyn-xmake-repo"

set_license("GPL-3.0-or-later")

add_rules("mode.debug", "mode.release")
add_rules("plugin.compile_commands.autoupdate")
set_defaultmode("release")
if is_mode("release") then
	set_optimize("fastest")
	set_strip("all")
	set_symbols("hidden")
end

option("valgrind", { default = false, description = "Run tests with Valgrind" })
option("tests", { default = true, description = "Build tests" })

set_languages("c23")
add_requires("libuv")
if has_config("tests") then
	add_requires("gtest")
	add_requires("rtsyn-test-utils")
	add_requires("rtsyn-mock", { private = true })
end

local rtsyn_dependencies = { "rtsyn-abi" }
for i, rtsyn_dependency in ipairs(rtsyn_dependencies) do
	add_requires(rtsyn_dependency)
end

local workspace = os.getenv("RTSYN_WORKSPACE")
if workspace then
	local repository_dir = path.join(workspace, project_xmake_repo)
	add_repositories(project_xmake_repo .. " " .. repository_dir)
else
	add_repositories(project_xmake_repo .. " https://github.com/seregioo/" .. project_xmake_repo .. ".git")
end

target(project_name)
set_kind("static")
if not is_plat("windows") then
	add_defines("_POSIX_C_SOURCE=200809L")
end
add_files("src/*.c")
add_files("src/**/*.c")
add_includedirs("include", { public = true })
for i, rtsyn_dependency in ipairs(rtsyn_dependencies) do
	add_packages(rtsyn_dependency)
end
add_packages("libuv")
add_includedirs("src")
add_headerfiles("include/(rtsyn/**.h)")

local rtsyn_modules = {
	{ path = "module_loader", name = "module-loader" },
}

if has_config("tests") then
	for _, rtsyn_module in ipairs(rtsyn_modules) do
		local tests_name = "tests/" .. rtsyn_module.path .. "-tests"
		target(tests_name)
		set_kind("binary")
		if has_config("valgrind") then
			add_rules("@rtsyn-test-utils/valgrind")
		end
		add_deps(project_name)
		add_packages("gtest")
		add_packages("libuv")
		for i, rtsyn_dependency in ipairs(rtsyn_dependencies) do
			add_packages(rtsyn_dependency)
		end
		add_links("gtest_main")
		add_includedirs("src")
		add_files("tests/" .. rtsyn_module.path .. ".cpp")
		add_rules("@rtsyn-test-utils/loadable_package", {
			package = "rtsyn-mock",
			define = "RTSYN_TEST_MODULE_PATH",
		})
		add_tests(rtsyn_module.name)
	end
end
--
-- If you want to known more usage about xmake, please see https://xmake.io
--
