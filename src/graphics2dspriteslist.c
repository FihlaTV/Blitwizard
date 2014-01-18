
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

#include <stdio.h>
#include <stdint.h>

#include "graphics2dspritestruct.h"
#include "graphics2dspriteslist.h"
#include "threading.h"
#include "avl-tree/avl-tree.h"

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
    AVLTreeNode *node = avl_tree_root_node(spritetree);
    while (1) {
        AVLTreeNode* newnode = avl_tree_node_child(node, AVL_TREE_NODE_LEFT);
        if (!newnode) {
            cachedfirst = node;
            return node;
        }
        node = newnode;
    }
}

static AVLTreeNode *findlastnode(void) {
    if (cachedlast) {
        return cachedlast;
    }
    AVLTreeNode *node = avl_tree_root_node(spritetree);
    while (1) {
        AVLTreeNode* newnode = avl_tree_node_child(node, AVL_TREE_NODE_RIGHT);
        if (!newnode) {
            cachedlast = node;
            return node;
        }
        node = newnode;
    }
}

static AVLTreeNode *findoutermostnode(AVLTreeNode *node, int forwards) {
    while (1) {
        AVLTreeNode *child;
        if (forwards) {
            child = avl_tree_node_child(node,
                AVL_TREE_NODE_LEFT);
        } else {
            child = avl_tree_node_child(node,
                AVL_TREE_NODE_RIGHT);
        }
        if (child) {
            node = child;
        } else {
            break;
        }
    }
    return node;
}

static AVLTreeNode *findnext(AVLTreeNode *node, int forwards) {
    // see if we have a parent:
    AVLTreeNode *dontreturnto = NULL;
    while (1) {
        // get next child in order if present:
        AVLTreeNode* child;
        if (forwards) {
            child = avl_tree_node_child(
                node, AVL_TREE_NODE_RIGHT);
        } else {
            child = avl_tree_node_child(
                node, AVL_TREE_NODE_LEFT);
        }
        if (child && child != dontreturnto) {
            // we want to go here! but the outermost child of this
            return findoutermostnode(child, forwards);
        }
        // get next sibling (through parent) if possible:
        AVLTreeNode* parent = avl_tree_node_parent(parent);
        if (parent) {
            // the parent may atually be our next node:
            if (forwards) {
                if (node == avl_tree_node_child(parent,
                        AVL_TREE_NODE_LEFT)) {
                    // parent is our forward/right node!
                    return parent;
                }
            } else {
                if (node == avl_tree_node_child(parent,
                        AVL_TREE_NODE_RIGHT)) {
                    // parent is our backwards/left node!
                    return parent;
                }
            }
            // look at this as node now, but make sure
            // that we don't return back to this node.
            dontreturnto = node;
            node = parent;
        } else {
            // no parent either? we're done!
            return NULL;
        }
    }
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
        callback(avl_tree_node_value(node), userdata);
        node = findnext(node, fromtop);
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


