#include "particle.h"
#include <stdlib.h>
#include <time.h>

void fill_particles_random(Particle *particles, int n, vec3_t min_pos, vec3_t max_pos)
{
    srand(time(NULL)); // Seed the random number generator with the current time
    for (int i = 0; i < n; i++)
    {
        particles[i].pos.x = min_pos.x + (max_pos.x - min_pos.x) * ((double)rand() / RAND_MAX);
        particles[i].pos.y = min_pos.y + (max_pos.y - min_pos.y) * ((double)rand() / RAND_MAX);
        particles[i].pos.z = min_pos.z + (max_pos.z - min_pos.z) * ((double)rand() / RAND_MAX);

        particles[i].pos = vec3_mul(((double)rand() / RAND_MAX) * max_pos.x, vec3_normalize(particles[i].pos));

        particles[i].vel = random_vec3(0, 15);

        particles[i].mass = 100000 + rand() % 50;
        particles[i].mass = particles[i].mass == 0.0 ? 1.0 : particles[i].mass;
    }
}

void update_particles(Particle *particles, int n, int start, int end)
{
    for (int i = start; i < end; i++)
    {
        particles[i].pos = vec3_add(particles[i].pos, particles[i].vel);
    }
}