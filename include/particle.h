#ifndef PARTICLE_H
#define PARTICLE_H
#include "vec3.h"
#include "camera.h"

typedef struct Particle {
    vec3 pos;   // Position of the particle
    vec3 vel;   // Velocity of the particle
    double mass;// The mass of the particle
} Particle;

#endif // PARTICLE_H
