

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

#ifndef BLITWIZARD_GRAPHICS2DSPRITESTRUCT_H_
#define BLITWIZARD_GRAPHICS2DSPRITESTRUCT_H_

#include "graphics2dsprites.h"

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
    int textureHandlingDone;

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
    uint64_t zindexsetid;

    // used by graphis2dspriteslist.c:
    void* avlptr;

    // enabled for sprite events:
    int enabledForEvent[SPRITE_EVENT_TYPE_COUNT];

    // untouchable/transparent for sprite event:
    int invisibleForEvent[SPRITE_EVENT_TYPE_COUNT];

    // pointers for global linear sprite list:
    struct graphics2dsprite* next,* prev;

    // custom userdata:
    void* userdata;
};


#endif  // BLITWIZARD_GRAPHICS2DSPRITESTRUCT_H_

