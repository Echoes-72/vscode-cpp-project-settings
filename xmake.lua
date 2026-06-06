set_config("buildir", ".vscode/build")
add_requires("opencv4","avcodec","avformat","avutil","swscale")

target("App")
    set_kind("binary")

    set_encodings("source:utf-8", "target:utf-8")
    add_cxxflags()
    
    add_packages("opencv4","avformat","avcodec","avutil","swscale")
    add_links()
    add_ldflags()

    add_includedirs("include")
    add_files("main.cpp","src/*.cpp", {defines={"UNICODE","_UNICODE"}})

    if is_mode("debug") then --明确指定debug模式
        -- 添加DEBUG编译宏
        add_defines("DEBUG")
        -- 设置目标文件存放目录
        set_targetdir(".vscode/bin/debug")
        -- 启用调试符号
        set_symbols("debug")
        -- 禁用优化
        set_optimize("none")
    end

    if is_mode("release") then
        -- 设置目标文件存放目录
        set_targetdir(".vscode/bin/release")
        set_optimize("fastest")
    end
    
