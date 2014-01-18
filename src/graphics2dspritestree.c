
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

#include <stdint.h>
#include <string.h>

#include "graphics2dspritestree.h"
#include "graphics2dspritestruct.h"
#include "graphics.h"

static int getspritedimensions(struct graphics2dsprite* sprite,
        double* w, double* h) {
#ifdef USE_GRAPHICS
    if (!unittopixelsset) {
        return 0;
    }
    double spritew = sprite->width;
    double spriteh = sprite->height;
    if (sprite->width == 0) {
        spritew = sprite->texWidth;
    }
    if (sprite->height == 0) {
        spriteh = sprite->texHeight;
    }
    spritew /= UNIT_TO_PIXELS;
    spriteh /= UNIT_TO_PIXELS;
    *w = spritew;
    *h = spriteh;
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
static struct faketreeentry* faketreelist = NULL;


void graphics2dspritestree_addToTree(struct graphics2dsprite* sprite) {
    struct faketreeentry* t = malloc(sizeof(*t));
    if (!t) {
        return;
    }
    memset(t, 0, sizeof(*t));
    t->sprite = sprite;
    getspritetopleft(t->sprite, &t->addedx, &t->addedy);
    getspritedimensions(t->sprite, &t->addedw, &t->addedh);
    if (faketreelist) {
        t->next = faketreelist;
    }
    faketreelist = t;
}

void graphics2dspritestree_doForAllSprites(
        double windowX, double windowY, double windowW,
        double windowH,
        int (*callback)(struct graphics2dsprite *sprite,
        void *userdata),
        void *userdata) {
    struct faketreeentry* t = faketreelist;
    while (t) {
        int onscreen = 0;
        if (t->addedx + t->addedw > windowX &&
                t->addedx < windowX + windowW &&
                t->addedy + t->addedh > windowH &&
                t->addedy < windowY + windowH) {
            onscreen = 1;
        }
        if (onscreen) {
            if (!callback(t->sprite, userdata)) {
                return;
            }
        }
        t = t->next;
    }
}

void graphics2dspritestree_removeFromTree(struct graphics2dsprite*
            sprite) {
    struct faketreeentry* prev = NULL;
    struct faketreeentry* t = faketreelist;
    while (t) {
        if (t->sprite == sprite) {
            if (prev) {
                prev->next = t->next;
            } else {
                faketreelist = t->next;
            }
            free(t);
            return;
        }
        prev = t;
        t = t->next;
    }
}

void graphics2dspritestree_update(struct graphics2dsprite *sprite) {
    struct faketreeentry* t = faketreelist;
    while (t) {
        if (t->sprite == sprite) {
            getspritetopleft(t->sprite, &t->addedx, &t->addedy);
            getspritedimensions(t->sprite, &t->addedw, &t->addedh);
            return;
        }
        t = t->next;
    }
}

#else

#endif

struct spritesortentry {
    struct graphics2dsprite* sprite;
    int zindex;
    uint64_t zindexsetid;
};
static size_t sortlistsize = 0;
static struct spritesortentry* sortlist = NULL;
static size_t sortlistfill = 0;

static int graphics2dspritestree_spriteSortCallback(
        struct graphics2dsprite* sprite, __attribute__((unused))
        void* userdata) {
    if (sortlistfill >= sortlistsize) {
        // expand list:
        int newsize = sortlistfill + 200;
        struct spritesortentry* newsortlist = realloc(sortlist,
            sizeof(struct spritesortentry) * newsize);
        if (!sortlist) {
            return 0;
        }
        sortlist = newsortlist;
        sortlistsize = newsize;
    }
    struct spritesortentry* s = &sortlist[sortlistfill];
    s->sprite = sprite;
    s->zindex = sprite->zindex;
    s->zindexsetid = sprite->zindexsetid;
    sortlistfill++;
    return 1;
}

static void graphics2dspritestree_obtainUnsortedList(
        double windowX, double windowY, double windowW,
        double windowH) {
    sortlistfill = 0;
    graphics2dspritestree_doForAllSprites(
        windowX, windowY, windowW, windowH,
        &graphics2dspritestree_spriteSortCallback, NULL);
}


static void graphics2dspritestree_reportSortedList(
        int (*callback)(struct graphics2dsprite* sprite,
        void* userdata), void* userdata) {
    size_t i = 0;
    while (i < sortlistfill) {
        callback(sortlist[i].sprite, userdata);
        i++;
    }
}

void graphics2dspritestree_doForAllSpritesSortedBottomToTop(
        double windowX, double windowY, double windowW,
        double windowH,
        int (*callback)(struct graphics2dsprite *sprite,
        void *userdata),
        void *userdata) {
    graphics2dspritestree_obtainUnsortedList(windowX,
        windowY, windowW, windowH);
    graphics2dspritestree_reportSortedList(callback, userdata);
}

void graphics2dspritestree_doForAllSpritesSortedTopToBottom(
        double windowX, double windowY, double windowW,
        double windowH,
        int (*callback)(struct graphics2dsprite *sprite,
        void *userdata),
        void *userdata) {
    graphics2dspritestree_obtainUnsortedList(windowX, windowY,
        windowW, windowH);
    graphics2dspritestree_reportSortedList(callback, userdata);
}


