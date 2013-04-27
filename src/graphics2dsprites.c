
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
    size_t texwidth, texheight;

    // texture info:
    struct graphicstexture* tex;
    struct texturerequesthandle* request;

    // position, size info:
    double x, y, width, height, angle;
    // width, height = 0 means height should match texture geometry

    // alpha, color and visibility:
    double alpha;
    double r, g, b, a;
    int visible;

    // pointers for global linear sprite list:
    struct graphics2dsprite* next,* prev;
};
// global linear sprite list:
static struct graphics2dsprite* spritelist = NULL;

// initialise threading mutex:
__attribute__((constructor)) static void graphics2dsprites_Init(void) {
    m = mutex_Create();
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

    s->texwidth = width;
    s->texheight = height;
    if (s->texwidth == 0 && s->texheight == 0) {
        // texture failed to load.
        s->loadingError = 1;
    }
    mutex_Release(m);
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
int graphics2dsprites_GetGeometry(struct graphics2dsprite* sprite,
size_t* width, size_t* height) {
    if (!sprite) {
        return 0;
    }
    mutex_Lock(m);
    if ((sprite->texwidth && sprite->texheight) || sprite->loadingError) {
        *width = sprite->texwidth;
        *height = sprite->texheight;
        mutex_Release(m);
        return 1;
    }
    mutex_Release(m);
    return 0;
}

// Check if actual texture is available:
int graphics2dsprites_IsTextureAvailable(struct graphics2dsprite* sprite) {
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

void graphics2dsprites_DoForAllSprites(
void (*spriteInformation) (const char* path, struct graphicstexture* tex,
double x, double y, double width, double height,
double angle, double alpha, double r, double g, double b, int visible)) {
    if (!spriteInformation) {
        return;
    }

    mutex_Lock(m);

    // walk through linear sprite list:
    struct graphics2dsprite* s = spritelist;
    while (s) {
        // call sprite information callback:
        spriteInformation(s->path, s->tex, s->x, s->y, s->width, s->height,
        s->angle, s->alpha, s->r, s->g, s->b, s->visible);

        s = s->next;
    }

    mutex_Release(m);
}

void graphics2dsprites_Destroy(struct graphics2dsprite* sprite) {
    if (!sprite) {
        return;
    }
    mutex_Lock(m);

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
    if (sprite->prev) {
        sprite->prev->next = sprite->next;
    } else {
        spritelist = sprite->next;
    }
    if (sprite->next) {
        sprite->next->prev = sprite->prev;
    }
    
    // free sprite:
    free(sprite);

    // done!
    mutex_Release(m);
}

struct graphics2dsprite* graphics2dsprites_Create(
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
    s->width = width;
    s->height = height;
    s->path = strdup(texturePath);
    if (!s->path) {
        free(s);
        mutex_Release(m);
        return NULL;
    }
    s->visible = 1;

    // add sprite to list:
    s->next = spritelist;
    if (s->next) {
        s->next->prev = s;
    }
    spritelist = s;

    // get a texture request:
    s->request = texturemanager_RequestTexture(
    s->path, graphics2dsprites_dimensionInfoCallback,
    graphics2dsprites_textureSwitchCallback,
    s);

    mutex_Release(m);

    return s;
}



#endif


