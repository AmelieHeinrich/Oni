-- 
-- @Author: Amélie Heinrich
-- @Create Time: 2024-03-27 13:39:53
-- 

target("D3D12MA")
    add_includedirs(".")
    set_kind("static")
    add_files("D3D12MA/*.cpp")
    add_headerfiles("D3D12MA/*.h")
