#ifndef CUDA_TICK_H
#define CUDA_TICK_H

#ifdef __cplusplus
extern "C"
{
#endif
#include "particle.h"
#include "defines.h"
#include "ma-log.h"

    void init_cuda_tick(Particle *p, int n);
    void cuda_tick(Particle *p, volatile int *running, int n, double dt);
    void free_cuda_tick(void);

#ifdef __cplusplus
}
#endif

#endif // CUDA_TICK_H
