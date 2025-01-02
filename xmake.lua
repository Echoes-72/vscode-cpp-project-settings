set_config("buildir", ".vscode/build")
target("App")
    set_kind("binary")
    set_plat("linux")
    set_arch("x64")
    set_languages("c++17")
    set_toolchains("llvm")
    set_toolset("cxx", "clang++")

    set_encodings("source:utf-8", "target:utf-8")
    add_includedirs("include")
    add_files("src/*.cpp","main.cpp", {defines={"UNICODE","_UNICODE"}})


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
    end
