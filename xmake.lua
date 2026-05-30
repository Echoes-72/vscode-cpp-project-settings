set_config("buildir", ".vscode/build")

-- 定义交叉工具链
toolchain("aarch64")
    set_kind("standalone")
    set_sdkdir("/run/media/echo/C/Code_Enviroment/CrossToolchains/gcc-arm-11.2-2022.02-x86_64-aarch64-none-linux-gnu/")
toolchain_end()

target("App")
    set_kind("binary")

    set_encodings("source:utf-8", "target:utf-8")

    add_includedirs("include")
    add_files("main.cpp","src/*.cpp","src/rknn/*.cpp","src/net/*.cpp")
    add_syslinks("pthread")

    if is_mode("cross") then
        set_toolchains("aarch64")

        local target_plat= "linux"
        local target_arch= "aarch64"
        local depsdir = path.join(os.scriptdir(), "res/deps")
        -- 依赖名称
        local deps = {"opencv","ffmpeg","rknn","rga"}
        -- 包含目录和库目录
        local includirs,libdirs = {},{}

        for _, dep in ipairs(deps) do
            local includir = path.join(depsdir, dep, "include")
            local libdir = path.join(depsdir, dep, "lib")

            if os.isdir(includir) then
                add_includedirs(includir)
                table.insert(includirs,includir)
            end

            if os.isdir(libdir) then
                add_linkdirs(libdir)
                table.insert(libdirs,libdir)
            end
        end

        set_plat(target_plat)
        set_arch(target_arch)
        -- 方便查看信息,改为.so后缀
        set_filename("Orangepi.so")

        -- 编译连接配置
        add_rpathdirs("$ORIGIN/lib",{runpath = true})
        add_ldflags("-Wl,--allow-shlib-undefined")
        set_optimize("fastest")
        add_links("opencv_core","opencv_videoio","opencv_imgproc","opencv_imgcodecs")
        add_links("avcodec","avdevice","avformat","avfilter","avutil","swresample","swscale")
        add_links("rknn_api","rknnrt")
        add_links("rga")

        -- 设置安装目录结构
        local mode = get_config("mode")
        local installdir =path.join(os.scriptdir(),".vscode/bin",mode)
        set_installdir(installdir)
        set_prefixdir("",{bindir=""})

        -- 添加安装运行依赖库文件
        for _, libdir in ipairs(libdirs) do
            add_installfiles(libdir.."/*", {prefixdir = "lib"})
        end

        -- 添加资源文件
        add_installfiles(os.scriptdir().."/res/model/*", {prefixdir="model"})
        add_installfiles(os.scriptdir().."/res/testvideos/*",{prefixdir="testvideos"})

    end
