#pragma once
#include <windows.h>
#define ATTRIBUTE TEXT("  Each cell in the cellular automaton has a certain wealth value (abbreviated as W). There is also a commitment value (abbreviated as C) between each cell on the map. For both parties belonging to the same pair, this commitment value is the same.")
#define ABOUT TEXT("   A cellular automaton is a dynamic system that is discrete in both time and space. Each cellular automaton consists of a regular grid of cell units (cells). Each cell unit has k possible states, and its current state is determined by its own and the previous states of the surrounding cell units.")
#define ACCELERATOR TEXT("New:\t\t\tCtrl+O\nSave:\t\t\tCtrl+C\nLoad:\t\t\tCtrl+G\nProperties:\t\t\tCtrl+A\nAbout:\t\t\tCtrl+R\n\
Shortcuts:\t\tCtrl+S\nHorizontal Scrollbar to Zero:\thome\nHorizontal Scrollbar to Max:\tend\nVertical Scrollbar to Zero:\tShift+home\nVertical Scrollbar to Max:\
\tShift+end\nGO:\t\t\tEnter\nSTOP:\t\t\tSpace\nImplementation Idea:\t\tCtrl+I\nImplementation Tools:\t\tCtrl+T")
#define SAVE TEXT("Do you want to save the current file?")
#define IDEA TEXT("    Most of this program is implemented using Windows API without using third-party libraries. During the implementation process, Visual Studio Code was used as the editor, g++ as the compiler, and a little bit of HTML and XML was used.\
\n    The main.cpp file is the main code file, which implements most of the program's GUI. The Include folder contains three header files: Caculate.hpp, Cell.hpp, and Evolution.h, which define classes and functions related to calculation paths, cell bodies, and evolution, respectively. Correspondingly, there are three source files in the Src folder: Caculate.cpp, Cell.cpp, and Evolution.cpp, which implement related functions. The Resource folder contains the resource.rc file and main.exe.xml file. The former is a resource script file that defines all dialogs, menus, window visual styles, and some shortcuts. The latter is written in XML and, together with the .rc file, is used to specify the window's visual style.\
The Cell.txt file in the Data folder is used to store save data. The .o files in the Build folder are object files generated during the compilation process.\n\
    The implementation idea of this program is to first define a cell body class, which contains information such as the cell's position, state, wealth value, and color. The visual representation of the cell's wealth (Wealth) is expressed using RGB: for a certain Wealth value, its color is RGB(max(0, 255-Wealth), min(255, max(0, 510-Wealth)), min(255, max(0, 765-Wealth))).\
From this, it can be seen that the maximum wealth value of a cell is 765, and the minimum value is 0. The richer the cell, the closer its color is to black, and the poorer it is, the closer it is to white. During the transition from poor to rich, the color changes from white to blue and then to black. The entire drawable map is 50x50, which cannot fit on the screen, so vertical and horizontal scrollbars are added.\
The window is divided into two parts: the data area on the left and the evolution area on the right. Due to limited energy, the window layout was not considered when the window size changes, so the window is set to a non-resizable state by intercepting the corresponding messages.\n     \
During the evolution process, if one cell wants to influence another cell, it will transmit the influence through the shortest path between the two cells. Therefore, before the evolution begins, a thread is created using the CreateThread function (the thread function is the Calcution function). The function of this thread is to use breadth-first search to calculate the shortest path between any two living cells and store the results in an adjacency matrix ChannelGraph. This adjacency matrix is a global variable. During the evolution process, each cell can access this matrix to quickly query the shortest path between it and other living cells, thereby influencing other cells. The relationship between two living cells is not only the shortest path but also the commitment value (ranging from 0 to 1). This commitment value dynamically changes during the evolution process. To quickly access it, an adjacency matrix is also used to record it. The core of this cellular automaton is the continuous evolution of living cells. This task is completed by another thread, and the thread function is the Evolution function. Its function is to continuously interact between living cells. The program is set to evolve once every 0.1 seconds and refresh the page once. The evolution process is to randomly select one-fifth of the living cells as the initiators of the evolution, and then select the optimal objects belonging to them from other living cells according to the rules in the problem for interaction. For the details of the interaction, see the problem.\n\
    The reason for creating two threads for calculation and evolution is that these two tasks are time-consuming and will block the main thread for a long time. The main thread mainly performs GUI rendering. By interacting with the main thread GUI, the other two threads can be controlled to start and stop at any time.\
In the implementation process of this program, thread safety is involved because these three threads will simultaneously access and modify multiple global variables. This is solved using EnterCriticalSection and LeaveCriticalSection.")

#define TOOL TEXT("clang -v 19.1.0 (x86_64-pc-windows-msv)\nVisual Studio Code\nWindows API\n")
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