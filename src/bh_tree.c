#include "bh_tree.h"

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

void bh_tree_init (bh_tree *tree, quad *q)
{
    if (q != nullptr)
    {
        tree->quad.length.full = q->length.full;
        tree->quad.length.half = q->length.half;
        tree->quad.x = q->x;
        tree->quad.y = q->y;
    }
    tree->node_body.active = false;
    tree->NE = nullptr;
    tree->NW = nullptr;
    tree->SE = nullptr;
    tree->SW = nullptr;
}

bool bh_tree_is_leaf (const bh_tree *tree)
{
    return tree->NE == nullptr && tree->NW == nullptr && tree->SE == nullptr
        && tree->SW == nullptr;
}

void bh_tree_insert (bh_tree *restrict tree, const ant *restrict v)
{
    if (!tree->node_body.active)
    {
        tree->node_body.active = true;
    }
    else if (!bh_tree_is_leaf (tree))
    {
        bh_tree_add_body (tree, v);
        bh_tree **quad_tree = nullptr;
        switch (quad_get_quadrant (&tree->quad, *v->x, *v->y))
        {
        case NW:
            quad_tree = &tree->NW;
            break;
        case NE:
            quad_tree = &tree->NE;
            break;
        case SW:
            quad_tree = &tree->SW;
            break;
        case SE:
            quad_tree = &tree->SE;
            break;
        }
        if (*quad_tree == nullptr)
        {
            *quad_tree = calloc (1, sizeof (bh_tree));
            bh_tree_init (*quad_tree, nullptr);
            quad_subdivide (&tree->quad, NW, &(*quad_tree)->quad);
        }
        bh_tree_insert (*quad_tree, v);
    }
    else
    {
        bh_tree **quad_tree = nullptr;
        switch (quad_get_quadrant (
            &tree->quad, tree->node_body.x, tree->node_body.y))
        {
        case NW:
            quad_tree = &tree->NW;
            break;
        case NE:
            quad_tree = &tree->NE;
            break;
        case SW:
            quad_tree = &tree->SW;
            break;
        case SE:
            quad_tree = &tree->SE;
            break;
        }
        if (*quad_tree == nullptr)
        {
            *quad_tree = calloc (1, sizeof (bh_tree));
            bh_tree_init (*quad_tree, nullptr);
            quad_subdivide (&tree->quad, NW, &(*quad_tree)->quad);
        }
        bh_tree_insert_body (*quad_tree, &tree->node_body);
        bh_tree_insert (tree, v);
    }
}


