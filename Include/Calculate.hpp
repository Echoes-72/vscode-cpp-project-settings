#pragma once
#include <windows.h>
#include "Cell.hpp"
using namespace std;
DWORD WINAPI Calculation(LPVOID Paramter);
//*路径求解标记
class CalculateData
{
public:
    POINT Before;
    int Visited=0;
};