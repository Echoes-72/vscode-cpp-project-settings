#include "../Include/Calculate.hpp"
#include "../Include/Evolution.hpp"
#include <queue>
#define BottomLeftCorner \
    ChannelGraph[AliveCell[before].x * MapSize.cy + AliveCell[before].y][AliveCell[after].x * MapSize.cy + AliveCell[after].y]
#define TopRightCorner \
    ChannelGraph[AliveCell[after].x * MapSize.cy + AliveCell[after].y][AliveCell[before].x * MapSize.cy + AliveCell[before].y]
static vector<vector<POINT>> Path(1);
extern SIZE MapSize;
extern vector<vector<HttpRequest>> Map;
extern vector<POINT> AliveCell;
extern vector<vector<vector<POINT>>> ChannelGraph;
vector<vector<CalculateData>> CalculateMap = vector<vector<CalculateData>>(MapSize.cx, vector<CalculateData>(MapSize.cy));
//* A* Algorithm 比较器
struct Comparator
{
    static POINT End;
    bool operator()(const POINT &a, const POINT &b)
    {
        return (abs(a.x - End.x) + abs(a.y - End.y)) > (abs(b.x - End.x) + abs(b.y - End.y));
    }
};
POINT Comparator::End = { -1, -1 };
//* A* Algorithm
POINT *A_Star_Algorithm(POINT Start, POINT End)
{
    static int Flag = 1;
    Comparator::End = End;
    priority_queue<POINT, vector<POINT>, Comparator> Mem;
    Mem.push(Start);
    CalculateMap[Start.x][Start.y].Visited = Flag;
    CalculateMap[Start.x][Start.y].Before  = { -1, -1 };
    POINT Now;
    while (!Mem.empty())
    {
        Now = Mem.top();
        Mem.pop();
        if (Now.x == End.x && Now.y == End.y)
        {
            Flag++;
            // POINT Temp = Now;
            return new POINT(Now);
        }
        if (Now.x > 0 && Map[Now.x - 1][Now.y].State && CalculateMap[Now.x - 1][Now.y].Visited != Flag)
        {
            Mem.push({ Now.x - 1, Now.y });
            CalculateMap[Now.x - 1][Now.y].Visited = Flag;
            CalculateMap[Now.x - 1][Now.y].Before  = Now;
        }
        if (Now.x < MapSize.cx - 1 && Map[Now.x + 1][Now.y].State && CalculateMap[Now.x + 1][Now.y].Visited != Flag)
        {
            Mem.push({ Now.x + 1, Now.y });
            CalculateMap[Now.x + 1][Now.y].Visited = Flag;
            CalculateMap[Now.x + 1][Now.y].Before  = Now;
        }
        if (Now.y > 0 && Map[Now.x][Now.y - 1].State && CalculateMap[Now.x][Now.y - 1].Visited != Flag)
        {
            Mem.push({ Now.x, Now.y - 1 });
            CalculateMap[Now.x][Now.y - 1].Visited = Flag;
            CalculateMap[Now.x][Now.y - 1].Before  = Now;
        }
        if (Now.y < MapSize.cy - 1 && Map[Now.x][Now.y + 1].State && CalculateMap[Now.x][Now.y + 1].Visited != Flag)
        {
            Mem.push({ Now.x, Now.y + 1 });
            CalculateMap[Now.x][Now.y + 1].Visited = Flag;
            CalculateMap[Now.x][Now.y + 1].Before  = Now;
        }
    }
    Flag++;
    return nullptr;
}
//* 计算线程执行函数
DWORD WINAPI Calculation(LPVOID Paramter)
{
    ChannelGraph = vector<vector<vector<POINT>>>(MapSize.cy * MapSize.cx, vector<vector<POINT>>(MapSize.cy * MapSize.cx));
    int num      = AliveCell.size();
    for (int before = 0; before < num - 1; before++)
    {
        for (int after = before + 1; after < num; after++)
        {
            POINT *Temp = A_Star_Algorithm(AliveCell[before], AliveCell[after]);
            if (Temp != nullptr)
            {
                POINT temp = *Temp;
                delete Temp;
                while (temp.x != -1)
                {
                    TopRightCorner.push_back(temp);
                    BottomLeftCorner.insert(BottomLeftCorner.begin(), temp);
                    temp = CalculateMap[temp.x][temp.y].Before;
                }
            }
        }
    }
    PostMessage(EvolutionData::FatherWindow, static_cast<EvolutionData *>(Paramter)->WM_SELF, 0, 0);
    return static_cast<EvolutionData *>(Paramter)->WM_SELF;
}