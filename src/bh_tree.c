﻿#include "bh_tree.h"

#include <stdlib.h>

static void bh_tree_add_body (bh_tree *restrict tree, const ant *restrict v)
{
    const float combined_mass = tree->node_body.mass + v->w;
    tree->node_body.x
        = (tree->node_body.x * tree->node_body.mass + *v->x * v->w)
        / combined_mass;
    tree->node_body.y
        = (tree->node_body.y * tree->node_body.mass + *v->y * v->w)
        / combined_mass;
    tree->node_body.mass = combined_mass;
}

static void bh_tree_insert_body (
    bh_tree *restrict tree, const body *restrict v)
{
    tree->node_body.active = true;
    tree->node_body.x = v->x;
    tree->node_body.y = v->y;
    tree->node_body.mass = v->mass;
}

void bh_tree_init (bh_tree *tree, const quad *q, Arena *arena)
{
    if (q != NULL)
    {
        tree->quad.width.full = q->width.full;
        tree->quad.width.half = q->width.half;
        tree->quad.height.full = q->height.full;
        tree->quad.height.half = q->height.half;
        tree->quad.x = q->x;
        tree->quad.y = q->y;
    }
    tree->node_body.active = false;
    tree->NE = NULL;
    tree->NW = NULL;
    tree->SE = NULL;
    tree->SW = NULL;
    tree->arena = arena;
}

bool bh_tree_is_leaf (const bh_tree *tree)
{
    return tree->NE == NULL && tree->NW == NULL && tree->SE == NULL
        && tree->SW == NULL;
}

static bh_tree *bh_tree_split (bh_tree *tree, float x, float y)
{
    bh_tree **quad_tree = NULL;
    quad_quadrant quadrant = NW;
    switch (quad_get_quadrant (&tree->quad, x, y))
    {
    case NW:
        quadrant = NW;
        quad_tree = &tree->NW;
        break;
    case NE:
        quadrant = NE;
        quad_tree = &tree->NE;
        break;
    case SW:
        quadrant = SW;
        quad_tree = &tree->SW;
        break;
    case SE:
        quadrant = SE;
        quad_tree = &tree->SE;
        break;
    }
    if (*quad_tree == NULL)
    {
        *quad_tree = arena_alloc (tree->arena, sizeof (bh_tree));
        bh_tree_init (*quad_tree, NULL, tree->arena);
        quad_subdivide (&tree->quad, quadrant, &(*quad_tree)->quad);
    }
    return *quad_tree;
}

void bh_tree_insert (bh_tree *restrict tree, const ant *restrict v)
{
    if (!tree->node_body.active)
    {
        tree->node_body.active = true;
        tree->node_body.x = *v->x;
        tree->node_body.y = *v->y;
        tree->node_body.mass = v->w;
    }
    else if (!bh_tree_is_leaf (tree))
    {
        bh_tree_add_body (tree, v);
        if (tree->quad.width.half > TREE_EPSILON
            && tree->quad.height.half > TREE_EPSILON)
        {
            bh_tree *sub_tree = bh_tree_split (tree, *v->x, *v->y);
            bh_tree_insert (sub_tree, v);
        }
    }
    else
    {
        bh_tree *sub_tree
            = bh_tree_split (tree, tree->node_body.x, tree->node_body.y);
        bh_tree_insert_body (sub_tree, &tree->node_body);
        bh_tree_insert (tree, v);
    }
}

static float dist (
    const float x1, const float y1, const float x2, const float y2)
{
    return sqrtf (powf (x2 - x1, 2) + powf (y2 - y1, 2));
}

int bh_tree_apply_force (const bh_tree *restrict tree, ant *restrict v)
{
    const double THETA = 2;
    if (bh_tree_is_leaf (tree))
    {
        if (tree->node_body.x != *v->x && tree->node_body.y != *v->y)
        {
            apply_force (
                v, tree->node_body.x, tree->node_body.y, tree->node_body.mass);
        }
    }
    else if (tree->quad.width.full
                     / dist (
                         *v->x, *v->x, tree->node_body.x, tree->node_body.x)
                 < THETA
             && tree->quad.height.full
                        / dist (
                            *v->y, *v->y, tree->node_body.y, tree->node_body.y)
                    < THETA)
    {
        apply_force (
            v, tree->node_body.x, tree->node_body.y, tree->node_body.mass);
    }
    else
    {
        if (tree->NW != NULL)
        {
            bh_tree_apply_force (tree->NW, v);
        }
        if (tree->NE != NULL)
        {
            bh_tree_apply_force (tree->NE, v);
        }
        if (tree->SW != NULL)
        {
            bh_tree_apply_force (tree->SW, v);
        }
        if (tree->SE != NULL)
        {
            bh_tree_apply_force (tree->SE, v);
        }
    }
}
