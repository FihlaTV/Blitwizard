
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

#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "graphics2dspritestruct.h"
#include "graphics2dspriteslist.h"
#include "threading.h"
#include "avl-tree/avl-tree.h"
#include "avl-tree-helpers.h"

#ifdef USE_GRAPHICS

AVLTree *spritetree = NULL;
struct spriteorderinfo {
    int zindex;
    uint64_t zindexsetid;
};

static AVLTreeNode* cachedfirst = NULL;
static AVLTreeNode* cachedlast = NULL;

int compareSprites(AVLTreeValue spriteAOrder, AVLTreeValue spriteBOrder) {
    struct spriteorderinfo *sprite_a, *sprite_b;
    sprite_a = spriteAOrder;
    sprite_b = spriteBOrder;
    if (sprite_a->zindex > sprite_b->zindex) {
        return -1;
    } else if (sprite_a->zindex < sprite_b->zindex) {
        return 1;
    } else if (sprite_a->zindexsetid > sprite_b->zindexsetid) {
        return -1;
    } else if (sprite_a->zindexsetid < sprite_b->zindexsetid) {
        return 1;
    }
    return 0;
}

static __attribute__((constructor)) void createSpriteList(void) {
    spritetree = avl_tree_new(&compareSprites);
}

void graphics2dspriteslist_addToList(struct graphics2dsprite
        *sprite) {
    struct spriteorderinfo* order = malloc(sizeof(*order));
    if (!order) {
        return;
    }
    order->zindex = sprite->zindex;
    order->zindexsetid = sprite->zindexsetid;
    if (cachedfirst) {
        struct graphics2dsprite *sprfirst = avl_tree_node_value(cachedfirst);
        if (sprfirst->zindex <= order->zindex) {
            // the first sprite is likely to change.
            cachedfirst = NULL;
        }
    }
    if (cachedlast) {
        struct graphics2dsprite *sprlast = avl_tree_node_value(cachedlast);
        if (sprlast->zindex >= order->zindex) {
            // the first sprite is likely to change.
            cachedlast = NULL;
        }
    }
    sprite->avlptr = avl_tree_insert(spritetree,
        order, sprite);
}

static AVLTreeNode *findfirstnode(void) {
    if (cachedfirst) {
        return cachedfirst;
    }
    cachedfirst = avl_tree_find_first_node(spritetree);
    return cachedfirst;
}

static AVLTreeNode *findlastnode(void) {
    if (cachedlast) {
        return cachedlast;
    }
    cachedlast = avl_tree_find_last_node(spritetree);
    return cachedlast;
}


static void graphics2dspriteslist_doForAllSprites_Internal(
        int (*callback)(struct graphics2dsprite *sprite,
        void *userdata), void *userdata, int fromtop) {
    AVLTreeNode *node;
    if (fromtop) {
        node = findfirstnode();
    } else {
        node = findlastnode();
    }
    while (node) {
        if (!callback(avl_tree_node_value(node), userdata)) {
            return;
        }
        node = avl_tree_find_next(node, fromtop);
    }
    return;
}

void graphics2dspriteslist_doForAllSpritesBottomToTop(
        int (*callback)(struct graphics2dsprite *sprite,
        void *userdata),
        void *userdata) {
    graphics2dspriteslist_doForAllSprites_Internal(
        callback, userdata, 0);
}

void graphics2dspriteslist_doForAllSpritesTopToBottom(
        int (*callback)(struct graphics2dsprite *sprite,
        void *userdata),
        void *userdata) {
    graphics2dspriteslist_doForAllSprites_Internal(
        callback, userdata, 1);
}

void graphics2dspriteslist_removeFromList(struct graphics2dsprite
        *sprite) {
    if (cachedfirst == sprite->avlptr) {cachedfirst = NULL;}
    if (cachedlast == sprite->avlptr) {cachedlast = NULL;}
    avl_tree_remove_node(spritetree, sprite->avlptr);
}

#endif  // USE_GRAPHICS

