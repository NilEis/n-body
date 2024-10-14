#include "bh_tree.h"

#include <stdlib.h>

static int bh_tree_get_depth_rec (const bh_tree *tree);
static int bh_tree_get_num_nodes_rec (const bh_tree *tree);
static void bh_tree_add_body (bh_tree *restrict tree, const ant *restrict v);
static void bh_tree_insert_body (
    bh_tree *restrict tree, const body *restrict v);

static void bh_tree_add_body (bh_tree *restrict tree, const ant *restrict v)
{
    const float combined_mass = tree->node_body.mass + v->w;
    const int index = 2 * v->pos_index;
    tree->node_body.x = (tree->node_body.x * tree->node_body.mass
                            + (*state.ants_pos_read)[index] * v->w)
                      / combined_mass;
    tree->node_body.y = (tree->node_body.y * tree->node_body.mass
                            + (*state.ants_pos_read)[index + 1] * v->w)
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

void bh_tree_insert (bh_tree *restrict tree, const ant *restrict v, int d)
{
    if (!tree->node_body.active)
    {
        const int index = 2 * v->pos_index;
        tree->node_body.active = true;
        tree->node_body.x = (*state.ants_pos_read)[index];
        tree->node_body.y = (*state.ants_pos_read)[index + 1];
        tree->node_body.mass = v->w;
    }
    else if (!bh_tree_is_leaf (tree))
    {
        bh_tree_add_body (tree, v);
        if (d < TREE_MAX_DEPTH
            && (tree->quad.width.half > TREE_EPSILON
                && tree->quad.height.half > TREE_EPSILON))
        {
            const int index = 2 * v->pos_index;
            bh_tree *sub_tree = bh_tree_split (tree,
                (*state.ants_pos_read)[index],
                (*state.ants_pos_read)[index + 1]);
            bh_tree_insert (sub_tree, v, d + 1);
        }
    }
    else
    {
        bh_tree *sub_tree
            = bh_tree_split (tree, tree->node_body.x, tree->node_body.y);
        bh_tree_insert_body (sub_tree, &tree->node_body);
        bh_tree_insert (tree, v, d + 1);
    }
}

static float dist (
    const float x1, const float y1, const float x2, const float y2)
{
    return sqrtf (powf (x2 - x1, 2) + powf (y2 - y1, 2));
}

int bh_tree_apply_force (const bh_tree *restrict tree, ant *restrict v)
{
    const double THETA = 2.25;
    const int index = 2 * v->pos_index;
    if (bh_tree_is_leaf (tree))
    {
        if (tree->node_body.x != (*state.ants_pos_read)[index]
            && tree->node_body.y != (*state.ants_pos_read)[index + 1])
        {
            apply_force (
                v, tree->node_body.x, tree->node_body.y, tree->node_body.mass);
        }
    }
    else if (tree->quad.width.full
                     / dist ((*state.ants_pos_read)[index],
                         (*state.ants_pos_read)[index],
                         tree->node_body.x,
                         tree->node_body.x)
                 < THETA
             && tree->quad.height.full
                        / dist ((*state.ants_pos_read)[index + 1],
                            (*state.ants_pos_read)[index + 1],
                            tree->node_body.y,
                            tree->node_body.y)
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

static int bh_tree_get_depth_rec (const bh_tree *tree)
{
    if (tree == NULL)
    {
        return 0;
    }
    if (bh_tree_is_leaf (tree))
    {
        return 1;
    }
    int res = 0;
    OPENMP_PRAGMA (omp parallel sections reduction (max : res))
    {
        OPENMP_PRAGMA (omp section)
        {
            const int tmp = bh_tree_get_depth_rec (tree->NE);
            res = res < tmp ? tmp : res;
        }
        OPENMP_PRAGMA (omp section)
        {
            const int tmp = bh_tree_get_depth_rec (tree->NW);
            res = res < tmp ? tmp : res;
        }
        OPENMP_PRAGMA (omp section)
        {
            const int tmp = bh_tree_get_depth_rec (tree->SE);
            res = res < tmp ? tmp : res;
        }
        OPENMP_PRAGMA (omp section)
        {
            const int tmp = bh_tree_get_depth_rec (tree->SW);
            res = res < tmp ? tmp : res;
        }
    }
    return 1 + res;
}

int bh_tree_get_num_nodes_rec (const bh_tree *tree)
{
    if(tree == NULL)
    {
        return 0;
    }
    if(bh_tree_is_leaf (tree))
    {
        return 1;
    }
    int res = 0;
    OPENMP_PRAGMA (omp parallel sections reduction (+ : res))
    {
        OPENMP_PRAGMA (omp section)
        {
            res += bh_tree_get_depth_rec (tree->NE);
        }
        OPENMP_PRAGMA (omp section)
        {
            res += bh_tree_get_depth_rec (tree->NW);
        }
        OPENMP_PRAGMA (omp section)
        {
            res += bh_tree_get_depth_rec (tree->SE);
        }
        OPENMP_PRAGMA (omp section)
        {
            res += bh_tree_get_depth_rec (tree->SW);
        }
    }
    return res;
}

int bh_tree_get_depth (const bh_tree *tree)
{
    return bh_tree_get_depth_rec (tree);
}
int bh_tree_get_num_nodes (const bh_tree *tree)
{
    return bh_tree_get_num_nodes_rec (tree);
}
