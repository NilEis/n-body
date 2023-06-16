#ifndef GLOBAL_DEFINES_H
#define GLOBAL_DEFINES_H

#ifndef P_RANGE
#define P_RANGE 1000.0
#endif

#ifndef NUM_THREADS
#define NUM_THREADS 0
#endif

#ifndef USE_CUDA
#define USE_CUDA 0
#endif

#ifndef USE_OPENCL
#define USE_OPENCL 0
#endif

#ifndef USE_PTHREAD
#if !USE_CUDA
#define USE_PTHREAD 1
#else
#define USE_PTHREAD 0
#endif
#endif

#ifndef TIME_STEPS
#define TIME_STEPS 1
#endif

#endif // GLOBAL_DEFINES_H
