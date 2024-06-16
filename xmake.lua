if is_plat("windows") then
  set_toolchains("clang-cl")
else 
  set_toolchains("clang")
end

set_warnings("allextra")

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
