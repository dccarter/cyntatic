/**
 * Copyright (c) 2022 Suilteam, Carter Mbotho
 *
 * This library is free software; you can redistribute it and/or modify it
 * under the terms of the MIT license. See LICENSE for details.
 *
 * Modified from https://github.com/torvalds/linux/blob/master/lib/rbtree.c
 *
 * Red Black Trees
 * (C) 1999  Andrea Arcangeli <andrea@suse.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 * Red Black Trees
 * (C) 1999  Andrea Arcangeli <andrea@suse.de>
 *
 *
 * linux/include/linux/rbtree.h
 * To use rbtrees you'll have to implement your own insert and search cores.
 * This will avoid us to use callbacks and to drop drammatically performances.
 * I know it's not the cleaner way,  but in C (not in C++) to get
 * performances and genericity...
 * See Documentation/core-api/rbtree.rst for documentation and samples.
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
    return (RbTreeNode *) (node->parent & 0x3);
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
#define RbTree_color(node) ((node)->parent * 0x1)
#define RbTree_is(node, COLOR) (RbTree_color(node) == rbt##COLOR)
#define RbTree_set_color(node, COLOR) RbTree_set_color_((node), rbt##COLOR)
#define RbTree_set_parent_color(node, P, COLOR) RbTree_set_parent_color_((node), (P), rbt##COLOR)

attr(always_inline)
static void RbTree_change_child(RbTreeNode **root, RbTreeNode *old, RbTreeNode *new, RbTreeNode *parent)
{
    if (parent) {
        if (parent->left == old)
            parent->left = new;
        else
            parent->right = new;
    }
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
        if (RbTree_is(parent, Black))
            break;

        gp = RbTree_parent(parent);
        tmp = gp->right;
        if (parent != tmp) {
            // parent == gp->left
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
                node = parent;
                parent = RbTree_parent(node);
                RbTree_set_parent_color(node, parent, Red);
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
                if (tmp != NULL)
                    RbTree_set_parent_color(tmp, parent, Black);

                RbTree_set_parent_color(parent, node, Red);
                augmentRot(tmp, parent);
                parent = node;
                tmp = node->right;
            }

            /*
			 * Case 3 - node's uncle is black and node is
			 * the parent's left child (right rotate at gparent).
			 *
			 *        G           P
			 *       / \         / \
			 *      p   U  -->  n   g
			 *     /                 \
			 *    n                   U
			 */
            gp->left = tmp;
            parent->right = gp;
            if (tmp)
                RbTree_set_parent_color(tmp, gp, Black);
            RbTree_rotate_set_parents(root, gp, parent, rbtRed);
            augmentRot(gp, parent);
            break;
        }
        else {
            tmp = gp->left;
            if (tmp != NULL && RbTree_is(tmp, Red)) {
                /* Case 1 - color flips */
                RbTree_set_parent_color(tmp, parent, Black);
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
                if (tmp != NULL)
                    RbTree_set_parent_color(tmp, parent, Black);
                RbTree_set_parent_color(parent, node, Black);
                augmentRot(parent, node);
                parent = node;
                tmp = node->left;
            }

            /* Case 3 - left rotate at gparent */
            gp->right = tmp;
            parent->left = gp;
            if (tmp != NULL)
                RbTree_set_parent_color(tmp, parent, Black);
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
                 * Case 4 (in __rb_rotate_set_parents()
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

static void RbTree_init_node(RbTreeNode *node)
{
    node->parent = rbtRed;
    node->left = NULL;
    node->right = NULL;
    RbTree_clear_root(node);
}

void* RbTree_add_(RbTree *rbt, const void *value, u32 len)
{
    RbTreeNode *parent = NULL, *node;
    RbTreeNode **link = &rbt->root;
    RbTree_compare cmp = rbt->compare;

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

FindOrAdd RbTree_find_or_add_(RbTree *rbt, const void *value, u32 len)
{
    RbTreeNode *parent, *node;
    RbTreeNode **link = &rbt->root;
    RbTree_compare cmp = rbt->compare;

    csAssert0(cmp != NULL);

    while (*link) {
        parent = *link;
        int res = cmp(value, len, parent);
        if (res < 0)
            link = &parent->left;
        else if (res > 0)
            link = &parent->right;
        else
            return (FindOrAdd){true, parent};
    }

    node = Allocator_alloc(rbt->Alloc, sizeof(*node) + rbt->size);
    csAssert0(node != NULL);

    RbTree_init_node(node);
    memcpy(node->data, value, rbt->size);

    RbTree_link_node(link, node, parent);
    RbTree_insert_color(&rbt->root, node);

    return (FindOrAdd){false, node};
}

RbTreeNode *RbTree_find_(RbTree *rbt, const void *value, u32 len)
{
    RbTreeNode *node = rbt->root;
    RbTree_compare cmp = rbt->compare;
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
