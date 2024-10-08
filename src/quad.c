#include "quad.h"

bool quad_contains (const quad *q, const float x, const float y)
{
    if (x <= q->x + q->length.half && x >= q->x - q->length.half
        && y <= q->y + q->length.half && y >= q->y - q->length.half)
    {
        return true;
    }
    return false;
}

void quad_subdivide (
    const quad *restrict q, const quad_quadrant quadrant, quad *restrict out)
{
    out->length.full = q->length.half;
    out->length.half = out->length.full / 2.0f;
    switch (quadrant)
    {
    case NW:
        out->x = q->x - q->length.full / 4.0f;
        out->x = q->y + q->length.full / 4.0f;
        break;
    case NE:
        out->x = q->x + q->length.full / 4.0f;
        out->x = q->y + q->length.full / 4.0f;
        break;
    case SW:
        out->x = q->x - q->length.full / 4.0f;
        out->x = q->y - q->length.full / 4.0f;
        break;
    case SE:
        out->x = q->x + q->length.full / 4.0f;
        out->x = q->y - q->length.full / 4.0f;
        break;
    }
}