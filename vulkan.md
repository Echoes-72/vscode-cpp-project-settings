<h1>Vulkan-ArchLinux</h1>

<h2>一.环境配置</h2>

- 在 Linux 上开发 Vulkan 应用程序所需的最重要组件是 Vulkan 加载器、验证层和一些命令行实用程序，用于测试您的机器是否支持 Vulkan。
 
        sudo pacman -S vulkan-devel #来安装上述所有必需的工具
    安装完成后运行`vkcube`来验证设备可行性。

- Vulkan 本身是一个与平台无关的 API，不包括创建窗口以显示渲染结果的工具。为了利用 Vulkan 的跨平台优势并避免 X11 的恐怖，我们将使用 GLFW 库来创建一个窗口，该窗口支持 Windows、Linux 和 MacOS。

        sudo pacman -S glm #安装glfw

- 还需要一个程序将着色器从人类可读的 GLSL 编译为字节码。Google 的 glslc 具有类似于 GCC 和 Clang 的用法(Vulkan本身只识别 `SPIR-V`,也可以使用 `HLSL`编写,但需要另外的工具转化)。
  
        sudo pacman -S shaderc #安装glslc

    安装后,运行`glslc`验证。