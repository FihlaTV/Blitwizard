
/* blitwizard game engine - source code file

  Copyright (C) 2013-2014 Jonas Thiem

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

#include <math.h>
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
#include "graphics2dspritestruct.h"
#include "graphics2dspriteslist.h"
#include "graphics2dspritestree.h"

static mutex* m = NULL;

//#define SMOOTH_SPRITES
#ifdef SMOOTH_SPRITES
const float smoothmaxdist = 1.0f;
#endif

// global sprite counter:
static int spritesInListCount = 0;

// sprite allocator:
struct poolAllocator *spriteAllocator = NULL;

// counting the amount of times the zindex has changed on a sprite:
uint64_t currentzindexsetid = 0;

// the "lowest" sprites enabled for each event:
static struct graphics2dsprite *lastEventSprite[SPRITE_EVENT_TYPE_COUNT];
static int recalculateLastEventSprite[SPRITE_EVENT_TYPE_COUNT];
// all the sprites enabled for a given event (which are visible):
int eventSpriteCount[SPRITE_EVENT_TYPE_COUNT];

// initialise threading mutex and sprite event info:
__attribute__((constructor)) static void graphics2dsprites_init(void) {
    m = mutex_create();

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

static int graphics2dsprites_findLastEventSpritesCallback(
        struct graphics2dsprite *sprite, void *userdata) {
    int eventId = ((int)(int64_t)userdata);
    if (sprite->enabledForEvent[eventId] && sprite->visible) {
        lastEventSprite[eventId] = sprite;
    }
    return 1;
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
                // (from zindex top to bottom)
                graphics2dspriteslist_doForAllSpritesTopToBottom(
                    &graphics2dsprites_findLastEventSpritesCallback,
                    ((void*)(int64_t)i));
            }
        }
        i++;
    }
}

static void graphics2dsprites_fixClippingWindow(struct
        graphics2dsprite *sprite) {
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
        __attribute__ ((unused)) struct texturerequesthandle *request,
        size_t width, size_t height, void *userdata) {
    printf("dimensionInfoCallback: %llu, %llu\n",
        (unsigned long long)width, (unsigned long long)height);
    mutex_lock(m);
    struct graphics2dsprite *s = userdata;

    if (s->deleted) {
        mutex_release(m);
        return;
    }

    s->texWidth = width;
    s->texHeight = height;
    graphics2dspritestree_update(s);
    if (s->texWidth == 0 && s->texHeight == 0) {
        // texture failed to load.
        printf("texture failed to load: %s\n", s->path);
        s->loadingError = 1;
    } else {
        printf("texture loaded: %s\n", s->path);
        graphics2dsprites_fixClippingWindow(s);
    }
    mutex_release(m);
}

double graphics2dsprites_getAlpha(
        struct graphics2dsprite *sprite) {
    return sprite->alpha;
}

void graphics2dsprites_setAlpha(
        struct graphics2dsprite *sprite, double alpha) {
    if (alpha > 1) {
        alpha = 1;
    }
    if (alpha < 0) {
        alpha = 0;
    }
    sprite->alpha = alpha;
}

// this callback will be called by the texture manager:
static void graphics2dsprites_textureHandlingDoneCallback(
        __attribute__ ((unused)) struct texturerequesthandle *request,
        void *userdata) {
    struct graphics2dsprite *s = userdata;

    mutex_lock(m);

    if (s->deleted) {
        mutex_release(m);
        return;
    }

    s->textureHandlingDone = 1;
    mutex_release(m);
}

// this callback will be called by the texture manager:
static void graphics2dsprites_textureSwitchCallback(
        __attribute__ ((unused)) struct texturerequesthandle *request,
        struct graphicstexture *texture, void *userdata) {
    printf("textureSwitchCallback\n");
    struct graphics2dsprite *s = userdata;

    mutex_lock(m);

    if (s->deleted) {
        mutex_release(m);
        return;
    }

    s->tex = texture;
    mutex_release(m);
}

// Get texture dimensions if known:
int graphics2dsprites_getGeometry(struct graphics2dsprite *sprite,
        size_t *width, size_t *height) {
    if (!sprite) {
        return 0;
    }
    mutex_lock(m);
    if (sprite->loadingError) {
        // sprite failed to load
        mutex_release(m);
        return 1;
    }
    // if a clipping window is set and final texture size is known,
    // report clipping window:
    if (sprite->texWidth && sprite->texHeight) {
        if (sprite->clippingWidth && sprite->clippingHeight) {
            *width = sprite->clippingWidth;
            *height = sprite->clippingHeight;
            mutex_release(m);
            return 1;
        }
        // otherwise, report texture size if known:
        *width = sprite->texWidth;
        *height = sprite->texHeight;
        mutex_release(m);
        return 1;
    }
    // no texture size known:
    mutex_release(m);
    return 0;
}

// Check if actual texture is available:
int graphics2dsprites_isTextureAvailable(struct graphics2dsprite *sprite) {
    if (!sprite) {
        return 0;
    }

    mutex_lock(m);
    if (sprite->tex || sprite->textureHandlingDone) {
        // texture is there, or texture manager won't load it now.
        mutex_release(m);
        return 1;
    }
    mutex_release(m);
    return 0;
}

void graphics2dsprites_setClippingWindow(
        struct graphics2dsprite *sprite,
        size_t x, size_t y, size_t w, size_t h) {
    if (!sprite) {
        return;
    }
    mutex_lock(m);
    sprite->clippingX = x;
    sprite->clippingY = y;
    sprite->clippingWidth = w;
    sprite->clippingHeight = h;
    sprite->clippingEnabled = 1;
    graphics2dsprites_fixClippingWindow(sprite);
    mutex_release(m);
}

void graphics2dsprites_unsetClippingWindow(struct graphics2dsprite *sprite) {
    if (!sprite) {
        return;
    }
    mutex_lock(m);
    sprite->clippingEnabled = 0;
    mutex_release(m);
}

struct doforallspritesonscreendata {
    int (*spriteInformation) (
    struct graphics2dsprite *sprite,
    const char *path, struct graphicstexture *tex,
    double r, double g, double b, double alpha,
    int visible, int cameraId, int textureFiltering);
    int cameraId;
};

static int abortedearly = 0;
static int graphics2dsprites_doForAllSpritesCallback(
        struct graphics2dsprite *s, void *userdata) {
    // get info from userdata:
    struct doforallspritesonscreendata* data = userdata;
    // get callback from userdata:
    int (*spriteInformation) (
    struct graphics2dsprite *sprite,
    const char *path, struct graphicstexture *tex,
    double r, double g, double b, double alpha,
    int visible, int cameraId, int textureFiltering) = data->
        spriteInformation;
    if (s->pinnedToCamera != data->cameraId &&
            s->pinnedToCamera >= 0) {
        // sprite is pinned to other camera, skip.
        return 1;
    }
    // call sprite information callback:
    int r;
    if ((s->clippingWidth > 0 && s->clippingHeight > 0) ||
    !s->clippingEnabled) {
        r = spriteInformation(
        s,
        s->path, s->tex, s->r, s->g, s->b, s->alpha,
        s->visible, data->cameraId, s->textureFiltering);
    } else {
        // report sprite as invisible:
        r = spriteInformation(
        s,
        s->path, s->tex, s->r, s->g, s->b, s->alpha,
        0, data->cameraId, s->textureFiltering);
    }
    if (!r) {
        abortedearly = 1;
    }
    return r;
}

#define DOFORALL_SORT_NONE 0
#define DOFORALL_SORT_TOPTOBOTTOM 1
#define DOFORALL_SORT_BOTTOMTOTOP 2
void graphics2dsprites_doForAllSpritesOnScreen(
        int cameraId,
        int (*spriteInformation) (
        struct graphics2dsprite *sprite,
        const char *path, struct graphicstexture *tex,
        double r, double g, double b, double alpha,
        int visible, int cameraId, int textureFiltering),
        int sort, int lock) {
    if (!spriteInformation) {
        return;
    }
    if (!unittopixelsset) {
        // graphics aren't up yet, nothing to do.
        return;
    }

    int c = graphics_GetCameraCount();
    if (cameraId < 0 || cameraId >= c) {
        return;
    }
    if (lock) {
        texturemanager_lockForTextureAccess();
        mutex_lock(m);
    }

    // get position of camera in game world:
    double zoom = graphics_GetCamera2DZoom(cameraId);
    double ratio = graphics_GetCamera2DAspectRatio(cameraId);
    double centerx = graphics_GetCamera2DCenterX(cameraId);
    double centery = graphics_GetCamera2DCenterY(cameraId);
    unsigned int winw,winh;
    winw = graphics_GetCameraWidth(cameraId);
    winh = graphics_GetCameraHeight(cameraId);
    // FIXME: take care of aspect ratio here at some point
    // top-left position:
    double posx = centerx - (((double)winw)/UNIT_TO_PIXELS)*0.5*zoom;
    double posy = centery - (((double)winh)/UNIT_TO_PIXELS)*0.5*zoom;
    // dimensions:
    double width = (((double)winw)/UNIT_TO_PIXELS)*zoom;
    double height = (((double)winh)/UNIT_TO_PIXELS)*zoom;

    // allocate data pointer:
    struct doforallspritesonscreendata* data = malloc(sizeof(*data));
    if (!data) {
        if (lock) {
            mutex_release(m);
            texturemanager_releaseFromTextureAccess();
        }
        return;
    }
    memset(data, 0, sizeof(*data));
    data->spriteInformation = spriteInformation;
    data->cameraId = cameraId;

    // for top to bottom, go for on-screen/pinned first and then world
    // objects:
    if (sort == DOFORALL_SORT_TOPTOBOTTOM) {
        abortedearly = 0;
        // on-screen (pinned) sprites:
        graphics2dspriteslist_doForAllSpritesTopToBottom(
             &graphics2dsprites_doForAllSpritesCallback, data);
        // for potentially visible sprites in the game world:
        if (!abortedearly) {
            graphics2dspritestree_doForAllSpritesSortedTopToBottom(
                posx, posy, width, height,
                &graphics2dsprites_doForAllSpritesCallback, data);
        }
    } else {
        abortedearly = 0;
        // first, for all the potentially visible sprites in the game world:
        if (sort == DOFORALL_SORT_BOTTOMTOTOP) {
            graphics2dspritestree_doForAllSpritesSortedBottomToTop(
                posx, posy, width, height,
                &graphics2dsprites_doForAllSpritesCallback, data);
        } else {
            graphics2dspritestree_doForAllSprites(
                posx, posy, width, height,
                &graphics2dsprites_doForAllSpritesCallback, data);
        }

        // now do this for all on-screen (pinned) sprites:
        if (!abortedearly) {
            graphics2dspriteslist_doForAllSpritesBottomToTop(
                &graphics2dsprites_doForAllSpritesCallback, data);
        }
    }

    if (lock) {
        mutex_release(m);
        texturemanager_releaseFromTextureAccess();
    }
}

void graphics2dsprites_setTextureFiltering(struct graphics2dsprite *sprite,
        int filter) {
    if (!sprite) {
        return;
    }
    mutex_lock(m);
    sprite->textureFiltering = filter;
    mutex_release(m);
}

void graphics2dsprites_resize(struct graphics2dsprite *sprite,
        double width, double height) {
    sprite->width = width;
    sprite->height = height;
    if (sprite->pinnedToCamera < 0) {
        graphics2dspritestree_update(sprite);
    }
}

static void graphics2dsprites_removeFromList(struct graphics2dsprite *sprite) {
    // Warning: this is NOT SAFE to call when not on the list!

    spritesInListCount--;

    // for pinned sprites only:
    if (sprite->pinnedToCamera >= 0 && sprite->visible) {
        // if this is enabled for an event, remember to recalculate
        // last event sprite if necessary:
        int i = 0;
        while (i < SPRITE_EVENT_TYPE_COUNT) {
            if (sprite->enabledForEvent[i]) {
                if (lastEventSprite[i] == sprite) {
                    recalculateLastEventSprite[i] = 1;
                    lastEventSprite[i] = NULL;
                }
                eventSpriteCount[i]--;
            }
            i++;
        }
    }

    // remove sprite from pinned sprite list or tree:
    if (sprite->pinnedToCamera >= 0) {
        graphics2dspriteslist_removeFromList(sprite);
    } else {
        graphics2dspritestree_removeFromTree(sprite);
    }
}

void graphics2dsprites_destroy(struct graphics2dsprite *sprite) {
    if (!sprite) {
        return;
    }
    mutex_lock(m);

    // remove sprite from list:
    graphics2dsprites_removeFromList(sprite);

    // destroy texture manager request
    sprite->deleted = 1;
    if (sprite->request) {
        mutex_release(m);
        texturemanager_destroyRequest(sprite->request);
        mutex_lock(m);
        sprite->request = NULL;
    }
    // ... this will handle the texture aswell:
    sprite->tex = NULL;

    // free resource path:
    if (sprite->path) {
        free(sprite->path);
    } 

    // free sprite:
    poolAllocator_free(spriteAllocator, sprite);

    // done!
    mutex_release(m);
}

void graphics2dsprites_move(struct graphics2dsprite *sprite,
        double x, double y, double angle) {
    mutex_lock(m);
#ifdef SMOOTH_SPRITES
    sprite->prevx = x;
    sprite->prevy = y;
#endif
    sprite->x = x;
    sprite->y = y;
    if (sprite->pinnedToCamera < 0) {
        graphics2dspritestree_update(sprite);
    }
    sprite->angle = angle;
    mutex_release(m);
}

static void graphics2dsprites_addToList(struct graphics2dsprite *s) {
    // Warning: this is NOT SAFE to call when already on the list!

#if (!defined(NDEBUG) && defined(EXTRADEBUG))
    mutex_release(m);
    assert(graphics2dsprites_count()
    <= texturemanager_getRequestCount());
    mutex_lock(m);
#endif

    if (s->pinnedToCamera >= 0) {
        // add to screen-pinned z index sorted list
        graphics2dspriteslist_addToList(s);
    } else {
        // add to game world tree
        graphics2dspritestree_addToTree(s);
    }

    // only important if pinned:
    if (s->pinnedToCamera >= 0 && s->visible) {
        // if enabled for event and lower in sorting than lowest sprite,
        // then we'll need to recalculate the lowest event sprite:
        int i = 0;
        while (i < SPRITE_EVENT_TYPE_COUNT) {
            if (s->enabledForEvent[i]
            && lastEventSprite[i]
            && lastEventSprite[i]->zindex >= s->zindex) {
                recalculateLastEventSprite[i] = 1;
                lastEventSprite[i] = NULL;
            }
            i++;
        }
    }
    spritesInListCount++;

#if (!defined(NDEBUG) && defined(EXTRADEBUG))
    mutex_release(m);
    assert(graphics2dsprites_count()
    <= texturemanager_getRequestCount());
    mutex_lock(m);
#endif
}

static size_t graphics2dsprites_count_internal(void) {
    return 0;
}

struct graphics2dsprite *graphics2dsprites_create(
        const char *texturePath,
        double x, double y,
        double width, double height) {
    if (!texturePath) {
        return NULL;
    }

    mutex_lock(m);

    // create new sprite struct:
    struct graphics2dsprite *s = poolAllocator_alloc(spriteAllocator);
    if (!s) {
        mutex_release(m);
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
    s->zindexsetid = currentzindexsetid;
    currentzindexsetid++;
    s->parallax = 1;
    if (!s->path) {
        poolAllocator_free(spriteAllocator, s);
        mutex_release(m);
        texturemanager_releaseFromTextureAccess();
        return NULL;
    }

    mutex_release(m);
    // get a texture request:
#ifdef EXTRADEBUG
    assert(graphics2dsprites_count()
        <= texturemanager_getRequestCount());
#endif
    // coverity[missing_lock] -
    // this is fine, the texture isn't in the list yet,
    // hence noone else will fiddle with it -> no lock required.
    s->request = texturemanager_requestTexture(
        s->path, graphics2dsprites_dimensionInfoCallback,
        graphics2dsprites_textureSwitchCallback,
        graphics2dsprites_textureHandlingDoneCallback,
        s);
#ifdef EXTRADEBUG
    assert(graphics2dsprites_count()
    <= texturemanager_getRequestCount());
#endif
    mutex_lock(m);

    // add us to the list:
    graphics2dsprites_addToList(s);

    mutex_release(m);

    return s;
}

int graphics2dsprites_getZIndex(struct graphics2dsprite *sprite) {
    mutex_lock(m);
    int z = sprite->zindex;
    mutex_release(m);
    return z;
}

void graphics2dsprites_setZIndex(struct graphics2dsprite *sprite,
        int zindex) {
    mutex_lock(m);
    if (!sprite || sprite->zindex == zindex) {
        // nothing to do.
        mutex_release(m);
        return;
    }

    // update zindex:
    sprite->zindex = zindex;
    sprite->zindexsetid = currentzindexsetid;
    currentzindexsetid++;

    // if pinned to screen, we need a new list position:
    if (sprite->pinnedToCamera >= 0) {
        // remove and re-add to the list:
        graphics2dsprites_removeFromList(sprite);
        graphics2dsprites_addToList(sprite);
    }

    mutex_release(m);
}

void graphics2dsprites_setPinnedToCamera(struct graphics2dsprite *sprite,
        int cameraId) {
    if (!sprite) {
        return;
    }
    mutex_lock(m);
    // abort if nothing has changed:
    if (sprite->pinnedToCamera == cameraId) {
        mutex_release(m);
        return;
    }

    // remove from tree/list before proceeding:
    graphics2dsprites_removeFromList(sprite);
    // (we only wouldn't want to do that if the sprite
    // was unpinned and still is unpinned -> always in the tree,
    // but that case is already caught with the check above)

    // set new pinned state:
    sprite->pinnedToCamera = cameraId;

    // readd to tree/list:
    graphics2dsprites_addToList(sprite);

    // done.
    mutex_release(m);
}

int graphics2dsprites_getVisible(struct graphics2dsprite *sprite) {
    mutex_lock(m);
    int r = sprite->visible;
    mutex_release(m);
    return r;
}

void graphics2dsprites_setVisible(struct graphics2dsprite *sprite,
        int visible) {
    mutex_lock(m);
    sprite->visible = (visible != 0);
    // only relevant if pinned:
    if (sprite->pinnedToCamera >= 0) {
        if (!sprite->visible) {
            // check if we were enabled for any event:
            int i = 0;
            while (i < SPRITE_EVENT_TYPE_COUNT) {
                if (sprite->enabledForEvent[i]) {
                    eventSpriteCount[i]--;
                    // last event sprite needs to be recalculated
                    // if it was us:
                    if (lastEventSprite[i]) {
                        if (lastEventSprite[i] == sprite) {
                            lastEventSprite[i] = NULL;
                        }
                    }
                }
                i++;
            }
        } else {
            // check if we're enabled for any event:
            int i = 0;
            while (i < SPRITE_EVENT_TYPE_COUNT) {
                if (sprite->enabledForEvent[i]) {
                    eventSpriteCount[i]++;
                    // last event sprite needs to be recalculated
                    // if it has a higher zindex:
                    if (lastEventSprite[i]) {
                        if (lastEventSprite[i]->zindex >= sprite->zindex) {
                            lastEventSprite[i] = NULL;
                        }
                    }
                }
                i++;
            }
        }
    }
    mutex_release(m);
}

void graphics2dsprites_setParallaxEffect(struct graphics2dsprite *sprite,
        double value) {
    if (value <= 0) {
        value = 1;
    }
    mutex_lock(m);
    sprite->parallax = value;
    mutex_release(m);
}

void graphics2dsprite_calculateSizeOnScreen(
        const struct graphics2dsprite *sprite,
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
    struct graphicstexture *tex = sprite->tex;
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
        winw = graphics_GetCameraWidth(cameraId);
        winh = graphics_GetCameraHeight(cameraId);
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

    // if rotated and we should make a rough guess for rotation,
    // do it here (really rough guess!):
    // FIXME: eventually, do something more accurate here.
    if (compensaterotation) {
        double maxedgelength = fmax(width, height);
        width = maxedgelength * 1.5;
        height = maxedgelength * 1.5;
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

static int graphics2dsprites_reportVisibilityCallback(
        struct graphics2dsprite *sprite,
        __attribute__((unused)) const char *path,
        __attribute__((unused)) struct graphicstexture *tex,
        __attribute__((unused)) double r,
        __attribute__((unused)) double g,
        __attribute__((unused)) double b,
        __attribute__((unused)) double alpha,
        __attribute__((unused)) int visible,
        int cameraId,
        __attribute__((unused)) int textureFiltering) {
    assert(cameraId >= 0);
    if (!sprite->texWidth || !sprite->texHeight
    || sprite->loadingError || !sprite->tex) {
        return 1;
    }
    if (!sprite->visible || sprite->alpha <= 0) {
        // sprite is set to invisible;
        return 1;
    }

    // get camera settings:
    int sw = graphics_GetCameraWidth(cameraId);
    int sh = graphics_GetCameraHeight(cameraId);

    // get sprite pos on screen:
    double x, y, w, h;
    graphics2dsprite_calculateSizeOnScreen(sprite,
    cameraId, &x, &y, &w, &h, NULL, NULL, NULL, NULL, NULL, NULL, 1);

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
    return 1;
}

void graphics2dsprites_reportVisibility(void) {
    if (!unittopixelsset) { 
        // graphics aren't up yet, nothing to do.
        return;
    }
    int i = 0;
    while (i < graphics_GetCameraCount()) {
        graphics2dsprites_doForAllSpritesOnScreen(
            i,
            &graphics2dsprites_reportVisibilityCallback,
            DOFORALL_SORT_NONE, 1);
        i++;
    }
}

void graphics2dsprites_setInvisibleForEvent(struct graphics2dsprite *sprite,
        int event, int invisible) {
    mutex_lock(m);
    sprite->invisibleForEvent[event] = invisible;
    mutex_release(m);
}

void graphics2dsprites_enableForEvent(
        struct graphics2dsprite *sprite, int event, int enabled) {
    mutex_lock(m);
    if (enabled == sprite->enabledForEvent[event]) {
        // nothing changes.
        mutex_release(m);
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
            if (lastEventSprite[event] && lastEventSprite[event] == sprite) {
                // we were the last, so now it must be someone else:
                recalculate = 1;
            }
        }
    }
    if (recalculate) {
        // force recalculation of last event sprite:
        recalculateLastEventSprite[event] = 1;
    }
    mutex_release(m);
}

struct getSpriteAtScreenPosData {
    int cameraId;
    int x,y;
    int event;
    struct graphics2dsprite *result;
};
struct getSpriteAtScreenPosData getspriteatscreenposdata;
static int graphics2dsprites_getSpriteAtScreenPosCallback(
        struct graphics2dsprite *sprite,
        __attribute__((unused)) const char *path,
        __attribute__((unused)) struct graphicstexture *tex,
        __attribute__((unused)) double r,
        __attribute__((unused)) double g,
        __attribute__((unused)) double b,
        __attribute__((unused)) double alpha,
        __attribute__((unused)) int visible,
        int cameraId,
        __attribute__((unused)) int textureFiltering) {
    assert(!abortedearly);
    struct getSpriteAtScreenPosData* data =
        &getspriteatscreenposdata;
    // skip sprite if disabled for event:
    if ((!sprite->enabledForEvent[data->event] &&
    sprite->invisibleForEvent[data->event]) || !sprite->visible) {
        return 1;
    }

    // get sprite pos on screen:
    double x, y, w, h;
    graphics2dsprite_calculateSizeOnScreen(sprite,
    cameraId, &x, &y, &w, &h, NULL, NULL, NULL, NULL, NULL, NULL, 0);

    // FIXME: consider rotation here!
    // check if sprite pos is under mouse pos:
    if (data->x >= x && data->x < x + w &&
    data->y >= y && data->y < y + h) {
        // mouse on this sprite
        if (sprite->enabledForEvent[data->event]) {
            data->result = sprite;
        } else {
            data->result = NULL;
        }
        return 0;
    }

    /*if (sprite == lastEventSprite[data->event]) {
        // this was the last one we needed to check!
        data->result = NULL;
        return 0;
    }*/
    return 1;
}

struct graphics2dsprite*
graphics2dsprites_getSpriteAtScreenPos(
        int cameraId, int mx, int my, int event) {
    texturemanager_lockForTextureAccess();
    mutex_lock(m);

    // first, update lastEventSprite entries if necessary:
    graphics2dsprites_findLastEventSprites();

    // set data:
    // FIXME: doing this global isn't really nice, but works for now
    getspriteatscreenposdata.cameraId = cameraId;
    getspriteatscreenposdata.x = mx;
    getspriteatscreenposdata.y = my;
    getspriteatscreenposdata.event = event;
    getspriteatscreenposdata.result = NULL;

    // go through all sprites from top to bottom:
    graphics2dsprites_doForAllSpritesOnScreen(
        cameraId,
        &graphics2dsprites_getSpriteAtScreenPosCallback,
        DOFORALL_SORT_TOPTOBOTTOM, 0);

    struct graphics2dsprite *result = getspriteatscreenposdata.result;
    mutex_release(m);
    texturemanager_releaseFromTextureAccess();
    return result;
}

void graphics2dsprites_setUserdata(struct graphics2dsprite *sprite,
        void *data) {
    mutex_lock(m);
    sprite->userdata = data;
    mutex_release(m);
}

void *graphics2dsprites_getUserdata(struct graphics2dsprite *sprite) {
    mutex_lock(m);
    void *udata = sprite->userdata;
    mutex_release(m);
    return udata;
}

size_t graphics2dsprites_count(void) {
    mutex_lock(m);
    size_t c = graphics2dsprites_count_internal();
    mutex_release(m);
    return c;
}

void graphics2dsprites_lockListOrTreeAccess(void) {
    mutex_lock(m);
}

void graphics2dsprites_releaseListOrTreeAccess(void) {
    mutex_release(m);
}

#endif


