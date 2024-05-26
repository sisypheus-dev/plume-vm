if is_plat("windows") then
    set_toolchains("clang-cl")
else
    set_toolchains("clang")
end

add_requires("libcurl")

set_warnings("allextra")

-- add_rpathdirs("/Volumes/Programmation/Plume/plume/standard/runtime/lib/")

target("plume-vm")
    add_rules("mode.release")
    add_files("src/**.c")
    add_includedirs("include")
    set_kind("binary")
    set_targetdir("bin")
    set_optimize("fastest")

target("plume-vm-test")
    add_rules("mode.debug", "mode.profile")
    add_files("src/**.c")
    add_includedirs("include")
    set_targetdir("bin")
    set_kind("binary")
    set_symbols("debug")
    add_cxflags("-pg")
    add_ldflags("-pg")
    set_optimize("fastest")

target("plume-natives")
    add_files("standard/**.c")
    add_files("src/core/*.c")
    add_files("src/value.c")
    add_includedirs("include")
    set_kind("shared")
    add_packages("libcurl")
    
    set_targetdir(".")
    set_basename("native")
    set_prefixname("")
    set_extension(".plmc")
