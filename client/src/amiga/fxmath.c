#include "fxmath.h"

static const signed char sine[32] = { 0, 1, 2, 2, 3, 3, 4, 4, 4, 4, 4, 3, 3, 2, 2, 1, 0, -1, -2, -2, -3, -3, -4, -4, -4, -4, -4, -3, -3, -2, -2, -1 };   /* round(4 sin(2 pi k / 32)) */

signed char fx_wave(unsigned char phase)
{
    return sine[phase >> 3];
}
