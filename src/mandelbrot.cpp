#include "mandelbrot.hpp"

#include <complex.h>

void mandelbrot(int image[], int xdim, int ydim, int max_iter)
{
    for (int y = 0; y < ydim; ++y)
    {
        for (int x = 0; x < xdim; ++x)
        { // <<<<< Breakpoint here
            std::complex<float> xy(-2.05 + x * 3.0 / xdim, -1.5 + y * 3.0 / ydim);
            std::complex<float> z(0, 0);
            int count = max_iter;
            for (int i = 0; i < max_iter; ++i)
            {
                z = z * z + xy;
                if (std::abs(z) >= 2)
                {
                    count = i;
                    break;
                }
            }
            image[y * xdim + x] = count;
        }
    }
}