#ifndef GLOBAL_DEFINES_H
#define GLOBAL_DEFINES_H

#ifndef WINDOW_WIDTH
#define WINDOW_WIDTH 600
#endif

#ifndef WINDOW_HEIGHT
#define WINDOW_HEIGHT 400
#endif

#ifndef NUM_PARTICLES
#define NUM_PARTICLES 2496//3744
#endif

#ifndef P_RANGE
#define P_RANGE 500.0
#endif

#ifndef NUM_THREADS
#define NUM_THREADS 3
#endif

#ifndef USE_PTHREAD
#define USE_PTHREAD 0
#endif

#ifndef USE_CUDA
#define USE_CUDA 1
#endif

#ifndef TIME_STEPS
#define TIME_STEPS 5
#endif

#if USE_CUDA
#define CUDA_THREADS_PER_BLOCK 192//288
#endif

#endif // GLOBAL_DEFINES_H
