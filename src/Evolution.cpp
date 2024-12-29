#include "../Include/Evolution.hpp"

#include <vector>

extern SIZE MapSize;
extern vector<vector<HttpRequest>> Map;
extern vector<vector<vector<POINT>>> ChannelGraph;
extern vector<POINT> AliveCell;
extern vector<vector<float>> CommitmentGraph;
// #include <iostream>
// #include <algorithm>
#define TributePath                                                                            \
    ChannelGraph[AliveCell[Actors[ActorTurn]].x * MapSize.cy + AliveCell[Actors[ActorTurn]].y] \
                [AliveCell[Demander].x * MapSize.cy + AliveCell[Demander].y]
#define CommitAToF                                                                                \
    CommitmentGraph[AliveCell[Actors[ActorTurn]].x * MapSize.cy + AliveCell[Actors[ActorTurn]].y] \
                   [TributePath[i].x * MapSize.cy + TributePath[i].y]
#define CommitFToA                                                    \
    CommitmentGraph[TributePath[i].x * MapSize.cy + TributePath[i].y] \
                   [AliveCell[Actors[ActorTurn]].x * MapSize.cy + AliveCell[Actors[ActorTurn]].y]
#define ComitDToE \
    CommitmentGraph[AliveCell[Demander].x * MapSize.cy + AliveCell[Demander].y][TributePath[i].x * MapSize.cy + TributePath[i].y]
#define ComitEToD \
    CommitmentGraph[TributePath[i].x * MapSize.cy + TributePath[i].y][AliveCell[Demander].x * MapSize.cy + AliveCell[Demander].y]
#define ComitAToD                                                                                 \
    CommitmentGraph[AliveCell[Actors[ActorTurn]].x * MapSize.cy + AliveCell[Actors[ActorTurn]].y] \
                   [AliveCell[Demander].x * MapSize.cy + AliveCell[Demander].y]
#define ComitDToA                                                               \
    CommitmentGraph[AliveCell[Demander].x * MapSize.cy + AliveCell[Demander].y] \
                   [AliveCell[Actors[ActorTurn]].x * MapSize.cy + AliveCell[Actors[ActorTurn]].y]
#define WT Map[AliveCell[Demander].x][AliveCell[Demander].y].Wealth
#define WA Map[AliveCell[Actors[ActorTurn]].x][AliveCell[Actors[ActorTurn]].y].Wealth
using namespace std;
DWORD EvolutionData::EvolutionThreadID;
CRITICAL_SECTION EvolutionData::CriticalSection;
HWND EvolutionData::FatherWindow;
extern RECT PaintArea, DataArea;
extern constexpr int CUBE = 25;
// constexpr int LIMIT = 600;
//*演化执行函数
DWORD WINAPI Evoulution(LPVOID Paramter)
{
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (msg.message == WM_TIMER)
        {
            EnterCriticalSection(&EvolutionData::CriticalSection);
            for (auto &i : AliveCell)
            {
                Map[i.x][i.y].Wealth = min(765, Map[i.x][i.y].Wealth + 10);
                Map[i.x][i.y].Color =
                    RGB(max(0, 255 - Map[i.x][i.y].Wealth),
                        min(255, max(0, 510 - Map[i.x][i.y].Wealth)),
                        min(255, max(0, 765 - Map[i.x][i.y].Wealth)));
            }
            bool Jump = false;
            if (AliveCell.size() < 2)
            {
                LeaveCriticalSection(&EvolutionData::CriticalSection);
                Jump = true;
            }
            vector<INT> Actors = vector<INT>(max(AliveCell.size() / 3, (size_t)1));
            for (int i = 0; i < Actors.size() && !Jump;)
            {
                Actors[i]   = rand() % AliveCell.size();
                BOOL Repeat = false;
                for (int j = 0; j < i; j++)
                {
                    if (Actors[i] == Actors[j])
                    {
                        Repeat = true;
                        break;
                    }
                }
                if (!Repeat) i++;
            }
            for (int ActorTurn = 0; ActorTurn < Actors.size() && !Jump; ActorTurn++)
            {
                int Demander = 0, Judge = 0, MaxValePos = -1;
                for (; Demander < AliveCell.size(); Demander++)
                {
                    if (TributePath.size() != 0)
                    {
                        BOOL Unobstructed = false;
                        for (int i = 1; i < TributePath.size() - 1; i++)
                        {
                            int ChoJionA = CommitmentGraph[AliveCell[Actors[ActorTurn]].x * MapSize.cy + AliveCell[Actors[ActorTurn]].y]
                                                          [TributePath[i].x * MapSize.cy + TributePath[i].y];
                            int ChoJionD = CommitmentGraph[AliveCell[Demander].x * MapSize.cy + AliveCell[Demander].y]
                                                          [TributePath[i].x * MapSize.cy + TributePath[i].y];
                            if (ChoJionA <= ChoJionD)
                            {
                                Unobstructed = true;
                                break;
                            }
                        }
                        if (WA == 0) WA = WA + 10;
                        if (!Unobstructed && Judge <= WT / WA)
                        {
                            Judge      = WT / WA;
                            MaxValePos = Demander;
                        }
                    }
                }
                if (Judge == 0) continue;
                Demander        = MaxValePos;
                int UnityWealth = Map[AliveCell[Actors[ActorTurn]].x][AliveCell[Actors[ActorTurn]].y].Wealth;
                // int UnitComitt = 1;
                for (int i = 1; i < TributePath.size() - 1; i++)
                {
                    UnityWealth += CommitAToF * Map[TributePath[i].x][TributePath[i].y].Wealth;
                    // UnitComitt += CommitAToF;
                }
                int PayCost    = min(150, Map[AliveCell[Demander].x][AliveCell[Demander].y].Wealth);
                int FightCost  = UnityWealth / 2;
                int TempValue1 = Map[AliveCell[Actors[ActorTurn]].x][AliveCell[Actors[ActorTurn]].y].Wealth;
                int TempValue2 = Map[AliveCell[Demander].x][AliveCell[Demander].y].Wealth;
                if (PayCost <= FightCost)
                {
                    Map[AliveCell[Actors[ActorTurn]].x][AliveCell[Actors[ActorTurn]].y].Wealth =
                        min(765, Map[AliveCell[Actors[ActorTurn]].x][AliveCell[Actors[ActorTurn]].y].Wealth + PayCost / 4);
                    Map[AliveCell[Demander].x][AliveCell[Demander].y].Wealth =
                        max(Map[AliveCell[Demander].x][AliveCell[Demander].y].Wealth - PayCost, 10);
                    Map[AliveCell[Actors[ActorTurn]].x][AliveCell[Actors[ActorTurn]].y].Color =
                        RGB(max(0, 255 - Map[AliveCell[Actors[ActorTurn]].x][AliveCell[Actors[ActorTurn]].y].Wealth),
                            min(255, max(0, 510 - Map[AliveCell[Actors[ActorTurn]].x][AliveCell[Actors[ActorTurn]].y].Wealth)),
                            min(255, max(0, 765 - Map[AliveCell[Actors[ActorTurn]].x][AliveCell[Actors[ActorTurn]].y].Wealth)));
                    Map[AliveCell[Demander].x][AliveCell[Demander].y].Color =
                        RGB(max(0, 255 - Map[AliveCell[Demander].x][AliveCell[Demander].y].Wealth),
                            min(255, max(0, 510 - Map[AliveCell[Demander].x][AliveCell[Demander].y].Wealth)),
                            min(255, max(0, 765 - Map[AliveCell[Demander].x][AliveCell[Demander].y].Wealth)));
                    ComitAToD = min(ComitAToD + 0.1, (double)1);
                    ComitDToA = ComitAToD;
                }
                else
                {
                    Map[AliveCell[Actors[ActorTurn]].x][AliveCell[Actors[ActorTurn]].y].Wealth =
                        min(TempValue1, Map[AliveCell[Actors[ActorTurn]].x][AliveCell[Actors[ActorTurn]].y].Wealth);
                    Map[AliveCell[Demander].x][AliveCell[Demander].y].Wealth =
                        max(Map[AliveCell[Demander].x][AliveCell[Demander].y].Wealth, TempValue2);
                    Map[AliveCell[Actors[ActorTurn]].x][AliveCell[Actors[ActorTurn]].y].Color =
                        RGB(max(0, 255 - Map[AliveCell[Actors[ActorTurn]].x][AliveCell[Actors[ActorTurn]].y].Wealth),
                            min(255, max(0, 510 - Map[AliveCell[Actors[ActorTurn]].x][AliveCell[Actors[ActorTurn]].y].Wealth)),
                            min(255, max(0, 765 - Map[AliveCell[Actors[ActorTurn]].x][AliveCell[Actors[ActorTurn]].y].Wealth)));
                    Map[AliveCell[Demander].x][AliveCell[Demander].y].Color =
                        RGB(max(0, 255 - Map[AliveCell[Demander].x][AliveCell[Demander].y].Wealth),
                            min(255, max(0, 510 - Map[AliveCell[Demander].x][AliveCell[Demander].y].Wealth)),
                            min(255, max(0, 765 - Map[AliveCell[Demander].x][AliveCell[Demander].y].Wealth)));
                    for (int i = 1; i < TributePath.size() - 1; i++)
                    {
                        Map[TributePath[i].x][TributePath[i].y].Wealth = max(0, int(Map[TributePath[i].x][TributePath[i].y].Wealth));
                        Map[TributePath[i].x][TributePath[i].y].Color =
                            RGB(max(0, 255 - Map[TributePath[i].x][TributePath[i].y].Wealth),
                                min(255, max(0, 510 - Map[TributePath[i].x][TributePath[i].y].Wealth)),
                                min(255, max(0, 765 - Map[TributePath[i].x][TributePath[i].y].Wealth)));
                        CommitAToF = min(CommitAToF + 0.1, (double)1);
                        CommitFToA = CommitAToF;
                        ComitDToE  = max(ComitDToE - 0.1, (double)0);
                        ComitEToD  = ComitDToE;
                    }
                    Map[AliveCell[Demander].x][AliveCell[Demander].y].Wealth =
                        max(0, Map[AliveCell[Demander].x][AliveCell[Demander].y].Wealth);
                    ComitAToD = max(ComitAToD - 0.1, (double)0);
                    ComitDToA = ComitAToD;
                }
            }
            HBRUSH Brush = CreateSolidBrush(
                Map[static_cast<EvolutionData *>(Paramter)->ZoomCellPos->y][static_cast<EvolutionData *>(Paramter)->ZoomCellPos->x].Color
            );
            FillRgn(
                HttpRequest::Hdc,
                static_cast<EvolutionData *>(Paramter)->ZoomCellRegion,
                *static_cast<EvolutionData *>(Paramter)->Changed ? Brush : WHITE_BRUSH
            );
            FrameRgn(HttpRequest::Hdc, static_cast<EvolutionData *>(Paramter)->ZoomCellRegion, (HBRUSH)GetStockObject(BLACK_BRUSH), 1, 1);
            TCHAR WealthText[20];
            _stprintf_s(
                WealthText,
                TEXT("%d"),
                Map[static_cast<EvolutionData *>(Paramter)->ZoomCellPos->y][static_cast<EvolutionData *>(Paramter)->ZoomCellPos->x].Wealth
            );
            FillRect(HttpRequest::Hdc, &static_cast<EvolutionData *>(Paramter)->ValueRect, WHITE_BRUSH);
            DrawTextEx(
                HttpRequest::Hdc,
                WealthText,
                _tcslen(WealthText),
                &static_cast<EvolutionData *>(Paramter)->ValueRect,
                DT_LEFT | DT_SINGLELINE,
                &static_cast<EvolutionData *>(Paramter)->DrawTextParams
            );
            DeleteObject(Brush);
            GetScrollInfo(EvolutionData::FatherWindow, SB_HORZ, &static_cast<EvolutionData *>(Paramter)->HScrollInfo);
            GetScrollInfo(EvolutionData::FatherWindow, SB_VERT, &static_cast<EvolutionData *>(Paramter)->VScrollInfo);
            int VPos = static_cast<EvolutionData *>(Paramter)->VScrollInfo.nPos;
            int HPos = static_cast<EvolutionData *>(Paramter)->HScrollInfo.nPos;
            RECT CELL;
            for (POINT &Temp : AliveCell)
            {
                if (Temp.x >= VPos && Temp.x < VPos + (PaintArea.bottom) / CUBE && Temp.y >= HPos &&
                    Temp.y < HPos + (PaintArea.right - PaintArea.left) / CUBE)
                {
                    CELL         = { DataArea.right + (Temp.y - HPos) * CUBE,
                                     DataArea.top + (Temp.x - VPos) * CUBE,
                                     DataArea.right + (Temp.y - HPos + 1) * CUBE,
                                     DataArea.top + (Temp.x - VPos + 1) * CUBE };
                    HBRUSH Brush = CreateSolidBrush(Map[Temp.x][Temp.y].Color);
                    FillRect(HttpRequest::Hdc, &CELL, Brush);
                    DeleteObject(Brush);
                }
            }
            LeaveCriticalSection(&EvolutionData::CriticalSection);
        }
    }
    return 0;
}