#ifndef BH_TREE_H
#define BH_TREE_H
#include "defines.h"
#include "quad.h"

#include <math.h>
#include <stdbool.h>

#include "arena.h"

#include <log.h>

typedef struct
{
    GLfloat *x;
    GLfloat *y;
    double vx;
    double vy;
    double fx;
    double fy;
    double w;
} ant;

typedef struct
{
    float x;
    float y;
    float mass;
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
void bh_tree_insert (bh_tree *restrict tree, const ant *restrict v);
int bh_tree_apply_force (const bh_tree *restrict tree, ant *restrict v);

static void apply_force (
    ant *a, const GLfloat x, const GLfloat y, const double w)
{
    const float dx = x - *a->x;
    const float dy = y - *a->y;
    double dist = sqrt (dx * dx + dy * dy);
    if (dist < EPSILON)
    {
        dist = EPSILON;
    }
    const double F = GRAVITATIONAL_CONSTANT * a->w * w / (dist * dist);
    a->fx += F * dx / dist;
    a->fy += F * dy / dist;
}
static void update_ant (ant *v)
{
    const double i_w = v->w;
    v->vx += v->fx / i_w;
    v->vy += v->fy / i_w;
    v->vx = v->vx == NAN ? 0 : v->vx;
    v->vy = v->vy == NAN ? 0 : v->vy;
    *v->x += (GLfloat)v->vx;
    *v->y += (GLfloat)v->vy;
}

#endif // BH_TREE_H
