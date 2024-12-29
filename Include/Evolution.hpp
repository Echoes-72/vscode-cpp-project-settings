#pragma once
#include "Cell.hpp"

class EvolutionData
{
public:
    static DWORD EvolutionThreadID;
    static CRITICAL_SECTION CriticalSection;
    static HWND FatherWindow;
    SCROLLINFO HScrollInfo = {sizeof(SCROLLINFO), SIF_POS}, VScrollInfo = {sizeof(SCROLLINFO), SIF_POS};
    DRAWTEXTPARAMS DrawTextParams = {sizeof(DRAWTEXTPARAMS), 4, 1, 0, 0};
    POINT *ZoomCellPos = nullptr;
    HRGN ZoomCellRegion = nullptr;
    UINT WM_SELF;
    RECT ValueRect;
    INT AliveNum = 0;
    BOOL *Changed;
};
DWORD WINAPI Evoulution(LPVOID Paramter);