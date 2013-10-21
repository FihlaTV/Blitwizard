
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

#include <limits.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

#ifdef USE_GRAPHICS

#include "graphics.h"
#include "graphics2dsprites.h"
#include "graphicstexture.h"
#include "threading.h"
#include "graphicstexturemanager.h"
#include "poolAllocator.h"

static mutex* m = NULL;

//#define SMOOTH_SPRITES
#ifdef SMOOTH_SPRITES
const float smoothmaxdist = 1.0f;
#endif

// data structure for a sprite:
struct graphics2dsprite {
    // this is set if the sprite was deleted:
    int deleted;
    // (it will be kept around until things with
    // the texture manager callbacks are sorted out)

    // texture filtering (1: enabled, 0: disabled)
    int textureFiltering;

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
#ifdef SMOOTH_SPRITES
    double prevx, prevy;
#endif
    // width, height = 0 means height should match texture geometry

    // texture clipping window:
    int clippingEnabled;
    size_t clippingX, clippingY;
    size_t clippingWidth, clippingHeight;

    // alpha, color and visibility:
    double alpha;
    double r, g, b, a;
    int visible;

    // parallax effect strength:
    double parallax;

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
static int spritesInListCount = 0;
static struct graphics2dsprite* spritelist = NULL;
static struct graphics2dsprite* spritelistEnd = NULL;

// sprite allocator:
struct poolAllocator* spriteAllocator = NULL;

// shortcut sprite list for jumping to a given zindex depth:
struct spriteShortcut {
    struct graphics2dsprite* entrance;
    int zindex;
    int nextzindex;
    int pinnedToCamera; // 1 if pinned, 0 if not
    int nextPinnedToCamera;
};
#define SPRITESHORTCUTCOUNT 20
struct spriteShortcut shortcut[SPRITESHORTCUTCOUNT];
int shortcutsFilled = 0;
int shortcutsNeedRecalculation = 0;

// the "lowest" sprites enabled for each event:
static struct graphics2dsprite* lastEventSprite[SPRITE_EVENT_TYPE_COUNT];
static int recalculateLastEventSprite[SPRITE_EVENT_TYPE_COUNT];
// all the sprites enabled for a given event (which are visible):
int eventSpriteCount[SPRITE_EVENT_TYPE_COUNT];

// initialise threading mutex and sprite event info:
__attribute__((constructor)) static void graphics2dsprites_init(void) {
    m = mutex_Create();

    // reset event info:
    int i = 0;
    while (i < SPRITE_EVENT_TYPE_COUNT) {
        lastEventSprite[i] = NULL;
        recalculateLastEventSprite[i] = 1;
        eventSpriteCount[i] = 0;
        i++;
    }

    // create our sprite allocator:
    spriteAllocator = poolAllocator_create(
    sizeof(struct graphics2dsprite), 1);
}

// recalculate sprite shortcuts:
static void graphics2dsprites_recalculateSpriteShortcuts(void) {
    shortcutsFilled = 0;
    int sinceLastShortcut = 0;
    int lastZIndex = 0;
    int lastPinnedState = 1;
    int spritesBetweenShortcuts = (spritesInListCount
        / ((double)SPRITESHORTCUTCOUNT * 1.5));
    struct graphics2dsprite* spr = spritelistEnd;
    while (spr) {
        if ((sinceLastShortcut >= spritesBetweenShortcuts &&
        (spr->zindex < lastZIndex ||
        (spr->pinnedToCamera < 0 && lastPinnedState)
        || sinceLastShortcut >= spritesBetweenShortcuts * 2))
        || shortcutsFilled == 0) {
            memset(&shortcut[shortcutsFilled], 0, sizeof(*shortcut));
            shortcut[shortcutsFilled].entrance = spr;
            shortcut[shortcutsFilled].zindex = spr->zindex;
            shortcut[shortcutsFilled].nextzindex = INT_MIN;
            shortcut[shortcutsFilled].pinnedToCamera =
            (spr->pinnedToCamera >= 0);
            shortcut[shortcutsFilled].nextPinnedToCamera = 0;
            if (shortcutsFilled > 0) {
                shortcut[shortcutsFilled-1].nextzindex = spr->zindex;
                shortcut[shortcutsFilled-1].nextPinnedToCamera =
                shortcut[shortcutsFilled].pinnedToCamera;
            }
            lastZIndex = spr->zindex;
            lastPinnedState = (spr->pinnedToCamera >= 0);
            sinceLastShortcut = 0;
            if (shortcutsFilled >= SPRITESHORTCUTCOUNT) {
                break;
            }
            shortcutsFilled++;
        }
        sinceLastShortcut++;
        spr = spr->prev;
    }
}

// This function searches the sprites with the lowest zindex
// which are still relevant for a given event.
// This allows an actual event search later to stop early when
// reaching the lowest relevant sprite, saving precious time.
static void graphics2dsprites_findLastEventSprites(void) {
    int i = 0;
    while (i < SPRITE_EVENT_TYPE_COUNT) {
        if (recalculateLastEventSprite[i]) {
            recalculateLastEventSprite[i] = 1;
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
__attribute__ ((unused)) struct texturerequesthandle* request,
size_t width, size_t height, void* userdata) {
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
__attribute__ ((unused)) struct texturerequesthandle* request,
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
void (*spriteInformation) (
const struct graphics2dsprite* sprite,
const char* path, struct graphicstexture* tex,
double r, double g, double b, double alpha,
int visible, int cameraId, int textureFiltering)) {
    if (!spriteInformation) {
        return;
    }

    mutex_Lock(m);

    // it might be a good idea to restore the jump list here
    // (although drawing doesn't need it)
    if (shortcutsNeedRecalculation > 0) {
        shortcutsNeedRecalculation = 0;
        graphics2dsprites_recalculateSpriteShortcuts();
    }

    // walk through linear sprite list:
    struct graphics2dsprite* s = spritelist;
    while (s) {
        // call sprite information callback:
        if ((s->clippingWidth > 0 && s->clippingHeight > 0) ||
        !s->clippingEnabled) {
            spriteInformation(
            s,
            s->path, s->tex, s->r, s->g, s->b, s->alpha,
            s->visible, s->pinnedToCamera, s->textureFiltering);
        } else {
            // report sprite as invisible:
            spriteInformation(
            s,
            s->path, s->tex, s->r, s->g, s->b, s->alpha,
            0, s->pinnedToCamera, s->textureFiltering);
        }

        s = s->next;
    }

    mutex_Release(m);
}

void graphics2dsprites_setTextureFiltering(struct graphics2dsprite* sprite,
int filter) {
    if (!sprite) {
        return;
    }
    mutex_Lock(m);
    sprite->textureFiltering = filter;
    mutex_Release(m);
}

void graphics2dsprites_resize(struct graphics2dsprite* sprite,
double width, double height) {
    sprite->width = width;
    sprite->height = height;
}

static void graphics2dsprites_removeFromList(struct graphics2dsprite* sprite) {
    if (!sprite->prev && !sprite->next &&
    spritelistEnd != sprite && spritelist != sprite) {
        // we're not actually in the list.
        return;
    }

    // if the jump list points at us, fix it:
    int i = 0;
    while (i < shortcutsFilled) {
        if ((shortcut[i].zindex < sprite->zindex &&
        shortcut[i].pinnedToCamera == sprite->pinnedToCamera) ||
        (!shortcut[i].pinnedToCamera && sprite->pinnedToCamera >= 0)) {
            // we are already past the possibly relevant entries
            break;
        }
        if (shortcut[i].entrance == sprite) {
            // make it point at the next (less on top, previous in list)
            // sprite since that is now the relevant entry point.
            shortcut[i].entrance = sprite->prev;
            if (sprite->prev) {
                shortcut[i].zindex = sprite->prev->zindex;
                shortcut[i].pinnedToCamera =
                (sprite->prev->pinnedToCamera >= 0);
            }
        }
        i++;
    }

    // remember we might need to recalculate the whole jump list soon:
    shortcutsNeedRecalculation++;

    spritesInListCount--;

    // if this is enabled for an event, remember to recalculate
    // last event sprite if necessary:
    i = 0;
    while (i < SPRITE_EVENT_TYPE_COUNT) {
        if (sprite->enabledForEvent[i]
        && lastEventSprite[i] == sprite) {
            recalculateLastEventSprite[i] = 1;
        }
        i++;
    }

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
    sprite->prev = NULL;
    sprite->next = NULL;
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
    if (sprite->visible) {
        while (i < SPRITE_EVENT_TYPE_COUNT) {
            if (sprite->enabledForEvent[i]) {
                eventSpriteCount[i]--;
                // last event sprite needs to be recalculated:
                lastEventSprite[i] = NULL;
            }
            i++;
        }
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

    // free resource path:
    if (sprite->path) {
        free(sprite->path);
    } 

    // free sprite:
    poolAllocator_free(spriteAllocator, sprite);

    // done!
    mutex_Release(m);
}

void graphics2dsprites_move(struct graphics2dsprite* sprite,
double x, double y, double angle) {
    mutex_Lock(m);
#ifdef SMOOTH_SPRITES
    sprite->prevx = x;
    sprite->prevy = y;
#endif
    sprite->x = x;
    sprite->y = y;
    sprite->angle = angle;
    mutex_Release(m);
}

static void graphics2dsprites_addToList(struct graphics2dsprite* s) {
#if (!defined(NDEBUG) && defined(EXTRADEBUG))
    mutex_Release(m);
    assert(graphics2dsprites_Count()
    <= texturemanager_getRequestCount());
    mutex_Lock(m);
#endif

    // seek the earliest sprite (from the back)
    // which has a lower or equal zindex, and add us behind
    struct graphics2dsprite* s2 = spritelistEnd;

    // if the jump list is very outdated, redo it:
    if (shortcutsNeedRecalculation > 30) {
        shortcutsNeedRecalculation = 0;
        graphics2dsprites_recalculateSpriteShortcuts();
    }

    // check the jump list first:
    int i = 0;
    while (i < shortcutsFilled-1) {
        if (shortcut[i].nextzindex <= s->zindex ||
        (s->pinnedToCamera >= 0 && !shortcut[i].nextPinnedToCamera)) {
            // the next jump shortcut is past the interesting sprites,
            // so use this one:
            if (shortcut[i].entrance) {
                s2 = shortcut[i].entrance;
            }
            break;
        }
        i++;
    }

    // search to the actual exact position (SLOW!)
    while (s2 && (s2->zindex > s->zindex ||
    (s2->pinnedToCamera >= 0 && s->pinnedToCamera < 0))) {
        s2 = s2->prev;
    }

    // remember we need to recalculate the jump list at some point:
    shortcutsNeedRecalculation++;

    spritesInListCount++;

    // add us into the list:
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

#if (!defined(NDEBUG) && defined(EXTRADEBUG))
    mutex_Release(m);
    assert(graphics2dsprites_Count()
    <= texturemanager_getRequestCount());
    mutex_Lock(m);
#endif
}

static size_t graphics2dsprites_Count_internal(void) {
    size_t count = 0;
    struct graphics2dsprite* sprite = spritelist;
    while (sprite) {
        count++;
        sprite = sprite->next;
    }
    return count;
}

struct graphics2dsprite* graphics2dsprites_create(
const char* texturePath, double x, double y, double width, double height) {
    if (!texturePath) {
        return NULL;
    }

    mutex_Lock(m);

    // create new sprite struct:
    struct graphics2dsprite* s = poolAllocator_alloc(spriteAllocator);
    if (!s) {
        mutex_Release(m);
        texturemanager_releaseFromTextureAccess();
        return NULL;
    }
    memset(s, 0, sizeof(*s));

    // set position and size info to sprite:
    s->x = x;
    s->y = y;
    s->r = 1;
    s->g = 1;
    s->b = 1;
    s->textureFiltering = 1;
    s->pinnedToCamera = -1;
    s->alpha = 1;
    s->visible = 0;
    s->width = width;
    s->height = height;
    s->path = strdup(texturePath);
    s->zindex = 0;
    s->parallax = 1;
    if (!s->path) {
        poolAllocator_free(spriteAllocator, s);
        mutex_Release(m);
        texturemanager_releaseFromTextureAccess();
        return NULL;
    }

    mutex_Release(m);
    // get a texture request:
#ifdef EXTRADEBUG
    assert(graphics2dsprites_Count()
    <= texturemanager_getRequestCount());
#endif
    s->request = texturemanager_requestTexture(
    s->path, graphics2dsprites_dimensionInfoCallback,
    graphics2dsprites_textureSwitchCallback,
    s);
#ifdef EXTRADEBUG
    assert(graphics2dsprites_Count()
    <= texturemanager_getRequestCount());
#endif
    mutex_Lock(m);

    // add us to the list:
    graphics2dsprites_addToList(s);

    mutex_Release(m);

    return s;
}

int graphics2dsprites_getZIndex(struct graphics2dsprite* sprite) {
    mutex_Lock(m);
    int z = sprite->zindex;
    mutex_Release(m);
    return z;
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

int graphics2dsprites_getVisible(struct graphics2dsprite* sprite) {
    mutex_Lock(m);
    int r = sprite->visible;
    mutex_Release(m);
    return r;
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

void graphics2dsprites_setParallaxEffect(struct graphics2dsprite* sprite,
double value) {
    if (value <= 0) {
        value = 1;
    }
    mutex_Lock(m);
    sprite->parallax = value;
    mutex_Release(m);
}

void graphics2dsprite_calculateSizeOnScreen(
const struct graphics2dsprite* sprite,
int cameraId,
double* screen_x, double* screen_y, double* screen_w,
double* screen_h, double* screen_sourceX, double* screen_sourceY,
double* screen_sourceW, double* screen_sourceH,
double* source_angle, int* phoriflip, int compensaterotation) {
    assert(cameraId >= 0);
    assert(sprite);
    // get camera settings:
    double zoom = graphics_GetCamera2DZoom(cameraId);
    double ratio = graphics_GetCamera2DAspectRatio(cameraId);
    double centerx = graphics_GetCamera2DCenterX(cameraId);
    double centery = graphics_GetCamera2DCenterY(cameraId);
    assert(zoom > 0);

    // various size info things:
    size_t sourceWidth, sourceHeight;
    size_t texWidth, texHeight;
    texWidth = sprite->texWidth;
    texHeight = sprite->texHeight;
    sourceWidth = sprite->clippingWidth;
    sourceHeight = sprite->clippingHeight;
    if (!sprite->clippingEnabled) {
        sourceWidth = texWidth;
        sourceHeight = texHeight;
    }
    double angle = sprite->angle;
    struct graphicstexture* tex = sprite->tex;
    double sourceX = sprite->clippingX;
    double sourceY = sprite->clippingY;    
    double width = sprite->width;
    double height = sprite->height;
    double x = sprite->x;
    double y = sprite->y;

    // get actual texture size (texWidth, texHeight are theoretical
    // texture size of full sized original texture)
    size_t actualTexW, actualTexH;
    if (tex) {
        graphics_GetTextureDimensions(tex, &actualTexW, &actualTexH);
    } else {
        actualTexW = texWidth;
        actualTexH = texHeight;
    }

    // if the actual texture is upscaled or downscaled,
    // we need to take this into account:
    if ((texWidth != actualTexW || texHeight != actualTexH) &&
    (texWidth != 0 && texHeight != 0)) {
        double scalex = (double)actualTexW / (double)texWidth;
        double scaley = (double)actualTexH / (double)texHeight;

        // scale all stuff according to this:
        sourceX = scalex * (double)sourceX;
        sourceY = scaley * (double)sourceY;
        sourceWidth = scalex * (double)sourceWidth;
        sourceHeight = scaley * (double)sourceHeight;
    }

    // now override texture size:
    texWidth = actualTexW;
    texHeight = actualTexH;

    // if a camera is specified for pinning,
    // the sprite will be pinned to the screen:
    int pinnedToCamera = 0;
    if (sprite->pinnedToCamera >= 0) {
        pinnedToCamera = 1;
    }

    // evaluate special width/height:
    // negative for flipping, zero'ed etc
    int horiflip = 0;
    if (width == 0 && height == 0) {
        // if no size given, go to standard size:
        width = ((double)texWidth) / UNIT_TO_PIXELS_DEFAULT;
        height = ((double)texHeight) / UNIT_TO_PIXELS_DEFAULT;
        if (sourceWidth > 0) {
            // if source width/height given, set clipping accordingly:
            width = ((double)sourceWidth) / UNIT_TO_PIXELS_DEFAULT;
            height = ((double)sourceHeight) / UNIT_TO_PIXELS_DEFAULT;
        }
    }
    if (width < 0) {
        width = -width;
        horiflip = 1;
    }
    if (height < 0) {
        angle += 180;
        horiflip = !horiflip;
    }

    // scale position according to zoom:
    if (!pinnedToCamera) {
        x *= UNIT_TO_PIXELS * zoom;
        y *= UNIT_TO_PIXELS * zoom;
    } else {
        // ignore zoom when pinned to screen
        x *= UNIT_TO_PIXELS;
        y *= UNIT_TO_PIXELS;
    }

    if (!pinnedToCamera) {
        // image center offset (when not pinned to screen):
        x -= (width/2.0) * UNIT_TO_PIXELS * zoom;
        y -= (height/2.0) * UNIT_TO_PIXELS * zoom;

        // screen center offset (if not pinned):
        unsigned int winw,winh;
        graphics_GetWindowDimensions(&winw, &winh);
        x += (winw/2.0);
        y += (winh/2.0);

        // move according to zoom, cam pos etc:
        width *= UNIT_TO_PIXELS * zoom;
        height *= UNIT_TO_PIXELS * zoom;
        x -= (centerx / sprite->parallax) * UNIT_TO_PIXELS * zoom;
        y -= (centery / sprite->parallax) * UNIT_TO_PIXELS * zoom;
    } else {
        // only adjust width/height to proper units when pinned
        width *= UNIT_TO_PIXELS;
        height *= UNIT_TO_PIXELS;
    }

    // set resulting info:
    *screen_x = x;
    *screen_y = y;
    *screen_w = width;
    *screen_h = height;
    if (screen_sourceX) {
        *screen_sourceX = sourceX;
    }
    if (screen_sourceY) {
        *screen_sourceY = sourceY;
    }
    if (screen_sourceW) {
        *screen_sourceW = sourceWidth;
    }
    if (screen_sourceH) {
        *screen_sourceH = sourceHeight;
    }
    if (source_angle) {
        *source_angle = angle;
    }
    if (phoriflip) {
        *phoriflip = horiflip;
    }
}

void graphics2dsprites_reportVisibility(void) {
    int c = graphics_GetCameraCount();
    texturemanager_lockForTextureAccess();
    mutex_Lock(m);
    struct graphics2dsprite* sprite = spritelist;
    while (sprite) {
        if (!sprite->texWidth || !sprite->texHeight
        || sprite->loadingError || !sprite->tex) {
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
            double x, y, w, h;
            graphics2dsprite_calculateSizeOnScreen(sprite,
            i, &x, &y, &w, &h, NULL, NULL, NULL, NULL, NULL, NULL, 1);

            // now check if rectangle is on screen:
            if ((((x >= 0 && x < sw) || (x + w >= 0 && x + w < sw) ||
            (x < 0 && x + w >= sw))
            &&
            ((y >= 0 && y < sh) || (y + h >= 0 && y + h < sh) ||
            (y < 0 && y + h >= sh)))) {
                // it is.
                texturemanager_usingRequest(sprite->request,
                USING_AT_VISIBILITY_DETAIL);
            }

            i++;
        }

        sprite = sprite->next;
    }
    mutex_Release(m); 
    texturemanager_releaseFromTextureAccess();
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
            if (lastEventSprite[event] == sprite) {
                // we were the last, so now it must be someone else:
                recalculate = 1;
            }
        }
    }
    if (recalculate) {
        // force recalculation of last event sprite:
        recalculateLastEventSprite[event] = 1;
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
        double x, y, w, h;
        graphics2dsprite_calculateSizeOnScreen(sprite,
        cameraId, &x, &y, &w, &h, NULL, NULL, NULL, NULL, NULL, NULL, 0);

        // FIXME: consider rotation here!
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
    mutex_Lock(m);
    size_t c = graphics2dsprites_Count_internal();
    mutex_Release(m);
    return c;
}

#endif


