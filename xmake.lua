-- 
-- @Author: Amélie Heinrich
-- @Create Time: 2024-03-27 13:39:53
-- 

includes("ext")

add_rules("mode.debug", "mode.release")

target("Oni")
    set_rundir(".")
    set_languages("c++17")
    add_files("src/**.cpp")
    add_includedirs("src", "ext")
    add_deps("D3D12MA", "ImGui")

    if is_plat("windows") then
        add_syslinks("user32", "kernel32", "gdi32", "dxgi", "d3d12", "dxcompiler")
    end
