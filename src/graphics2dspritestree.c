
/* blitwizard game engine - source code file

  Copyright (C) 2014 Jonas Thiem

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

*/

#include "graphics2dspritestree.h"
#include "graphics.h"
static int getspritedimensions(struct graphics2dsprite* sprite,
        double* w, double* h) {
#ifdef USE_GRAPHICS
    if (!unittopixelsset) {
        return 0;
    }
    double w = sprite->width;
    double h = sprite->height;
    if (sprite->width == 0) {
        w = sprite->texWidth;
    }
    if (sprite->height == 0) {
        h = sprite->texHeight
    }
    w /= UNIT_TO_PIXELS;
    h /= UNIT_TO_PIXELS;
    return 1;
#else
    return 0;
#endif
}

static int getspritetopleft(struct graphics2dsprite* sprite,
        double* x, double* y) {
    double w,h;
    if (!getspritedimensions(sprite, &w, &h)) {
        w = 0;
        h = 0;
    }
    *x = (sprite->x) - (w / 2);
    *y = (sprite->y) - (h / 2);
    return 1;
}

#define CHEAPOFAKETREE

#ifdef CHEAPOFAKETREE

struct faketreeentry {
    struct graphics2dsprite* sprite;
    double addedx, addedy, addedw, addedh;
    struct faketreeentry* next;
};



#else

#endif

