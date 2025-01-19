#include "../Include/Cell.hpp"

int a = 0b1001'1110'1101;

HDC Cell::Hdc;
ofstream &operator<<(ofstream &Out, const Cell &cell)
{
    Out << cell.Position.x << "\t" << cell.Position.y << "\t" << cell.Wealth << "\t" << cell.Color;
    return Out;
}