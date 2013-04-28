
/* blitwizard game engine - source code file

  Copyright (C) 2012-2013 Jonas Thiem

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

#include "os.h"

#ifdef USE_GRAPHICS

#include <string.h>

#include "graphics.h"
#include "luafuncs_objectgraphics.h"
#include "luafuncs_objectphysics.h"
#include "graphics2dsprites.h"

void luafuncs_objectgraphics_load(struct blitwizardobject* o,
const char* resource) {
    double x,y,z;
    objectphysics_getPosition(o, &x, &y, &z);

    if (!resource || strlen(resource) <= 0) {
        return;
    }

    // create graphics thing if necessary:
    if (!o->graphics) {
        o->graphics = malloc(sizeof(*(o->graphics)));
        if (!o->graphics) {
            // memory allocation error
            return;
        }
        memset(o->graphics, 0, sizeof(*(o->graphics)));
    }

    if (!o->is3d) {
        o->graphics->sprite = graphics2dsprites_Create(
        resource, x, y, 0, 0);
        o->scale2d.x = 1;
        o->scale2d.y = 1;
    }
}

void luafuncs_objectgraphics_updatePosition(struct blitwizardobject* o) {
    if (!o->graphics) {
        return;
    }
    double x, y, z;
    objectphysics_getPosition(o, &x, &y, &z);
    if (o->is3d) {
        // update 3d mesh position
    } else {
        double angle;
        objectphysics_get2dRotation(o, &angle);
        // update sprite position
        if (o->graphics->sprite) {
            graphics2dsprites_Move(o->graphics->sprite, x, y, angle);
        }
        // update sprite scale if geometry is known
        size_t w,h;
        if (graphics2dsprites_GetGeometry(o->graphics->sprite,
        &w, &h)) {
            graphics2dsprites_Resize(o->graphics->sprite,
            ((double)w) * o->scale2d.x / ((double)UNIT_TO_PIXELS),
            ((double)h) * o->scale2d.y / ((double)UNIT_TO_PIXELS));
        }
    }
}

void luafuncs_objectgraphics_unload(struct blitwizardobject* o) {
    if (o->is3d) {

    } else {
        if (o->graphics && o->graphics->sprite) {
            graphics2dsprites_Destroy(o->graphics->sprite);
            o->graphics->sprite = NULL;
        }
    }
    if (o->graphics) {
        free(o->graphics);
        o->graphics = NULL;
    }
}

int luafuncs_objectgraphics_NeedGeometryCallback(
struct blitwizardobject* o) {
    if (o->graphics->geometryCallbackDone) {
        return 0;
    }

    if (o->is3d) {
        return 0;
    } else {
        if (!o->graphics->sprite) {
            return 0;
        }
        size_t w,h;
        if (graphics2dsprites_GetGeometry(o->graphics->sprite,
        &w, &h)) {
            o->graphics->geometryCallbackDone = 1;
            return 1;
        }
        return 0;
    }
}

int luafuncs_objectgraphics_NeedVisibleCallback(
struct blitwizardobject* o) {
    if (o->graphics->visibilityCallbackDone) {
        return 0;
    }
    if (o->is3d) {
        return 0;
    } else {
        if (!o->graphics->sprite) {
            return 0;
        }
        if (graphics2dsprites_IsTextureAvailable(o->graphics->sprite
        )) {
            o->graphics->visibilityCallbackDone = 1;
            return 1;
        }
        return 0;
    }
}

int luacfuncs_objectgraphics_getDimensions(
struct blitwizardobject* o, double *x, double *y, double *z) {
    if (o->is3d) {
        return 0;
    } else {
        if (!o->graphics->sprite) {
            // no-graphics object with zero size
            *x = 0;
            *y = 0;
            return 1;
        }
        size_t w,h;
        if (graphics2dsprites_GetGeometry(o->graphics->sprite,
        &w, &h)) {
            if (w || h) {
                *x = (double)w / ((double)UNIT_TO_PIXELS);
                *y = (double)h / ((double)UNIT_TO_PIXELS);
                return 1;
            }
            return 0;
        }
        return 0;
    }
}

#endif

