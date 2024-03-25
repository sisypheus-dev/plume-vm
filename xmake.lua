add_rules("mode.debug", "mode.release", "mode.profile")

add_files("src/**.c")
add_includedirs("include")

if is_mode("profile") then
  set_symbols("debug")
  add_cxflags("-pg")
  add_ldflags("-pg")
end

target("plume-vm")
  set_kind("binary") 
  set_targetdir("bin")
  set_optimize("fastest")
  -- set_symbols("debug")
  -- add_cxflags("-pg")
  -- add_ldflags("-pg")
