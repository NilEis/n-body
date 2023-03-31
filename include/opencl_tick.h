#ifndef OPENCL_TICK_H
#define OPENCL_TICK_H

#ifdef __cplusplus
extern "C"
{
#endif
#include "particle.h"
#include "defines.h"
#include "ma-log.h"

    void init_opencl_tick(Particle *p, int n);
    void opencl_tick(Particle *p, volatile int *running, int n, double dt);
    void free_opencl_tick(void);

#ifdef __cplusplus
}
#endif

#endif // OPENCL_TICK_H
