
/* blitwizard game engine - source code file

  Copyright (C) 2013 Jonas Thiem

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

#include <stdio.h>
#include <string.h>

#ifdef USE_GRAPHICS

#include "graphics.h"
#include "graphics2dsprites.h"
#include "graphicstexture.h"
#include "threading.h"
#include "graphicstexturemanager.h"

static mutex* m = NULL;

// data structure for a sprite:
struct graphics2dsprite {
    // this is set if the sprite was deleted:
    int deleted;
    // (it will be kept around until things with
    // the texture manager callbacks are sorted out)

    // texture path:
    char* path;

    // set to 1 if there was an error loading things:
    int loadingError;

    // texture dimensions (initially 0, 0 if not known):
    size_t texWidth, texHeight;

    // pinned to camera with given id (or -1 if not):
    int pinnedToCamera;

    // texture info:
    struct graphicstexture* tex;
    struct texturerequesthandle* request;

    // position, size info:
    double x, y, width, height, angle;
    // width, height = 0 means height should match texture geometry

    // texture clipping window:
    int clippingEnabled;
    size_t clippingX, clippingY;
    size_t clippingWidth, clippingHeight;

    // alpha, color and visibility:
    double alpha;
    double r, g, b, a;
    int visible;

    // z index:
    int zindex;

    // enabled for sprite events:
    int enabledForEvent[SPRITE_EVENT_TYPE_COUNT];

    // untouchable/transparent for sprite event:
    int invisibleForEvent[SPRITE_EVENT_TYPE_COUNT];

    // pointers for global linear sprite list:
    struct graphics2dsprite* next,* prev;

    // custom userdata:
    void* userdata;
};
// global linear sprite list:
static struct graphics2dsprite* spritelist = NULL;
static struct graphics2dsprite* spritelistEnd = NULL;

// the "lowest" sprites enabled for each event:
static struct graphics2dsprite* lastEventSprite[SPRITE_EVENT_TYPE_COUNT];
// all the sprites enabled for a given event (which are visible):
int eventSpriteCount[SPRITE_EVENT_TYPE_COUNT];

// initialise threading mutex and sprite event info:
__attribute__((constructor)) static void graphics2dsprites_init(void) {
    m = mutex_Create();

    // reset event info:
    int i = 0;
    while (i < SPRITE_EVENT_TYPE_COUNT) {
        lastEventSprite[i] = NULL;
        eventSpriteCount[i] = 0;
        i++;
    }
}

static void graphics2dsprites_findLastEventSprites(void) {
    int i = 0;
    while (i < SPRITE_EVENT_TYPE_COUNT) {
        if (!lastEventSprite[i]) {
            if (eventSpriteCount[i] > 0) {
                // search for first sprite that is enabled for this event:
                struct graphics2dsprite* s = spritelist;
                while (s) {
                    if (s->enabledForEvent[i] && s->visible) {
                        lastEventSprite[i] = s;
                        return;
                    }
                    s = s->next;
                }
            }
        }
        i++;
    }
}

static void graphics2dsprites_fixClippingWindow(struct
graphics2dsprite* sprite) {
    if (!sprite->texWidth || !sprite->texHeight
    || !sprite->clippingEnabled) {
        // size not known yet, we cannot fix cliping window.
        // (or no clipping active)
        return;
    }
    // X, Y offset shouldn't be out of boundaries:
    if (sprite->clippingX > sprite->texWidth
    && sprite->clippingY > sprite->texHeight) {
        sprite->clippingX = sprite->texWidth;
        sprite->clippingY = sprite->texHeight;
        return;
    }
    // limit width, height to texture boundaries:
    if (sprite->clippingX + sprite->clippingWidth >= sprite->texWidth) {
        sprite->clippingWidth = sprite->texWidth - sprite->clippingX;
    }
    if (sprite->clippingY + sprite->clippingHeight >= sprite->texHeight) {
        sprite->clippingY = sprite->texHeight - sprite->clippingY;
    }
}

// this callback will be called by the texture manager:
static void graphics2dsprites_dimensionInfoCallback(
struct texturerequesthandle* request, size_t width, size_t height,
void* userdata) {
    mutex_Lock(m);
    struct graphics2dsprite* s = userdata;

    if (s->deleted) {
        mutex_Release(m);
        return;
    }

    s->texWidth = width;
    s->texHeight = height;
    if (s->texWidth == 0 && s->texHeight == 0) {
        // texture failed to load.
        s->loadingError = 1;
    } else {
        graphics2dsprites_fixClippingWindow(s);
    }
    mutex_Release(m);
}

double graphics2dsprites_getAlpha(
struct graphics2dsprite* sprite) {
    return sprite->alpha;
}

void graphics2dsprites_setAlpha(
struct graphics2dsprite* sprite, double alpha) {
    if (alpha > 1) {
        alpha = 1;
    }
    if (alpha < 0) {
        alpha = 0;
    }
    sprite->alpha = alpha;
}

// this callback will be called by the texture manager:
static void graphics2dsprites_textureSwitchCallback(
struct texturerequesthandle* request,
struct graphicstexture* texture, void* userdata) {
    struct graphics2dsprite* s = userdata;

    mutex_Lock(m);

    if (s->deleted) {
        mutex_Release(m);
        return;
    }

    s->tex = texture;
    mutex_Release(m);
}

// Get texture dimensions if known:
int graphics2dsprites_getGeometry(struct graphics2dsprite* sprite,
size_t* width, size_t* height) {
    if (!sprite) {
        return 0;
    }
    mutex_Lock(m);
    if (sprite->loadingError) {
        // sprite failed to load
        mutex_Release(m);
        return 1;
    }
    // if a clipping window is set and final texture size is known,
    // report clipping window:
    if (sprite->texWidth && sprite->texHeight) {
        if (sprite->clippingWidth && sprite->clippingHeight) {
            *width = sprite->clippingWidth;
            *height = sprite->clippingHeight;
            mutex_Release(m);
            return 1;
        }
        // otherwise, report texture size if known:
        *width = sprite->texWidth;
        *height = sprite->texHeight;
        mutex_Release(m);
        return 1;
    }
    // no texture size known:
    mutex_Release(m);
    return 0;
}

// Check if actual texture is available:
int graphics2dsprites_isTextureAvailable(struct graphics2dsprite* sprite) {
    if (!sprite) {
        return 0;
    }

    mutex_Lock(m);
    if (sprite->tex) {
        mutex_Release(m);
        return 1;
    }
    mutex_Release(m);
    return 0;
}

void graphics2dsprites_setClippingWindow(struct graphics2dsprite* sprite,
size_t x, size_t y, size_t w, size_t h) {
    if (!sprite) {
        return;
    }
    mutex_Lock(m);
    sprite->clippingX = x;
    sprite->clippingY = y;
    sprite->clippingWidth = w;
    sprite->clippingHeight = h;
    sprite->clippingEnabled = 1;
    graphics2dsprites_fixClippingWindow(sprite);
    mutex_Release(m);
}

void graphics2dsprites_unsetClippingWindow(struct graphics2dsprite* sprite) {
    if (!sprite) {
        return;
    }
    mutex_Lock(m);
    sprite->clippingEnabled = 0;
    mutex_Release(m);
}

void graphics2dsprites_doForAllSprites(
void (*spriteInformation) (const char* path, struct graphicstexture* tex,
double x, double y, double width, double height,
size_t texWidth, size_t texHeight,
double angle, double alpha, double r, double g, double b,
size_t sourceX, size_t sourceY, size_t sourceWidth, size_t sourceHeight,
int visible, int cameraId)) {
    if (!spriteInformation) {
        return;
    }

    mutex_Lock(m);

    // walk through linear sprite list:
    struct graphics2dsprite* s = spritelist;
    while (s) {
        // call sprite information callback:
        if ((s->clippingWidth > 0 && s->clippingHeight > 0) ||
        !s->clippingEnabled) {
            size_t sx,sy,sw,sh;
            sx = s->clippingX;
            sy = s->clippingY;
            sw = s->clippingWidth;
            sh = s->clippingHeight;
            if (!s->clippingEnabled && s->texWidth && s->texHeight) {
                // reset to full texture size:
                sx = 0;
                sy = 0;
                sw = s->texWidth;
                sh = s->texHeight;
            }
            spriteInformation(s->path, s->tex, s->x, s->y, s->width, s->height,
            s->texWidth, s->texHeight, s->angle, s->alpha, s->r, s->g, s->b,
            sx, sy, sw, sh,
            s->visible, s->pinnedToCamera);
        } else {
            // report sprite as invisible:
            spriteInformation(s->path, s->tex, s->x, s->y, s->width, s->height,
            s->texWidth, s->texHeight, s->angle, s->alpha, s->r, s->g, s->b,
            s->clippingX, s->clippingY, s->clippingWidth, s->clippingHeight,
            0, s->pinnedToCamera);
        }

        s = s->next;
    }

    mutex_Release(m);
}

void graphics2dsprites_resize(struct graphics2dsprite* sprite,
double width, double height) {
    sprite->width = width;
    sprite->height = height;
}

static void graphics2dsprites_removeFromList(struct graphics2dsprite* sprite) {
    // remove sprite from list:
    if (sprite->prev) {
        sprite->prev->next = sprite->next;
    } else {
        spritelist = sprite->next;
    }
    if (sprite->next) {
        sprite->next->prev = sprite->prev;
    } else {
        spritelistEnd = sprite->prev;
    }
}

void graphics2dsprites_destroy(struct graphics2dsprite* sprite) {
    if (!sprite) {
        return;
    }
    mutex_Lock(m);

    // remove sprite from list:
    graphics2dsprites_removeFromList(sprite);

    // check if we're enabled for any event:
    int i = 0;
    while (i < SPRITE_EVENT_TYPE_COUNT) {
        if (sprite->enabledForEvent[i]) {
            eventSpriteCount[i]++;
            // last event sprite needs to be recalculated:
            lastEventSprite[i] = NULL;
        }
        i++;
    }

    // destroy texture manager request
    sprite->deleted = 1;
    if (sprite->request) {
        mutex_Release(m);
        texturemanager_destroyRequest(sprite->request);
        mutex_Lock(m);
        sprite->request = NULL;
    }
    // ... this will handle the texture aswell:
    sprite->tex = NULL;

    // remove sprite from list:
    graphics2dsprites_removeFromList(sprite);
 
    // free sprite:
    free(sprite);

    // done!
    mutex_Release(m);
}

void graphics2dsprites_move(struct graphics2dsprite* sprite,
double x, double y, double angle) {
    mutex_Lock(m);
    sprite->x = x;
    sprite->y = y;
    sprite->angle = angle;
    mutex_Release(m);
}

static void graphics2dsprites_addToList(struct graphics2dsprite* s) {
    // seek the earliest sprite (from the back)
    // which has a lower or equal zindex, and add us behind
    struct graphics2dsprite* s2 = spritelistEnd;
    while (s2 && (s2->zindex > s->zindex ||
    (s2->pinnedToCamera >= 0 && s->pinnedToCamera < 0))) {
        s2 = s2->prev;
    }
    if (s2) {
        s->next = s2->next;
        s->prev = s2;
    } else {
        s->next = spritelist;
        s->prev = NULL;
    }
    if (s->next) {
        s->next->prev = s;
    } else {
        spritelistEnd = s;
    }
    if (s->prev) {
        s->prev->next = s;
    } else {
        spritelist = s;
    }
}

struct graphics2dsprite* graphics2dsprites_create(
const char* texturePath, double x, double y, double width, double height) {
    if (!texturePath) {
        return NULL;
    }

    mutex_Lock(m);

    // create new sprite struct:
    struct graphics2dsprite* s = malloc(sizeof(*s));
    if (!s) {
        mutex_Release(m);
        return NULL;
    }
    memset(s, 0, sizeof(*s));

    // set position and size info to sprite:
    s->x = x;
    s->y = y;
    s->r = 1;
    s->g = 1;
    s->b = 1;
    s->pinnedToCamera = -1;
    s->alpha = 1;
    s->visible = 1;
    s->width = width;
    s->height = height;
    s->path = strdup(texturePath);
    s->zindex = 0;
    if (!s->path) {
        free(s);
        mutex_Release(m);
        return NULL;
    }
    s->visible = 1;

    mutex_Release(m);
    // get a texture request:
    s->request = texturemanager_requestTexture(
    s->path, graphics2dsprites_dimensionInfoCallback,
    graphics2dsprites_textureSwitchCallback,
    s);
    mutex_Lock(m);

    // add us to the list:
    graphics2dsprites_addToList(s);

    mutex_Release(m);

    return s;
}

void graphics2dsprites_setZIndex(struct graphics2dsprite* sprite,
int zindex) {
    mutex_Lock(m);
    if (!sprite || sprite->zindex == zindex) {
        // nothing to do.
        mutex_Release(m);
        return;
    }

    // remove us from the list:
    graphics2dsprites_removeFromList(sprite);

    // update zindex:
    sprite->zindex = zindex;

    // add us back to the list:
    graphics2dsprites_addToList(sprite);

    mutex_Release(m);
}

void graphics2dsprites_setPinnedToCamera(struct graphics2dsprite* sprite,
int cameraId) {
    if (!sprite) {
        return;
    }
    mutex_Lock(m);
    if (sprite->pinnedToCamera == cameraId) {
        mutex_Release(m);
        return;
    }

    // set new pinned state:
    sprite->pinnedToCamera = cameraId;

    // readd to list:
    graphics2dsprites_removeFromList(sprite);
    graphics2dsprites_addToList(sprite);

    // done.
    mutex_Release(m);
}

void graphics2dsprites_setVisible(struct graphics2dsprite* sprite,
int visible) {
    mutex_Lock(m);
    sprite->visible = (visible != 0);
    if (!sprite->visible) {
        // check if we were enabled for any event:
        int i = 0;
        while (i < SPRITE_EVENT_TYPE_COUNT) {
            if (sprite->enabledForEvent[i]) {
                eventSpriteCount[i]--;
                // last event sprite needs to be recalculated:
                lastEventSprite[i] = NULL;
            }
            i++;
        }
    } else {
        // check if we're enabled for any event:
        int i = 0;
        while (i < SPRITE_EVENT_TYPE_COUNT) {
            if (sprite->enabledForEvent[i]) {
                eventSpriteCount[i]++;
                // last event sprite needs to be recalculated:
                lastEventSprite[i] = NULL;
            }
            i++;
        }
    }
    mutex_Release(m);
}

void graphics2dsprite_calculateSizeOnScreen(
struct graphics2dsprite* sprite,
int cameraId,
int* screen_x, int* screen_y, int* screen_w,
int* screen_h) {
    // get camera settings:
    double zoom = graphics_GetCamera2DZoom(cameraId);
    double ratio = graphics_GetCamera2DAspectRatio(cameraId);
    double centerx = graphics_GetCamera2DCenterX(cameraId);
    double centery = graphics_GetCamera2DCenterY(cameraId);
    double w = graphics_GetCameraWidth(cameraId);
    double h = graphics_GetCameraHeight(cameraId);

    // calculate visible area on screen:
    double x,y;
    if (sprite->pinnedToCamera < 0) {
        x = ((sprite->x - centerx + (w/2) /
            UNIT_TO_PIXELS) * zoom) * UNIT_TO_PIXELS;
        y = ((sprite->y - centery + (h/2) /
            UNIT_TO_PIXELS) * zoom) * UNIT_TO_PIXELS;
    } else {
        x = sprite->x * UNIT_TO_PIXELS;
        y = sprite->y * UNIT_TO_PIXELS;
    }
    double width = sprite->width;
    double height = sprite->height;
    if (width == 0 && height == 0) {
        width = sprite->texWidth * (UNIT_TO_PIXELS/UNIT_TO_PIXELS_DEFAULT);
        height = sprite->texHeight * (UNIT_TO_PIXELS/UNIT_TO_PIXELS_DEFAULT);
        if (sprite->clippingWidth > 0) {
            width = sprite->clippingWidth * (UNIT_TO_PIXELS/UNIT_TO_PIXELS_DEFAULT);
            height = sprite->clippingHeight * (UNIT_TO_PIXELS/UNIT_TO_PIXELS_DEFAULT);
        }
    } else {
        width *= UNIT_TO_PIXELS;
        height *= UNIT_TO_PIXELS;
    }   

    // set visible area:
    *screen_x = x;
    *screen_y = y;
    *screen_w = width;
    *screen_h = height;
}

void graphics2dsprites_reportVisibility(void) {
    int c = graphics_GetCameraCount();
    mutex_Lock(m);
    struct graphics2dsprite* sprite = spritelist;
    while (sprite) {
        if (!sprite->texWidth || !sprite->texHeight
        || sprite->loadingError) {
            sprite = sprite->next;
            continue;
        }
        // cycle through all cameras:
        int i = 0;
        while (i < c) {
            if (i != sprite->pinnedToCamera
            && sprite->pinnedToCamera >= 0) {
                // not visible on this camera.
                i++;
                continue;
            }

            // get camera settings:
            int sw = graphics_GetCameraWidth(i);
            int sh = graphics_GetCameraHeight(i);

            // get sprite pos on screen:
            int x, y, w, h;
            graphics2dsprite_calculateSizeOnScreen(sprite,
            i, &x, &y, &w, &h);

            // now check if rectangle is on screen:
            if (((x >= 0 && x < sw) || (x + w >= 0 && x + w < sw) ||
            (x < 0 && x + w >= sw))
            &&
            ((y >= 0 && y < sh) || (y + h >= 0 && y + h < sh) ||
            (y < 0 && y + h >= sh))) {
                // it is.
                texturemanager_usingRequest(sprite->request,
                USING_AT_VISIBILITY_DETAIL);
            }

            i++;
        }

        sprite = sprite->next;
    }
    mutex_Release(m); 
}

void graphics2dsprites_setInvisibleForEvent(struct graphics2dsprite* sprite,
int event, int invisible) {
    mutex_Lock(m);
    sprite->invisibleForEvent[event] = invisible;
    mutex_Release(m);
}

void graphics2dsprites_enableForEvent(
struct graphics2dsprite* sprite, int event, int enabled) {
    mutex_Lock(m);
    if (enabled == sprite->enabledForEvent[event]) {
        // nothing changes.
        mutex_Release(m);
        return;
    }
    sprite->enabledForEvent[event] = enabled;
    int recalculate = 0;
    if (enabled) {
        if (sprite->visible) {
            eventSpriteCount[event]++;
            recalculate = 1;
        }
    } else {
        if (sprite->visible) {
            eventSpriteCount[event]--;
            recalculate = 1;
        }
    }
    if (recalculate) {
        // recalculate last event sprite:
        lastEventSprite[event] = NULL;
        graphics2dsprites_findLastEventSprites();
    }
    mutex_Release(m);
}

struct graphics2dsprite*
graphics2dsprites_getSpriteAtScreenPos(
int cameraId, int mx, int my, int event) {
    mutex_Lock(m);

    // first, update lastEventSprite entries if necessary:
    graphics2dsprites_findLastEventSprites();

    // go through sprite list from end
    // (on top) to beginning (below)
    struct graphics2dsprite* sprite = spritelistEnd;
    while (sprite) {
        // skip sprite if disabled for event:
        if ((!sprite->enabledForEvent[event] && 
        sprite->invisibleForEvent[event]) || !sprite->visible) {
            sprite = sprite->prev;
            continue;
        }

        // get sprite pos on screen:
        int x, y, w, h;
        graphics2dsprite_calculateSizeOnScreen(sprite,
        cameraId, &x, &y, &w, &h);

        // check if sprite pos is under mouse pos:
        if (mx >= x && mx < x + w &&
        my >= y && my < y + h) {
            // mouse on this sprite
            mutex_Release(m);
            return sprite;
        }

        if (sprite == lastEventSprite[event]) {
            // this was the last one we needed to check!
            mutex_Release(m);
            return NULL;
        }
        sprite = sprite->prev;
    }
    mutex_Release(m);
    return NULL;
}

void graphics2dsprites_setUserdata(struct graphics2dsprite* sprite,
void* data) {
    mutex_Lock(m);
    sprite->userdata = data;
    mutex_Release(m);
}

void* graphics2dsprites_getUserdata(struct graphics2dsprite* sprite) {
    mutex_Lock(m);
    void* udata = sprite->userdata;
    mutex_Release(m);
    return udata;
}

size_t graphics2dsprites_Count(void) {
    size_t count = 0;
    mutex_Lock(m);
    struct graphics2dsprite* sprite = spritelist;
    while (sprite) {
        count++;
        sprite = sprite->next;
    }
    mutex_Release(m);
    return count;
}

#endif


