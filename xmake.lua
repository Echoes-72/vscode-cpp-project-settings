set_config("buildir", ".vscode/build")
set_config("projectdir", ".vscode/")
target("App")
    if is_mode("debug") then --!当代码里明确指定了debug模式时,默认的就不奏效了 
        -- 添加DEBUG编译宏
        add_defines("DEBUG")
        -- 设置目标文件存放目录
        set_targetdir("bin/debug")
        -- 启用调试符号
        set_symbols("debug")
        -- 禁用优化
        set_optimize("none")
    end

    if is_mode("release") then
        -- 设置目标文件存放目录
        set_targetdir("bin/release")
        set_optimize("fastest")
        print('Compiling in release mode Successfully !!!!!!!')
        after_build(function()
            os.exec("bin\\release\\App.exe")
        end)
    end
    set_kind("binary")
    set_plat("windows")
    set_arch("x64")
    set_toolchains("llvm")
    set_languages("c++17")
    add_includedirs("include")
    add_defines("UNICODE", "_UNICODE")
    add_files("src/*.cpp","main.cpp","resource/res.rc")