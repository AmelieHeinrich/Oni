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
    add_includedirs("src", "ext", "ext/PIX/include", "ext/optick/", "ext/nvtt")
    add_deps("D3D12MA", "ImGui", "stb", "optick", "ImGuizmo", "cgltf", "meshopt")
    add_defines("GLM_FORCE_DEPTH_ZERO_TO_ONE", "USE_PIX")

    add_linkdirs("ext/PIX/lib")
    add_linkdirs("ext/nvtt/lib64")

    if is_plat("windows") then
        add_syslinks("user32", "kernel32", "gdi32", "dxgi", "d3d12", "dxcompiler", "WinPixEventRuntime.lib", "nvtt30205.lib")
    end

    if is_mode("debug") then
        set_symbols("debug")
        set_optimize("none")
        add_defines("ONI_DEBUG")
    end

    if is_mode("release") then
        set_symbols("hidden")
        set_optimize("fastest")
        set_strip("all")
    end
