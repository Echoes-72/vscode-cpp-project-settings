set_config("buildir", ".vscode/build")

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
    set_plat("linux")
    set_arch("x64")
    set_languages("c++17")
    set_toolchains("llvm")
    set_toolset("cxx", "clang++")
    set_encodings("source:utf-8", "target:utf-8")

    add_links("glfw", "vulkan", "dl", "pthread", "X11", "Xxf86vm", "Xrandr", "Xi")

    add_includedirs("include")
    add_files("main.cpp", {defines={"UNICODE","_UNICODE"}})
    
    add_rules("Glsl")
    add_files("resource/shaders/*.frag","resource/shaders/*.vert")


    if is_mode("debug") then --明确指定debug模式
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
    
