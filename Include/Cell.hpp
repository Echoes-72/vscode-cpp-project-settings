#include <fstream>
#include <tchar.h>
#include <windows.h>

using namespace std;
#if !defined DEAD
    #define DEAD         false
    #define DefaultColor RGB(186, 194, 207)
    #define ALIVE        true
class HttpRequest
{
public:
    POINT Position = { 0, 0 };
    BOOL State     = DEAD;
    INT Wealth     = 10;
    COLORREF Color = DefaultColor;
    HttpRequest() {};
    HttpRequest(POINT Position, bool State) : Position(Position), State(State) {}
    HttpRequest(POINT Position, bool State, COLORREF Color, LPRECT Rect) : Position(Position), State(State) {}
    inline static SIZE Size = { 550, 25 };
    static HDC Hdc;
};
ofstream &operator<<(ofstream &out, const HttpRequest &cell);
#endif