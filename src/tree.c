/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * Modified from https://github.com/torvalds/linux/blob/master/lib/RbTreeBase.c
 *
 * Red Black Trees
 * (C) 1999  Andrea Arcangeli <andrea@suse.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Red Black Trees
 * (C) 1999  Andrea Arcangeli <andrea@suse.de>
 *
 *
 * linux/include/linux/RbTreeBase.h
 * To use rbtrees you'll have to implement your own insert and search cores.
 * This will avoid us to use callbacks and to drop drammatically performances.
 * I know it's not the cleaner way,  but in C (not in C++) to get
 * performances and genericity...
 * See Documentation/core-api/RbTreeBase.rst for documentation and samples.
 */

#include "tree.h"

typedef void (*RbTreeNodeAugmentCallback)(RbTreeNode *old, RbTreeNode *new);
typedef struct {
    RbTreeNodeAugmentCallback copy;
    RbTreeNodeAugmentCallback prop;
    RbTreeNodeAugmentCallback rot;
} RbTreeNodeAugmentCallbacks;

enum {
    rbtRed  = 0u, rbtBlack = 1u
};

attr(always_inline)
static void RbTree_set_parent(RbTreeNode *node, RbTreeNode *parent)
{
    node->parent = (node->parent & 0x3) | (unsigned long) parent;
}

attr(always_inline)
static RbTreeNode *RbTree_parent(RbTreeNode *node)
{
    return (RbTreeNode *) (node->parent & ~0x1);
}

attr(always_inline)
static void RbTree_set_color_(RbTreeNode *node, int color)
{
    node->parent = (node->parent & ~0x1) | (color & 0x1);
}

attr(always_inline)
static void RbTree_set_parent_color_(RbTreeNode *node, RbTreeNode *parent, int color)
{
    node->parent = (uptr)(parent) | (color & 0x1);
}

#define RbTree_clear_root(node) RbTree_set_parent((node), (node))
#define RbTree_color(node) ((node)->parent & 0x1)
#define RbTree_is(node, COLOR) (RbTree_color(node) == rbt##COLOR)
#define RbTree_set_color(node, COLOR) RbTree_set_color_((node), rbt##COLOR)
#define RbTree_set_parent_color(node, P, COLOR) RbTree_set_parent_color_((node), (P), rbt##COLOR)
#define RbTree_node_is_empty(node) ((node)->parent == (uptr)(node))
#define RbTree_node_clear(node) (node)->parent = (uptr)(node)

attr(always_inline)
static void RbTree_change_child(RbTreeNode **root, RbTreeNode *old, RbTreeNode *new, RbTreeNode *parent)
{
    if (parent) {
        if (parent->left == old)
            parent->left = new;
        else
            parent->right = new;
    }
    else
        *root = new;
}

attr(always_inline)
static void RbTree_link_node(RbTreeNode **link, RbTreeNode *node, RbTreeNode *parent)
{
    node->parent = (uptr) parent;
    node->left = node->right = NULL;
    *link = node;
}

attr(always_inline)
static void RbTree_rotate_set_parents(RbTreeNode **root, RbTreeNode *old, RbTreeNode *new, int color)
{
    RbTreeNode *parent = RbTree_parent(old);
    new->parent = old->parent;
    RbTree_set_parent_color_(old, new, color);
    RbTree_change_child(root, old, new, parent);
}

bool RbTree_dump_level(RbTreeNode* node, int level, void(*DumpValue)(const void*, char))
{
    if (node == NULL) {
        return false;
    }

    if (level == 1) {
        DumpValue(node->data, RbTree_is(node, Red)? 'R' : 'B');
        return true;
    }

    bool left = RbTree_dump_level(node->left, level - 1, DumpValue);
    bool right = RbTree_dump_level(node->right, level - 1, DumpValue);

    return left || right;
}

attr(always_inline)
static void RbTree_insert__(RbTreeNode **root, RbTreeNode *node, RbTreeNodeAugmentCallback augmentRot)
{
    RbTreeNode *gp, *tmp;
    RbTreeNode *parent = RbTree_parent(node);

    while (true) {
        /*
         * Loop invariant: node is red.
         */
        if (parent == NULL) {
            /*
             * The inserted node is root. Either this is the
             * first node, or we recursed at Case 1 below and
             * are no longer violating 4).
             */
            RbTree_set_parent_color(node, NULL, Black);
            break;
        }

        /*
         * If there is a black parent, we are done.
         * Otherwise, take some corrective action as,
         * per 4), we don't want a red root or two
         * consecutive red nodes.
         */
        if(RbTree_is(parent, Black))
            break;

        gp = RbTree_parent(parent);

        tmp = gp->right;
        if (parent != tmp) {	/* parent == gp->left */
            if (tmp != NULL && RbTree_is(tmp, Red)) {
                /*
                 * Case 1 - node's uncle is red (color flips).
                 *
                 *       G            g
                 *      / \          / \
                 *     p   u  -->   P   U
                 *    /            /
                 *   n            n
                 *
                 * However, since g's parent might be red, and
                 * 4) does not allow this, we need to recurse
                 * at g.
                 */
                RbTree_set_parent_color(tmp, gp, Black);
                RbTree_set_parent_color(parent, gp, Black);
                node = gp;
                parent = RbTree_parent(node);
                RbTree_set_parent_color(node, parent, Red);
                continue;
            }

            tmp = parent->right;
            if (node == tmp) {
                /*
                 * Case 2 - node's uncle is black and node is
                 * the parent's right child (left rotate at parent).
                 *
                 *      G             G
                 *     / \           / \
                 *    p   U  -->    n   U
                 *     \           /
                 *      n         p
                 *
                 * This still leaves us in violation of 4), the
                 * continuation into Case 3 will fix that.
                 */
                tmp = node->left;
                parent->right = tmp;
                node->left = parent;
                if (tmp)
                    RbTree_set_parent_color(tmp, parent, Black);
                RbTree_set_parent_color(parent, node, Black);
                augmentRot(parent, node);
                parent = node;
                tmp = node->right;
            }

            /*
             * Case 3 - node's uncle is black and node is
             * the parent's left child (right rotate at gp).
             *
             *        G           P
             *       / \         / \
             *      p   U  -->  n   g
             *     /                 \
             *    n                   U
             */
            gp->left = tmp; /* == parent->right */
            parent->right = gp;
            if (tmp)
                RbTree_set_parent_color(tmp, gp, Black);
            RbTree_rotate_set_parents(root, gp, parent, rbtRed);
            augmentRot(gp, parent);
            break;
        } else {
            tmp = gp->left;
            if (tmp && RbTree_is(tmp, Red)) {
                /* Case 1 - color flips */
                RbTree_set_parent_color(tmp, gp, Black);
                RbTree_set_parent_color(parent, gp, Black);
                node = gp;
                parent = RbTree_parent(node);
                RbTree_set_parent_color(node, parent, Red);
                continue;
            }

            tmp = parent->left;
            if (node == tmp) {
                /* Case 2 - right rotate at parent */
                tmp = node->right;
                parent->left = tmp;
                node->right = parent;
                if (tmp)
                    RbTree_set_parent_color(tmp, parent, Black);
                RbTree_set_parent_color(parent, node, Red);
                augmentRot(parent, node);
                parent = node;
                tmp = node->left;
            }

            /* Case 3 - left rotate at gp */
            gp->right = tmp; /* == parent->left */
            parent->left = gp;
            if (tmp)
                RbTree_set_parent_color(tmp, gp, Black);
            RbTree_rotate_set_parents(root, gp, parent, rbtRed);
            augmentRot(gp, parent);
            break;
        }
    }
}

attr(always_inline)
static void RbTree_erase_color__(RbTreeNode **root, RbTreeNode *parent, RbTreeNodeAugmentCallback augmentRotate)
{
    RbTreeNode *node = NULL, *sibling, *tmp1, *tmp2;

    while (true) {
        /*
         * Loop invariants:
         * - node is black (or NULL on first iteration)
         * - node is not the root (parent is not NULL)
         * - All leaf paths going through parent and node have a
         *   black node count that is 1 lower than other leaf paths.
         */
        sibling = parent->right;
        if (node != sibling) {	/* node == parent->left */
            if (RbTree_is(sibling, Red)) {
                /*
                 * Case 1 - left rotate at parent
                 *
                 *     P               S
                 *    / \             / \
                 *   N   s    -->    p   Sr
                 *      / \         / \
                 *     Sl  Sr      N   Sl
                 */
                tmp1 = sibling->left;
                parent->right = tmp1;
                sibling->left = parent;
                RbTree_set_parent_color(tmp1, parent, Black);
                RbTree_rotate_set_parents(root, parent, sibling, rbtRed);
                augmentRotate(parent, sibling);
                sibling = tmp1;
            }
            tmp1 = sibling->right;
            if (!tmp1 || RbTree_is(tmp1, Black)) {
                tmp2 = sibling->left;
                if (!tmp2 || RbTree_is(tmp2, Black)) {
                    /*
                     * Case 2 - sibling color flip
                     * (p could be either color here)
                     *
                     *    (p)           (p)
                     *    / \           / \
                     *   N   S    -->  N   s
                     *      / \           / \
                     *     Sl  Sr        Sl  Sr
                     *
                     * This leaves us violating 5) which
                     * can be fixed by flipping p to black
                     * if it was red, or by recursing at p.
                     * p is red when coming from Case 1.
                     */
                    RbTree_set_parent_color(sibling, parent,Red);
                    if (RbTree_is(parent, Red))
                        RbTree_set_color(parent, Black);
                    else {
                        node = parent;
                        parent = RbTree_parent(node);
                        if (parent != NULL)
                            continue;
                    }
                    break;
                }
                /*
                 * Case 3 - right rotate at sibling
                 * (p could be either color here)
                 *
                 *   (p)           (p)
                 *   / \           / \
                 *  N   S    -->  N   sl
                 *     / \             \
                 *    sl  Sr            S
                 *                       \
                 *                        Sr
                 *
                 * Note: p might be red, and then both
                 * p and sl are red after rotation(which
                 * breaks property 4). This is fixed in
                 * Case 4 (in RbTree_rotate_set_parents()
                 *         which set sl the color of p
                 *         and set p RB_BLACK)
                 *
                 *   (p)            (sl)
                 *   / \            /  \
                 *  N   sl   -->   P    S
                 *       \        /      \
                 *        S      N        Sr
                 *         \
                 *          Sr
                 */
                tmp1 = tmp2->right;
                sibling->left = tmp1;
                tmp2->right = sibling;
                parent->right = tmp2;
                if (tmp1)
                    RbTree_set_parent_color(tmp1, sibling, Black);
                augmentRotate(sibling, tmp2);
                tmp1 = sibling;
                sibling = tmp2;
            }
            /*
             * Case 4 - left rotate at parent + color flips
             * (p and sl could be either color here.
             *  After rotation, p becomes black, s acquires
             *  p's color, and sl keeps its color)
             *
             *      (p)             (s)
             *      / \             / \
             *     N   S     -->   P   Sr
             *        / \         / \
             *      (sl) sr      N  (sl)
             */
            tmp2 = sibling->left;
            parent->right = tmp2;
            sibling->left = parent;
            RbTree_set_parent_color(tmp1, sibling, Black);
            if (tmp2)
                RbTree_set_parent(tmp2, parent);
            RbTree_rotate_set_parents(root, parent, sibling, rbtBlack);
            augmentRotate(parent, sibling);
            break;
        } else {
            sibling = parent->left;
            if (RbTree_is(sibling, Red)) {
                /* Case 1 - right rotate at parent */
                tmp1 = sibling->right;
                parent->left = tmp1;
                sibling->right = parent;
                RbTree_set_parent_color(tmp1, parent, Black);
                RbTree_rotate_set_parents(root, parent, sibling, rbtRed);
                augmentRotate(parent, sibling);
                sibling = tmp1;
            }
            tmp1 = sibling->left;
            if (!tmp1 || RbTree_is(tmp1, Black)) {
                tmp2 = sibling->right;
                if (!tmp2 || RbTree_is(tmp2, Black)) {
                    /* Case 2 - sibling color flip */
                    RbTree_set_parent_color(sibling, parent, Red);
                    if (RbTree_is(parent, Red))
                        RbTree_set_color(parent, Black);
                    else {
                        node = parent;
                        parent = RbTree_parent(node);
                        if (parent)
                            continue;
                    }
                    break;
                }
                /* Case 3 - left rotate at sibling */
                tmp1 = tmp2->left;
                sibling->right = tmp1;
                tmp2->left = sibling;
                parent->left = tmp2;
                if (tmp1)
                    RbTree_set_parent_color(tmp1, sibling, Black);
                augmentRotate(sibling, tmp2);
                tmp1 = sibling;
                sibling = tmp2;
            }
            /* Case 4 - right rotate at parent + color flips */
            tmp2 = sibling->right;
            parent->left = tmp2;
            sibling->right = parent;
            RbTree_set_parent_color(tmp1, sibling, Black);
            if (tmp2)
                RbTree_set_parent(tmp2, parent);
            RbTree_rotate_set_parents(root, parent, sibling, rbtBlack);
            augmentRotate(parent, sibling);
            break;
        }
    }
}

attr(always_inline)
static struct RbTreeNode *RbTree_erase_augmented_(
        RbTreeNode **root, RbTreeNode *node, const RbTreeNodeAugmentCallbacks *augment)
{
    RbTreeNode *child = node->right;
    RbTreeNode *tmp = node->left;
    RbTreeNode *parent, *rebalance;
    uptr pc;

    if (!tmp) {
        /*
         * Case 1: node to erase has no more than 1 child (easy!)
         *
         * Note that if there is one child it must be red due to 5)
         * and node must be black due to 4). We adjust colors locally
         * so as to bypass __rb_erase_color() later on.
         */
        pc = node->parent;
        parent = RbTree_parent(node);
        RbTree_change_child(root, node, child, parent);
        if (child) {
            child->parent = pc;
            rebalance = NULL;
        } else
            rebalance = RbTree_is(node, Black)? parent : NULL;
        tmp = parent;
    } else if (!child) {
        /* Still case 1, but this time the child is node->left */
        tmp->parent = pc = node->parent;
        parent = RbTree_parent(node);
        RbTree_change_child(root, node, tmp, parent);
        rebalance = NULL;
        tmp = parent;
    } else {
        RbTreeNode *successor = child, *child2;

        tmp = child->left;
        if (!tmp) {
            /*
             * Case 2: node's successor is its right child
             *
             *    (n)          (s)
             *    / \          / \
             *  (x) (s)  ->  (x) (c)
             *        \
             *        (c)
             */
            parent = successor;
            child2 = successor->right;

            augment->copy(node, successor);
        } else {
            /*
             * Case 3: node's successor is leftmost under
             * node's right child subtree
             *
             *    (n)          (s)
             *    / \          / \
             *  (x) (y)  ->  (x) (y)
             *      /            /
             *    (p)          (p)
             *    /            /
             *  (s)          (c)
             *    \
             *    (c)
             */
            do {
                parent = successor;
                successor = tmp;
                tmp = tmp->left;
            } while (tmp);
            child2 = successor->right;
            parent->left = child2;
            successor->right = child;
            child = successor;

            augment->copy(node, successor);
            augment->prop(parent, successor);
        }

        tmp = node->left;
        successor->left = tmp;
        RbTree_set_parent(tmp, successor);

        tmp = RbTree_parent(node);
        RbTree_change_child(root, node, successor, tmp);

        if (child2) {
            RbTree_set_parent(successor, node);
            RbTree_set_parent_color(child2, parent, Black);
            rebalance = NULL;
        } else {
            RbTree_set_parent(successor, node);
            rebalance = RbTree_is(successor, Black) ? parent : NULL;
        }
        tmp = successor;
    }

    augment->prop(tmp, NULL);
    return rebalance;
}

static inline void RbTree_dummy_copy(RbTreeNode *old, RbTreeNode *new) {}
static inline void RbTree_dummy_rotate(RbTreeNode *old, RbTreeNode *new) {}
static inline void RbTree_dummy_propagate(RbTreeNode *old, RbTreeNode *new) {}

static const RbTreeNodeAugmentCallbacks sRbTree_dummy_augment_callbacks = {
    .prop = RbTree_dummy_propagate,
    .copy = RbTree_dummy_copy,
    .rot = RbTree_dummy_rotate
};

attr(always_inline)
static void RbTree_insert_color(RbTreeNode **root, RbTreeNode *node)
{
    RbTree_insert__(root, node, RbTree_dummy_rotate);
}

attr(always_inline)
static void RbTree_erase_(RbTreeNode **root, RbTreeNode *node)
{
    RbTreeNode *rebalance;
    rebalance = RbTree_erase_augmented_(root, node, &sRbTree_dummy_augment_callbacks);

    if (rebalance)
        RbTree_erase_color__(root, rebalance, RbTree_dummy_rotate);
}

inline
static void RbTree_deinit_node(RbTreeNode *node, RbTreeElementDctor elementDctor)
{
    if (node == NULL) return;
    RbTree_deinit_node(node->left, elementDctor);
    RbTree_deinit_node(node->right, elementDctor);
    if (elementDctor) elementDctor(node->data);
    RbTree_node_clear(node);
    Allocator_dealloc(node);
}

attr(always_inline)
static void RbTree_init_node(RbTreeNode *node)
{
    node->parent = rbtRed;
    node->left = NULL;
    node->right = NULL;
    RbTree_clear_root(node);
}

RbTreeNode *RbTreeNode_next(RbTreeNode *node)
{
    RbTreeNode *parent;

    if (RbTree_node_is_empty(node))
        return NULL;

    /*
     * If we have a right-hand child, go down and then left as far
     * as we can.
     */
    if (node->right) {
        node = node->right;
        while (node->left)
            node = node->left;
        return node;
    }

    /*
     * No right-hand children. Everything down and left is smaller than us,
     * so any 'next' node must be in the general direction of our parent.
     * Go up the tree; any time the ancestor is a right-hand child of its
     * parent, keep going up. First time it's a left-hand child of its
     * parent, said parent is our 'next' node.
     */
    while ((parent = RbTree_parent(node)) && node == parent->right)
        node = parent;

    return parent;
}

RbTreeNode *RbTree_first(RbTreeBase *rbt)
{
    RbTreeNode 	*node;

    node = rbt->root;
    if (!node)
        return NULL;
    while (node->left)
        node = node->left;
    return node;
}

RbTreeNode *RbTreeNode_prev(RbTreeNode *node)
{
    RbTreeNode *parent;

    if (RbTree_node_is_empty(node))
        return NULL;

    /*
     * If we have a left-hand child, go down and then right as far
     * as we can.
     */
    if (node->left) {
        node = node->left;
        while (node->right)
            node = node->right;
        return (RbTreeNode *)node;
    }

    /*
     * No left-hand children. Go up till we find an ancestor which
     * is a right-hand child of its parent.
     */
    while ((parent = RbTree_parent(node)) && node == parent->left)
        node = parent;

    return parent;
}

void* RbTree_add_(RbTreeBase *rbt, const void *value, u32 len)
{
    RbTreeNode *parent = NULL, *node;
    RbTreeNode **link = &rbt->root;
    RbTreeCompare cmp = rbt->compare;

    csAssert0(cmp != NULL);
    node = Allocator_alloc(rbt->Alloc, sizeof(*node) + rbt->size);
    csAssert0(node != NULL);

    RbTree_init_node(node);

    while (*link) {
        parent = *link;
        if (cmp(value, len, parent->data) < 0)
            link = &parent->left;
        else
            link = &parent->right;
    }

    RbTree_link_node(link, node, parent);
    RbTree_insert_color(&rbt->root, node);
    return node->data;
}

FindOrAdd RbTree_find_or_add_(RbTreeBase *rbt, const void *value, u32 len)
{
    RbTreeNode *parent = NULL, *node;
    RbTreeNode **link = &rbt->root;
    RbTreeCompare cmp = rbt->compare;

    csAssert0(cmp != NULL);

    while (*link) {
        parent = *link;
        int res = cmp(value, len, parent->data);
        if (res < 0)
            link = &parent->left;
        else if (res > 0)
            link = &parent->right;
        else
            return (FindOrAdd){false, parent->data};
    }

    node = Allocator_alloc(rbt->Alloc, sizeof(*node) + rbt->size);
    csAssert0(node != NULL);

    RbTree_init_node(node);

    RbTree_link_node(link, node, parent);
    RbTree_insert_color(&rbt->root, node);

    return (FindOrAdd){true, node->data};
}

RbTreeNode *RbTree_find_(RbTreeBase *rbt, const void *value, u32 len)
{
    RbTreeNode *node = rbt->root;
    RbTreeCompare cmp = rbt->compare;
    csAssert0(cmp != NULL);

    while (node) {
        int res = cmp(value, len, node->data);

        if (res < 0)
            node = node->left;
        else if (res > 0)
            node = node->right;
        else
            return node;
    }

    return NULL;
}

RbTreeNode *RbTree_find_first_(RbTreeBase *rbt, const void *value, u32 len)
{
    RbTreeNode *node = rbt->root, *match;
    RbTreeCompare cmp = rbt->compare;
    csAssert0(cmp != NULL);

    while (node) {
        int res = cmp(value, len, node->data);
        if (res <= 0) {
            if (res == 0) match = node;
            node = node->left;
        }
        else
            node = node->right;
    }

    return match;
}

void RbTree_deinit_(RbTreeBase *rbt, RbTreeElementDctor dct)
{
    RbTree_deinit_node(rbt->root, dct);
    rbt->root = NULL;
}

void RbTree_dump_(RbTreeBase *rbt, void(*DumpValue)(const void*, char))
{
    int level = 1;
    csAssert0(DumpValue != NULL);

    while (RbTree_dump_level(rbt->root, level, DumpValue)) {
        level++;
        DumpValue(NULL, 0);
    }
}
