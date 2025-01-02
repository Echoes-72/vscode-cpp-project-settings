#include "mandelbrot.hpp"

#include <cstdio>


int main()
{
    const int xdim         = 500;
    const int ydim         = 500;
    const int max_iter     = 100;
    int image[xdim * ydim] = { 0 };
    mandelbrot(image, xdim, ydim, max_iter);
    for (int y = 0; y < ydim; y += 10)
    {
        for (int x = 0; x < xdim; x += 5)
        {
            putchar(image[y * xdim + x] < max_iter ? '.' : '#');
        }
        putchar('\n');
    }
    return 0;
}