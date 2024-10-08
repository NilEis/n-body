#ifndef BH_TREE_H
#define BH_TREE_H
#include "quad.h"
typedef struct bh_tree
{
    int value;
    quad quad;
    struct bh_tree *NW;
    struct bh_tree *NE;
    struct bh_tree *SW;
    struct bh_tree *SE;
} bh_tree;

void bh_tree_init (bh_tree *tree, quad *q);

#endif // BH_TREE_H
