#include "octree.h"
#include <stdlib.h>

struct Octant {
    double center[3];
    double size;
    Octant *children[8];
    int num_particles;
    Particle *particles[MAX_PARTICLES_PER_OCTANT];
};

Octant *new_octant(double center[3], double size) {
    Octant *octant = malloc(sizeof(Octant));
    for (int i = 0; i < 3; i++) {
        octant->center[i] = center[i];
    }
    octant->size = size;
    for (int i = 0; i < 8; i++) {
        octant->children[i] = NULL;
    }
    octant->num_particles = 0;
    return octant;
}

void insert_particle(Octant *octant, Particle *particle) {
    if (octant->num_particles < MAX_PARTICLES_PER_OCTANT) {
        octant->particles[octant->num_particles] = particle;
        octant->num_particles++;
    } else {
        if (octant->children[0] == NULL) {
            double child_size = octant->size / 2;
            for (int i = 0; i < 8; i++) {
                double child_center[3];
                for (int j = 0; j < 3; j++) {
                    child_center[j] = octant->center[j] + child_size * ((i & 1<<j) ? 0.5 : -0.5);
                }
                octant->children[i] = new_octant(child_center, child_size);
            }
        }
        for (int i = 0; i < 8; i++) {
            if (particle_in_octant(particle, octant->children[i])) {
                insert_particle(octant->children[i], particle);
                break;
            }
        }
    }
}

void traverse_octree(Octant *octant, void (*func)(Particle *)) {
    for (int i = 0; i < octant->num_particles; i++) {
        func(octant->particles[i]);
    }
    for (int i = 0; i < 8; i++) {
        if (octant->children[i] != NULL) {
            traverse_octree(octant->children[i], func);
        }
    }
}

int particle_in_octant(Particle *particle, Octant *octant) {
    double min[3], max[3];
    for (int i = 0; i < 3; i++) {
        min[i] = octant->center[i] - octant->size / 2;
        max[i] = octant->center[i] + octant->size / 2;
    }
    return (particle->pos.x >= min[0] && particle->pos.x <= max[0] &&
            particle->pos.y >= min[1] && particle->pos.y <= max[1] &&
            particle->pos.z >= min[2] && particle->pos.z <= max[2]);
}