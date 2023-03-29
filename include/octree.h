#ifndef OCTREE_H
#define OCTREE_H

#include "particle.h"

#define MAX_PARTICLES_PER_OCTANT 50

typedef struct Octant Octant;

Octant *new_octant(double center[3], double size);

void insert_particle(Octant *octant, Particle *particle);

void traverse_octree(Octant *octant, void (*func)(Particle *));

int particle_in_octant(Particle *particle, Octant *octant);

#endif // OCTREE_H
