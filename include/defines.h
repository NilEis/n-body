#ifndef DEFINES_H
#define DEFINES_H

#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#define MAP_WIDTH (WINDOW_WIDTH / 1)
#define MAP_HEIGHT (WINDOW_HEIGHT / 1)
#define NUM_UNIFORMS 3
#define NUM_ANTS 20000
#define NUM_COMP_SHADERS 2

#define SIZE_UNIFORM_INDEX 0
#define MAP_SIZE_INDEX 1
#define TIME_UNIFORM_INDEX 2

#define UNIVERSE_SIZE 1e6
#define GRAVITATIONAL_CONSTANT 6.67E-11
#define EPSILON 1.5E2
#define TREE_EPSILON 0.00001
#define TREE_MAX_DEPTH 40

#define MACRO_SQUARED_DIST(a, b) ((a) * (a) + (b) * (b))

#ifndef INCLUDED_FROM_GLSL_CODE

#define USE_OPENMP 1
#if USE_OPENMP
#define OPENMP_PRAGMA(x) _Pragma (#x)
#else
#define OPENMP_PRAGMA(x)
#endif

#endif

#endif // DEFINES_H
