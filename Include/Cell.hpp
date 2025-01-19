#include <windows.h>
#include <fstream>
#include <CommCtrl.h>
#include <windowsx.h>
#include <tchar.h>
using namespace std;
#if !defined DEAD
#define DEAD false 
#define DefaultColor RGB(186,194,207)
#define ALIVE true
class Cell
{
    public:
    POINT Position = { 0, 0 };
    BOOL State = DEAD;
    INT Wealth = 10;
    COLORREF Color = DefaultColor;
    Cell() {};
    Cell(POINT Position, bool State): Position(Position), State(State) {}
    Cell(POINT Position, bool State, COLORREF Color, LPRECT Rect): Position(Position), State(State) {}
    static HDC Hdc;
    
    
};
ofstream& operator<<(ofstream& out, const Cell& cell);
#endif