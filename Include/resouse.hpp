#pragma once
#include <windows.h>
#define ATTRIBUTE TEXT("  细胞自动机里的每个细胞都具有一定的财富值(简称W) 存在于地图上的每个细胞之间还会\
存在承诺值(简称C) 对于属于同一对的双方来说 这个承诺值是一样的")
#define ABOUT TEXT("   细胞自动机是一个时间和空间都离散的动态系统 每个细胞自动机都是由细胞单元(cell)组成的规则网格 \
每个细胞元都有 k 个可能的状态 其当前状态由其本身及周围细胞元的前一状态共同决定.")
#define ACCELERATOR TEXT("新建:\t\t\tCtrl+O\n保存:\t\t\tCtrl+C\n读取:\t\t\tCtrl+G\n属性:\t\t\tCtrl+A\n关于:\t\t\tCtrl+R\n\
快捷键:\t\tCtrl+S\n水平滚动条置零:\thome\n水平滚动条置最大:\tend\n垂直滚动条置零:\tShift+home\n垂直滚动条置最大:\
\tShift+end\nGO:\t\t\tEnter\nSTOP:\t\t\tSpace\n实现思路:\t\tCtrl+I\n实现工具:\t\tCtrl+T")
#define SAVE TEXT("是否保存当前文件?")
#define IDEA TEXT("    该程序绝大部分是使用Windows API来实现的 没有使用到第三方库 在实现过程中 使用Visual Studio Code作为编辑器 g++作为编译器 用到了一点点html和xml。\
\n    main.cpp文件是最主要的代码文件 它实现了程序大部分的GUI。Include文件夹下包涵的Caculate.hpp Cell.hpp Evolution.h三个头文件分别定义了计算路径、细胞体、演化有关的类和函数 \
对应的在Src文件夹下有Caculate.cpp Cell.cpp Evolution.cpp三个源文件来实现相关函数功能。Resource文件夹下包含了resource.rc文件和main.exe.xml两个文件 \
前者是的资源脚本文件 这里定义了所有的对话框 菜单栏 窗口的视觉样式 和部分的快捷键 后者使用xml编写 和.rc文件一起用于规定窗口的视觉样式。\
Data文件夹下的Cell.txt用于存放存档数据。Build下的.o文件是编译过程生成的目标文件。\n\
    该程序的实现思路是 首先定义了一个细胞体类 它包含了细胞的位置 状态 财富值 颜色等信息 对于细胞财富Wealth在视觉上的展现 使用RGB来表现:对于一定的Wealth值 其颜色是RGB(max(0 255-Wealth) min(255 max(0 510-Wealth) min(255 max(0 765-Wealth)) \
由此可以看出一个细胞财富最大值是765 最小值为0 越富有的细胞颜色越接近黑色 越贫穷的越接近白色。在由贫穷到富有的变化过程中 颜色是从白色到蓝色再变为黑色。整个可以绘制的版图是50x50 屏幕放不下 因此添加了垂直和水平滚动条 \
窗口被分成了两块部分 左边是数据区 右边是演化区.由于精力有限 没考虑窗口大小变化时的窗口布局 因此干脆通过拦截相应的消息把窗口设置成无法调节大小的状态。\n     \
在演化过程里 如果一个细胞想对另外一个细胞产生影响 那么它会通过这两个细胞之间的一条最短路径来传递影响 因此在演化开始前 会使用CreateThread函数来创建一个线程(线程函数就是Calcution函数) 这个线程的功能是使用广度优先计算出任意两个活细胞之间的最短路径.然后将结果存放于一个邻接矩阵ChannelGraph里 \
这个零阶矩阵矩阵是一个全局变量 在演化过程中 每个细胞都可以访问到这个矩阵 从而可以快速查询它和其他活细胞细胞之间的最短路径 从而可以对其他细胞产生影响。两个活细胞之间的关系不仅仅是最短路径 还有承诺值(范围是0-1) 这个承诺值是在演化过程里动态变化的 为了可以快速访问它 \
这里依然采用了邻接矩阵来记录它。而该细胞自动机的核心是活细胞的不断演化 该任务是由另外一个线程来完成的 线程函数是Evolution函数 它的功能是不断地进行活细胞之间的交互 程序设置的是每0.1秒进行一次演化 并刷新一次页面。演化的过程是 随机挑选五分之一的活细胞作为演化的发起者 然后根据题目里的规定从\
其他活细胞里挑选出属于它的最优对象 进行交互 至于交互的细节 见题目。\n\
    之所以计算和演化开辟了两个线程 是因为这两项工作都是耗时的工作 会长时间阻塞主线程。主线程主要进行的GUI的渲染 通过与主线程GUI的交互 可以随时控制其他连个线程的开始与停止\
在本程序的实现过程里 涉及到了线程安全 因为这三个线程在同时工作的过程里会程序同时访问需改多个全局变量的情况 这个使用EnterCriticalSection和LeaveCriticalSection来解决。")

#define TOOL TEXT("g++ -v 13.1.0 (x86_64-posix-seh-rev1)\nVisual Studo Code\nWindows API\nC++11\n")
#define ID_BUTGO 1
#define ID_BUTSTOP 2 
#define ID_BUTERASE 3
#define ID_EDITOR 4
#define ID_TIMER 5
#define ID_PROBAR 6
#define ID_STATICTEXT 7
#define ID_MENU 1000
#define ID_FILE_NEW 1001
#define ID_FILE_SAVE 1002
#define ID_FILE_READ 1003
#define ID_HELP_ATTRIBUTE 1004
#define ID_HELP_ABOUT 1005
#define ID_HELP_ACCELERATORS 1006
#define ID_CODE_IDEA 1007
#define ID_CODE_TOOL 1008
#define IDR_ACC 1009
#define ID_DIALOG_ATTRIBUTE 10010
#define ID_DIALOG_ABOUT 10011
#define ID_DIALOG_ACCELERATORS 1012
#define ID_DIALOG_SAVEORNOT 1013
#define ID_LEFT 1016  
#define ID_RIGHT 1017
#define ID_UP 1018
#define ID_DOWN 1019 
#define ID_SPACE 1020
#define ID_RETURN 1021
#define ID_H_HOME 1022
#define ID_V_HOME 1023
#define ID_H_END 1024
#define ID_V_END 1025
#define WM_DONE (WM_USER+1)