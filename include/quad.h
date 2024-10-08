#ifndef QUADTREE_H
#define QUADTREE_H

typedef enum
{
    NW,
    NE,
    SW,
    SE
} quad_quadrant;

typedef struct
{
    float x;
    float y;
    struct
    {
        float full;
        float half;
    } length;
} quad;

bool quad_contains (const quad *q, float x, float y);

void quad_subdivide (const quad *restrict q, quad_quadrant quadrant, quad *restrict out);

#endif // QUADTREE_H
