
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
#include <string.h>

#include "avl-tree/avl-tree.h"

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

AVLTreeNode *avl_tree_find_next(AVLTreeNode *node, int forwards) {
    assertverifynode(node);
    // see if we have a parent:
    AVLTreeNode *dontreturnto = NULL;
    while (1) {
        assertverifynode(node);
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
            assertverifynode(child);
            // we want to go here! but the outermost child of this
            return findoutermostnode(child, forwards);
        }
        // get next sibling (through parent) if possible:
        AVLTreeNode* parent = avl_tree_node_parent(node);
        if (parent) {
            assertverifynode(parent);
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

AVLTreeNode *avl_tree_find_first_node(AVLTree *tree) {
    AVLTreeNode *node = avl_tree_root_node(tree);
    if (!node) {
        return NULL;
    }
    while (1) {
        AVLTreeNode* newnode = avl_tree_node_child(node, AVL_TREE_NODE_LEFT);
        if (!newnode) {
            return node;
        }
        node = newnode;
    }
}

AVLTreeNode *avl_tree_find_last_node(AVLTree *tree) {
    AVLTreeNode *node = avl_tree_root_node(tree);
    if (!node) {
        return NULL;
    }
    while (1) {
        AVLTreeNode* newnode = avl_tree_node_child(node, AVL_TREE_NODE_RIGHT);
        if (!newnode) {
            return node;
        }
        node = newnode;
    }
}

