#include "../Include/Cell.hpp"

HDC HttpRequest::Hdc;
ofstream &operator<<(ofstream &Out, const HttpRequest &cell)
{
    Out << cell.Position.x << "\t" << cell.Position.y << "\t" << cell.Wealth << "\t" << cell.Color;
    return Out;
}