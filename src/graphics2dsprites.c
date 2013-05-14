
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

    // pointers for global linear sprite list:
    struct graphics2dsprite* next,* prev;
};
// global linear sprite list:
static struct graphics2dsprite* spritelist = NULL;
static struct graphics2dsprite* spritelistEnd = NULL;

// initialise threading mutex:
__attribute__((constructor)) static void graphics2dsprites_init(void) {
    m = mutex_Create();
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

    // destroy texture manager request
    sprite->deleted = 1;
    if (sprite->request) {
        mutex_Release(m);
        texturemanager_DestroyRequest(sprite->request);
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
    s->request = texturemanager_RequestTexture(
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
    mutex_Release(m);
}

void graphics2dsprites_ReportVisibility(void) {
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
            if (i != sprite->pinnedToCamera && sprite->pinnedToCamera >= 0) {
                // not visible on this camera.
                i++;
                continue;
            }

            // get camera settings:
            double w = graphics_GetCameraWidth(i);
            double h = graphics_GetCameraHeight(i);
            double zoom = graphics_GetCamera2DZoom(i);
            double ratio = graphics_GetCamera2DAspectRatio(i);
            double centerx = graphics_GetCamera2DCenterX(i);
            double centery = graphics_GetCamera2DCenterY(i);

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
                width = sprite->texWidth;
                height = sprite->texHeight;
                if (sprite->clippingWidth > 0) {
                    width = sprite->clippingWidth;
                    height = sprite->clippingHeight;
                }
            } else {
                width *= UNIT_TO_PIXELS;
                height *= UNIT_TO_PIXELS;
            }

            // now check if rectangle is on screen:
            if (((x >= 0 && x < w) || (x + width >= 0 && x + width < w))
            && ((y >= 0 && y < h) || (y + height >= 0 && y + height < h))) {
                // it is.
                texturemanager_UsingRequest(sprite->request,
                USING_AT_VISIBILITY_DETAIL);
                // printf("on screen: %s\n", sprite->path);
            } else {
                // printf("not on screen: %s\n", sprite->path);
            }

            i++;
        }

        sprite = sprite->next;
    }
    mutex_Release(m); 
}

#endif


