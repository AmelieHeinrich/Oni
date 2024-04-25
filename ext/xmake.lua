-- 
-- @Author: Amélie Heinrich
-- @Create Time: 2024-03-27 13:39:53
-- 

target("D3D12MA")
    add_includedirs(".")
    set_kind("static")
    add_files("D3D12MA/*.cpp")
    add_headerfiles("D3D12MA/*.h")

target("ImGui")
    add_includedirs(".")
    set_kind("static")
    add_files("ImGui/**.cpp")
    add_headerfiles("ImGui/**.h")
    add_syslinks("d3d12", "dxgi")

target("stb")
    add_includedirs(".")
    set_kind("static")
    add_files("stb/*.cpp")
    add_headerfiles("stb/*.h")

target("optick")
    add_includedirs(".")
    set_kind("static")
    add_files("optick/*.cpp")
    add_headerfiles("optick/*.h")
