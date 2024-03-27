-- 
-- @Author: Amélie Heinrich
-- @Create Time: 2024-03-27 13:39:53
-- 

add_rules("mode.debug", "mode.release")

target("Oni")
    set_languages("c99")
    add_files("src/*.c",
              "src/core/*.c")
    add_includedirs("src")
    set_rundir(".")

    if is_plat("windows") then
        add_files("src/core/windows/**.c")
        add_syslinks("user32", "kernel32", "gdi32")
    end
