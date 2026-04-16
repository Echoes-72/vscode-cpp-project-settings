set_config("buildir", ".vscode/build")

add_requires("avcodec","avformat","avutil","swscale","opencv4")

-- 定义交叉工具链
toolchain("aarch64")
    set_kind("standalone")
    set_sdkdir("/run/media/echo/C/Code_Enviroment/CrossToolchains/gcc-arm-11.2-2022.02-x86_64-aarch64-none-linux-gnu/")
toolchain_end()

target("App")
    set_kind("binary")

    set_encodings("source:utf-8", "target:utf-8")

    add_includedirs("include")
    add_files("main.cpp","src/*.cpp")

    if not is_mode("cross") then
        add_packages("avcodec","avformat","avutil","swscale","opencv4")
    end

    if is_mode("debug") then --明确指定debug模式
        -- 添加DEBUG编译宏
        add_defines("DEBUG")
        -- 设置目标文件存放目录
        -- set_targetdir(".vscode/bin/debug")

        set_installdir("$(scriptdir)/.vscode/bin/$(arch)/${mode}")
        -- 启用调试符号
        set_symbols("debug")
        -- 禁用优化
        set_optimize("none")
    end

    if is_mode("release") then
        -- 设置目标文件存放目录
        -- set_targetdir(".vscode/bin/release")  

        set_installdir("$(scriptdir)/.vscode/bin/$(arch)/${mode}")      
        set_optimize("fastest")
    end

    if is_mode("cross") then
        set_toolchains("aarch64")

        local target_plat="linux"
        local target_arch="aarch64"
        local depsdir = "$(scriptdir)/res/deps/"
        -- 依赖名称
        local deps={"opencv","ffmpeg","rknn"}

        set_plat(target_plat)
        set_arch(target_arch)
        -- 方便查看信息,改为.so后缀
        set_filename("Orangepi.so")

        -- 添加依赖库的包含目录和链接目录
        for _, dep in ipairs(deps) do
             add_includedirs(depsdir..dep.."/include/")
             add_linkdirs(depsdir..dep.."/lib/")
        end

        -- 编译连接配置
        add_rpathdirs("$ORIGIN/lib",{runpath = true})
        set_optimize("fastest")
        add_links("opencv_core","opencv_videoio","opencv_imgproc","opencv_imgcodecs")
        add_links("avcodec","avdevice","avformat","avfilter","avutil","swresample","swscale")

        -- 设置安装目录结构
        set_installdir("$(scriptdir)/.vscode/bin/$(mode)/")
        set_prefixdir("",{bindir=""})

        -- 添加安装运行依赖文件
        for _, dep in ipairs(deps) do
            add_installfiles(depsdir..dep.."/lib/*", {prefixdir = "lib"})
        end

        -- 添加资源文件
        add_installfiles("$(scriptdir)/res/model/*", {prefixdir="model"})
        add_installfiles("$(scriptdir)/res/testvideos/*",{prefixdir="testvideos"})

    end
