
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

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "graphics2dspritestree.h"
#include "graphics2dspritestruct.h"
#include "graphics.h"

#ifdef USE_GRAPHICS

static int getspritedimensions(struct graphics2dsprite* sprite,
        double* w, double* h) {
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
#ifndef NDEBUG
    struct faketreeentry* titerate = faketreelist;
    while (titerate) {
        assert(titerate->sprite != sprite);
        assert(titerate->sprite->zindexsetid != sprite->zindexsetid);
        titerate = titerate->next;
    }
#endif
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
                t->addedy + t->addedh > windowY &&
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
        if (!newsortlist) {
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
        void* userdata), void* userdata, int bottomup) {
    int i = 0;
    if (!bottomup) {
        i = sortlistfill - 1;
    }
    while ((bottomup && i < (int)sortlistfill) ||
            (!bottomup && i >= 0)) {
        // report current list item:
        if (!callback(sortlist[i].sprite, userdata)) {
            return;
        }
        // ensure the sorting was done correctly:
#ifndef NDEBUG
        if (i > 0) { 
            assert(sortlist[i].zindex >= sortlist[i-1].zindex);
            assert(sortlist[i].zindex != sortlist[i-1].zindex ||
                sortlist[i].zindexsetid > sortlist[i-1].zindexsetid);
        }
#endif
        // advance to next item:
        if (bottomup) {
            i++;
        } else {
            i--;
        }
    }
}

static int spriteisbefore(int a, int b) {
    if (sortlist[a].zindex < sortlist[b].zindex) {
        return 1;
    } else if (sortlist[a].zindex > sortlist[b].zindex) {
        return 0;
    } else {
        assert(sortlist[a].zindex == sortlist[b].zindex);
        return (sortlist[a].zindexsetid < sortlist[b].zindexsetid);
    }
}

static void swap(int a, int b) {
    assert(0 <= a);
    assert(a < sortlistfill);
    assert(0 <= b);
    assert(b < sortlistfill);
    assert(a != b);
    struct spritesortentry tempentry;
    memcpy(&tempentry, &sortlist[a], sizeof(tempentry));
    memcpy(&sortlist[a], &sortlist[b], sizeof(sortlist[a]));
    memcpy(&sortlist[b], &tempentry, sizeof(tempentry));
}

static void graphics2dspritestree_printSortedList(int currentItem, int pivot) {
    int i = 0;
    while (i < sortlistfill) {
        char currentItemC = ' ';
        char pivotC = ' ';
        if (currentItem == i) {
            currentItemC = 'C';
        }
        if (pivot == i) {
            pivotC == 'P';
        }
        printf("[%d] zindex: %d, zindexsetid: %llu [%c][%c]\n", i,
            sortlist[i].zindex, sortlist[i].zindexsetid,
            currentItemC, pivotC);
        i++;
    }
}

static void graphics2dspritestree_doSortFunc(int from, int to) {
    assert(from < to);
    if (from == to - 1) {
        return;
    }
    if (from == to - 2) {
        if (!spriteisbefore(from, to - 1)) {
            swap(from, to - 1);
        }
        return;
    }
    int pivot = from+(to-from)/2;
    int i = from;
    while (i < to) {
        assert(from <= pivot);
        assert(pivot <= to);
        if (i < pivot) {
            // this item needs to be smaller
            if (!spriteisbefore(i, pivot)) {
                assert(spriteisbefore(pivot, i));
                assert(i >= from);
                assert(i < pivot);
                // it isn't. fix!
                if (i == pivot - 1) {
                    // swap with pivot:
                    swap(i, pivot);
                    assert(spriteisbefore(i, pivot));
                    pivot--;
                } else {
                    // swap the item before with the pivot,
                    // then that item with us.
                    swap(pivot - 1, pivot);
                    swap(i, pivot);
                    pivot--;
                    assert(spriteisbefore(pivot, pivot + 1));
                    continue;  // do not advance i
                }
                assert(pivot >= from);
                assert(from <= pivot);
            }
        } else {
            if (i == pivot) {
                i++;
                continue;
            }
            // this item needs to be bigger
            if (spriteisbefore(i, pivot)) {
                assert(!spriteisbefore(pivot, i));
                // it isn't. fix!
                if (i == pivot + 1) {
                    // swap with pivot:
                    swap(i, pivot);
                    assert(!spriteisbefore(i, pivot));
                    pivot++;
                } else {
                    // swap the item after with the pivot,
                    // then that item with us
                    swap(pivot + 1, pivot);
                    swap(i, pivot);
                    assert(spriteisbefore(pivot, pivot+1));
                    pivot++;
                    continue;  // do not advance i
                }
                assert(pivot <= to);
                assert(from <= pivot);
            }
        }
        i++;
    }
    assert(from < to);
    assert(from <= pivot);
    assert(pivot <= to);
#ifndef NDEBUG
    // ensure sorting is correct:
    i = from;
    while (i < to) {
        if (i < pivot) {
            if (!spriteisbefore(i, pivot)) {
                printf("faulty element: %d\n", i);
                fflush(stdout);
                assert(spriteisbefore(i, pivot));
            }
        } else if (i > pivot) {
            if (!spriteisbefore(pivot, i)) {
                printf("faulty element: %d\n", i);
                fflush(stdout);
                assert(spriteisbefore(pivot, i));
            }
        }
        i++;
    }
#endif
    if (pivot > from) {
        graphics2dspritestree_doSortFunc(from, pivot);
    }
    if (pivot + 1 < to) {
        graphics2dspritestree_doSortFunc(pivot + 1, to);
    }
}

static void graphics2dspritestree_doSort(void) {
    if (sortlistfill <= 1) {
        return;
    }
    int from = 0;
    int to = sortlistfill;
    graphics2dspritestree_doSortFunc(from, to);
}

void graphics2dspritestree_doForAllSpritesSortedBottomToTop(
        double windowX, double windowY, double windowW,
        double windowH,
        int (*callback)(struct graphics2dsprite *sprite,
        void *userdata),
        void *userdata) {
    graphics2dspritestree_obtainUnsortedList(windowX,
        windowY, windowW, windowH);
    graphics2dspritestree_doSort();
    graphics2dspritestree_reportSortedList(callback, userdata, 1);
}

void graphics2dspritestree_doForAllSpritesSortedTopToBottom(
        double windowX, double windowY, double windowW,
        double windowH,
        int (*callback)(struct graphics2dsprite *sprite,
        void *userdata),
        void *userdata) {
    graphics2dspritestree_obtainUnsortedList(windowX, windowY,
        windowW, windowH);
    graphics2dspritestree_doSort();
    graphics2dspritestree_reportSortedList(callback, userdata, 0);
}

#endif

