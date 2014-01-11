
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

#include "config.h"
#include "os.h"

#ifdef USE_GRAPHICS

#include <string.h>

#include "graphics.h"
#include "luaheader.h"
#include "luafuncs_objectgraphics.h"
#include "luafuncs_objectphysics.h"
#include "luafuncs_object.h"
#include "luastate.h"
#include "graphics2dsprites.h"
#include "poolAllocator.h"

struct poolAllocator* objectGraphicsAllocator = NULL;
__attribute__((constructor)) void
    luacfuncs_objectgraphics_initAllocator(void) {
    objectGraphicsAllocator = poolAllocator_create(
    sizeof(struct objectgraphicsdata), 0);
}

// load object graphics. might do nothing if too many have been 
// loaded this frame!
void luafuncs_objectgraphics_load(struct blitwizardobject* o,
const char* resource) {
    if (o->deleted == 1) {
        return;
    }

    double x,y,z;
    objectphysics_getPosition(o, &x, &y, &z);

    if (!resource || strlen(resource) <= 0) {
        return;
    }

    // create graphics thing if necessary:
    if (!o->graphics) {
        o->graphics = poolAllocator_alloc(objectGraphicsAllocator);
        if (!o->graphics) {
            // memory allocation error
            return;
        }
        memset(o->graphics, 0, sizeof(*(o->graphics)));
    }

    if (!o->is3d && !o->graphics->sprite) {
        o->graphics->sprite = graphics2dsprites_create(
        resource, x, y, 0, 0);
        graphics2dsprites_setUserdata(o->graphics->sprite, o);
        o->scale2d.x = 1;
        o->scale2d.y = 1;
    }
}

static int object_checkgraphics(struct blitwizardobject* o) {
    // check for graphics struct and attempt to create it:
    if (!o->graphics) {
        luafuncs_objectgraphics_load(o, o->respath);
        if (!o->graphics) {
            return 0;
        }
    }
    return 1;
}

void luacfuncs_objectgraphics_setAlpha(struct blitwizardobject* o,
double alpha) {
    if (!object_checkgraphics(o)) {
        return;
    }
    if (o->is3d) {
        // update 3d mesh alpha
    } else {
        // update sprite alpha
        o->graphics->alpha = alpha;
        if (o->graphics->sprite) {
            graphics2dsprites_setAlpha(o->graphics->sprite, alpha);
        }
    }
}

double luacfuncs_objectgraphics_getAlpha(struct blitwizardobject* o) {
    if (!object_checkgraphics(o)) {
        return 1;
    }
    if (o->is3d) {
        // get 3d mesh alpha
    } else {
        // get 2d sprite alpha
        if (o->graphics->sprite) {
            return graphics2dsprites_getAlpha(o->graphics->sprite);
        }
    }
    return o->graphics->alpha;
}

void luacfuncs_objectgraphics_updateParallax(struct blitwizardobject* o) {
    if (!object_checkgraphics(o)) {
        return;
    }
    if (!o->is3d && o->graphics->sprite) {
        graphics2dsprites_setParallaxEffect(o->graphics->sprite,
        o->parallax);
    }
}

void luacfuncs_objectgraphics_updatePosition(struct blitwizardobject* o) {
    if (!object_checkgraphics(o)) {
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
            graphics2dsprites_move(o->graphics->sprite, x, y, angle);
        }
        // update sprite scale if geometry is known
        if (o->scalechanged) {
            size_t w,h;
            if (graphics2dsprites_getGeometry(o->graphics->sprite,
            &w, &h)) {
                o->scalechanged = 0;
                double horiFlipFactor = 1;
                if (o->horiFlipped) {
                    horiFlipFactor = -1;
                }
                double vertiFlipFactor = 1;
                if (o->vertiFlipped) {
                    vertiFlipFactor = -1;
                }
                graphics2dsprites_resize(o->graphics->sprite,
                ((double)w) * o->scale2d.x * horiFlipFactor /
                    ((double)UNIT_TO_PIXELS_DEFAULT),
                ((double)h) * o->scale2d.y * vertiFlipFactor /
                    ((double)UNIT_TO_PIXELS_DEFAULT));
            }
        }
    }
}

void luafuncs_objectgraphics_unload(struct blitwizardobject* o) {
    if (o->is3d) {

    } else {
        if (o->graphics && o->graphics->sprite) {
            graphics2dsprites_destroy(o->graphics->sprite);
            o->graphics->sprite = NULL;
        }
    }
    if (o->graphics) {
        poolAllocator_free(objectGraphicsAllocator, o->graphics);
        o->graphics = NULL;
    }
}

int luafuncs_objectgraphics_needGeometryCallback(
struct blitwizardobject* o) {
    if (!o->graphics || o->graphics->geometryCallbackDone) {
        return 0;
    }

    if (o->is3d) {
        return 0;
    } else {
        if (!o->graphics->sprite) {
            return 0;
        }
        size_t w,h;
        if (graphics2dsprites_getGeometry(o->graphics->sprite,
        &w, &h)) {
            o->graphics->geometryCallbackDone = 1;
            return 1;
        }
        return 0;
    }
}

int luafuncs_objectgraphics_needVisibleCallback(
struct blitwizardobject* o) {
    if (!object_checkgraphics(o)) {
        return 0;
    }
    if (o->graphics->visibilityCallbackDone) {
        return 0;
    }
    if (o->is3d) {
        return 0;
    } else {
        if (!o->graphics->sprite) {
            return 0;
        }
        if (graphics2dsprites_isTextureAvailable(o->graphics->sprite
        )) {
            o->graphics->visibilityCallbackDone = 1;
            return 1;
        }
        return 0;
    }
}

int luacfuncs_objectgraphics_getOriginalDimensions(
struct blitwizardobject* o, double *x, double *y, double *z) {
    if (!object_checkgraphics(o)) {
        return 0;
    }
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
        if (graphics2dsprites_getGeometry(o->graphics->sprite,
        &w, &h)) {
            if (w || h) {
                *x = (double)w / ((double)UNIT_TO_PIXELS_DEFAULT);
                *y = (double)h / ((double)UNIT_TO_PIXELS_DEFAULT);
                return 1;
            }
            return 0;
        }
        return 0;
    }
}

void luacfuncs_objectgraphics_newFrame() {
}

void luacfuncs_objectgraphics_unsetTextureClipping(
struct blitwizardobject* o) {
    if (!object_checkgraphics(o)) {  
        return;
    }
    if (o->is3d) {
        // FIXME: handle 3d decals
    } else {
        if (o->graphics->sprite) {
            graphics2dsprites_unsetClippingWindow(o->graphics->sprite);
        }
    }
}


void luacfuncs_objectgraphics_setTextureClipping(struct blitwizardobject* o,
size_t x, size_t y, size_t width, size_t height) {
    if (!object_checkgraphics(o)) {
        return;
    }
    if (o->is3d) {
        // FIXME: 3d decals
    } else {
        if (o->graphics->sprite) {
            graphics2dsprites_setClippingWindow(o->graphics->sprite,
            x, y, width, height);
        }
    }
}

void luacfuncs_objectgraphics_pinToCamera(struct blitwizardobject* o,
int id) {
    if (!object_checkgraphics(o)) {
        return;
    }
    if (!o->is3d) {
        if (o->graphics->sprite) {
            graphics2dsprites_setPinnedToCamera(o->graphics->sprite, id);
            o->graphics->pinnedToCamera = id;
        }
    }
}

int luacfuncs_objectgraphics_getVisible(struct blitwizardobject* o) {
    return o->visible;
}

void luacfuncs_objectgraphics_setVisible(struct blitwizardobject* o,
int visible) {
    o->visible = (visible != 0);
    if (!object_checkgraphics(o)) {
        return;
    }
    if (!o->is3d) {
        if (o->graphics->sprite) {
            graphics2dsprites_setVisible(o->graphics->sprite,
            (o->visible != 0));
        }
    }
}

void luacfuncs_objectgraphics_enableMouseClickEvent(struct blitwizardobject* o,
int enabled) {
    if (!object_checkgraphics(o)) {
        return;
    }
    if (!o->is3d) {
        if (o->graphics->sprite) {
            graphics2dsprites_enableForEvent(o->graphics->sprite,
            SPRITE_EVENT_TYPE_CLICK,
            enabled);
        }
    }
}

void luacfuncs_objectgraphics_enableMouseMoveEvent(struct blitwizardobject* o,
int enabled) {
    if (!object_checkgraphics(o)) {
        return;
    }
    if (!o->is3d) {
        if (o->graphics->sprite) {
            graphics2dsprites_enableForEvent(o->graphics->sprite,
            SPRITE_EVENT_TYPE_MOTION,
            enabled);
        }
    }
}

void luacfuncs_objectgraphics_setInvisibleToMouseEvents(
struct blitwizardobject* o, int enabled) {
    if (!object_checkgraphics(o)) {
        return;
    }
    if (!o->is3d) {
        if (o->graphics->sprite) {
            graphics2dsprites_setInvisibleForEvent(o->graphics->sprite,
            SPRITE_EVENT_TYPE_MOTION,
            enabled);
            graphics2dsprites_setInvisibleForEvent(o->graphics->sprite,
            SPRITE_EVENT_TYPE_CLICK,
            enabled);
            if (enabled) {
                // we should be invisible to event ->
                // we need to disable us for the events in addition.
                luacfuncs_objectgraphics_enableMouseMoveEvent(o, 0);
                luacfuncs_objectgraphics_enableMouseClickEvent(o, 0);
            } else {
                // we shall no longer be invisible ->
                // re-enable us for events.
                if (o->haveOnMouseEnter || o->haveOnMouseLeave) {
                    luacfuncs_objectgraphics_enableMouseMoveEvent(o, 1);
                }
                if (o->haveOnMouseClick) {
                    luacfuncs_objectgraphics_enableMouseClickEvent(o, 1);
                }
            }
        }
    }
}

void luacfuncs_objectgraphics_processMouseClick(int x, int y,
int button) {
    // find out which camera was clicked on:
    int cameraid = -1;
#ifdef USE_SDL_GRAPHICS
    cameraid = 0;
#else
    cameraid = graphics_getCameraAt(x, y);
    if (cameraid < 0) {
        // nothing further to check
        return;
    }
#endif
    if (cameraid < 0) {
        // no viewport/camera was clicked on.
        return;
    }

    // process mouse click for 2d objects:
    struct graphics2dsprite* s = graphics2dsprites_getSpriteAtScreenPos(
    cameraid, x, y, SPRITE_EVENT_TYPE_CLICK);
    if (s) {
        // get associated blitwizard 2d object:
        struct blitwizardobject* o = graphics2dsprites_getUserdata(s);

        // trigger onMouseClick event:
        lua_State* l = luastate_GetStatePtr();
        lua_pushnumber(l, ((double)x)/UNIT_TO_PIXELS);
        lua_pushnumber(l, ((double)y)/UNIT_TO_PIXELS);
        lua_pushnumber(l, button);
        luacfuncs_object_callEvent(l, o, "onMouseClick",
        3, 0);
    }
}

void luacfuncs_objectgraphics_processMouseMove(int x, int y) {

}

#endif

