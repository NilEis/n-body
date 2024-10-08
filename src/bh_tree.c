#include "bh_tree.h"

void bh_tree_init (bh_tree *tree, quad *q)
{
    tree->quad.length.full = q->length.full;
    tree->quad.length.half = q->length.half;
    tree->quad.x = q->x;
    tree->quad.y = q->y;
}