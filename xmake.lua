set_config("buildir", ".vscode/build")
add_requires("opencv4","avcodec","avformat","avutil","swscale")

rule("Glsl")
    set_extensions(".frag", ".vert", ".comp")
    on_build_file(function (target, sourcefile, opt)
        import("core.project.depend")

        -- 确保构建目录存在
        os.mkdir(path.join(target:targetdir() , path.directory(sourcefile)))
        -- print("targetdir:", target:targetdir() .. path.directory(sourcefile))
        
        local targetfile = path.join(target:targetdir(), sourcefile .. ".spv")
        
        -- 只在文件改变时重新构建
        depend.on_changed(function ()
            -- 调用 pandoc 将 markdown 转换为 html
            os.vrunv('glslc', {sourcefile, "-o", targetfile})
        end, {files = sourcefile})
    end)


target("App")
    set_kind("binary")

    set_encodings("source:utf-8", "target:utf-8")
    add_cxxflags()
    
    add_packages("opencv4","avformat","avcodec","avutil","swscale")
    add_links()
    add_ldflags()

    add_includedirs("include")
    add_files("main.cpp","src/*.cpp", {defines={"UNICODE","_UNICODE"}})

    add_rules("Glsl")
    add_files("res/shaders/*.frag","res/shaders/*.vert")

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
    
