#ifndef PARTICLE_H
#define PARTICLE_H
#include "vec3.h"
#include "camera.h"

typedef struct Particle {
    vec3_t pos;   // Position of the particle
    vec3_t vel;   // Velocity of the particle
    double mass;// The mass of the particle
} Particle;

void fill_particles_random(Particle *particles, int n, vec3_t min_pos, vec3_t max_pos);

void update_particles(Particle *particles, int n, int start, int end, double dt);

#endif // PARTICLE_H
