#include "quad.h"

bool quad_contains (const quad *q, double x, double y)
{
    if (x <= q->x + q->width.half && x >= q->x - q->width.half
        && y <= q->y + q->height.half && y >= q->y - q->height.half)
    {
        return true;
    }
    return false;
}

void quad_subdivide (
    const quad *restrict q, const quad_quadrant quadrant, quad *restrict out)
{
    out->width.full = q->width.half;
    out->width.half = out->width.full / 2.0f;
    out->height.full = q->height.half;
    out->height.half = out->height.full / 2.0f;
    switch (quadrant)
    {
    case NW:
        out->x = q->x - q->width.full / 4.0f;
        out->y = q->y + q->height.full / 4.0f;
        break;
    case NE:
        out->x = q->x + q->width.full / 4.0f;
        out->y = q->y + q->height.full / 4.0f;
        break;
    case SW:
        out->x = q->x - q->width.full / 4.0f;
        out->y = q->y - q->height.full / 4.0f;
        break;
    case SE:
        out->x = q->x + q->width.full / 4.0f;
        out->y = q->y - q->height.full / 4.0f;
        break;
    }
}

quad_quadrant quad_get_quadrant (const quad *restrict q, double x, double y)
{
    if (x < q->x)
    {
        return y < q->y ? SW : NW;
    }
    return y < q->y ? SE : NE;
}