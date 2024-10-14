#ifndef BH_TREE_H
#define BH_TREE_H
#include "defines.h"
#include "quad.h"
#include "state.h"

#include <math.h>
#include <stdbool.h>

#include "arena.h"

typedef struct
{
    double x;
    double y;
    double mass;
    bool active;
} body;

typedef struct bh_tree
{
    body node_body;
    quad quad;
    struct bh_tree *NW;
    struct bh_tree *NE;
    struct bh_tree *SW;
    struct bh_tree *SE;
    Arena *arena;
} bh_tree;

void bh_tree_init (bh_tree *tree, const quad *q, Arena *arena);
bool bh_tree_is_leaf (const bh_tree *tree);
void bh_tree_insert (bh_tree *restrict tree, const ant *restrict v, int d);
int bh_tree_apply_force (const bh_tree *restrict tree, ant *restrict v);

int bh_tree_get_depth (const bh_tree *tree);
int bh_tree_get_num_nodes (const bh_tree *tree);

extern state_t state;

static void apply_force (
    ant *a, const double x, const double y, const double w)
{
    const int index = 2 * a->pos_index;
    const double dx = x - (*state.ants_pos_read)[index];
    const double dy = y - (*state.ants_pos_read)[index + 1];
    double dist = sqrt (dx * dx + dy * dy);
    if (dist < EPSILON)
    {
        dist = EPSILON;
    }
    const double F = GRAVITATIONAL_CONSTANT * a->w * w / (dist * dist);
    OPENMP_PRAGMA (omp atomic)
    a->fx += F * dx / dist;
    OPENMP_PRAGMA (omp atomic)
    a->fy += F * dy / dist;
}
static void update_ant (ant *v)
{
    const double i_w = v->w;
    v->vx += v->fx / i_w;
    v->vy += v->fy / i_w;
    v->vx = v->vx == NAN ? 0 : v->vx;
    v->vy = v->vy == NAN ? 0 : v->vy;
    const int index = 2 * v->pos_index;
    (*state.ants_pos_write)[index]
        = (*state.ants_pos_read)[index] + (GLfloat)v->vx;
    (*state.ants_pos_write)[index + 1]
        = (*state.ants_pos_read)[index + 1] + (GLfloat)v->vy;
}

#endif // BH_TREE_H
