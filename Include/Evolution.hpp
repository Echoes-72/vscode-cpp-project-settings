#pragma once
#include "Cell.hpp"
#include <vector>
extern SIZE MapSize;
extern vector<vector<Cell>> Map;
extern vector<vector<vector<POINT>>> ChannelGraph;
extern vector<POINT> AliveCell;
extern vector<vector<float>> CommitmentGraph;

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