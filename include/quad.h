#ifndef QUADTREE_H
#define QUADTREE_H

typedef enum
{
    NW,
    NE,
    SW,
    SE
} quad_quadrant;

typedef double quad_member_type;

typedef struct
{
    quad_member_type full;
    quad_member_type half;
} quad_length;

typedef struct
{
    quad_member_type x;
    quad_member_type y;
    quad_length width;
    quad_length height;
} quad;

bool quad_contains (const quad *q, float x, float y);

void quad_subdivide (
    const quad *restrict q, quad_quadrant quadrant, quad *restrict out);

quad_quadrant quad_get_quadrant (const quad *restrict q, float x, float y);

#endif // QUADTREE_H
