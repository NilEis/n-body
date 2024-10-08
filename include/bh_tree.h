#ifndef BH_TREE_H
#define BH_TREE_H
#include "quad.h"

typedef struct
{
    GLfloat *x;
    GLfloat *y;
    double vx;
    double vy;
    double fx;
    double fy;
    double w;
    double kin;
    double pot;
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
} bh_tree;

void bh_tree_init (bh_tree *tree, quad *q);
bool bh_tree_is_leaf (const bh_tree *tree);
void bh_tree_insert (bh_tree *restrict tree, const ant *restrict v);

void

#endif // BH_TREE_H
