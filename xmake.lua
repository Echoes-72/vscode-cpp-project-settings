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

    if is_mode("debug") then --明确指定debug模式
        -- 添加DEBUG编译宏
        add_defines("DEBUG")
        -- 设置目标文件存放目录
        set_targetdir(".vscode/bin/debug")
        add_packages("avcodec","avformat","avutil","swscale","opencv4")
        -- 启用调试符号
        set_symbols("debug")
        -- 禁用优化
        set_optimize("none")
    end

    if is_mode("release") then
        -- 设置目标文件存放目录
        set_targetdir(".vscode/bin/release")        
        add_packages("avcodec","avformat","avutil","swscale","opencv4")
        set_optimize("fastest")
    end

    if is_mode("cross") then
        -- 方便查看信息,改为.so后缀
        set_filename("App.so")

        set_optimize("fastest")

        -- 设置目标文件存放目录
        set_targetdir(".vscode/bin/cross")
        set_toolchains("aarch64")

        add_includedirs("./res/opencv/include/opencv4")
        add_linkdirs("./res/opencv/lib")
        add_links("opencv_core","opencv_videoio","opencv_imgproc","opencv_imgcodecs")

        -- 此为静态链接库
        -- add_links("opencv_world")
        -- add_linkdirs("./res/opencv/lib/opencv4/3rdparty")
        
        add_includedirs("./res/ffmpeg/include")
        add_linkdirs("./res/ffmpeg/lib")
        add_links("avcodec","avdevice","avformat","avfilter","avutil","swresample","swscale")
        add_rpathdirs("$ORIGIN/lib",{runpath = true})
    end