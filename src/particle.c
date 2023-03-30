#include "particle.h"
#include <stdlib.h>
#include <time.h>

void fill_particles_random(Particle *particles, int n, vec3_t min_pos, vec3_t max_pos)
{
    srand(time(NULL)); // Seed the random number generator with the current time
    for (int i = 0; i < n - 1; i++)
    {
        particles[i].pos.x = min_pos.x + (max_pos.x - min_pos.x) * ((double)rand() / RAND_MAX);
        particles[i].pos.y = (min_pos.y + (max_pos.y - min_pos.y) * ((double)rand() / RAND_MAX)) / 10.0;
        particles[i].pos.z = min_pos.z + (max_pos.z - min_pos.z) * ((double)rand() / RAND_MAX);

        particles[i].pos = vec3_mul((((double)rand() / RAND_MAX) * max_pos.x) + 5.0, vec3_normalize(particles[i].pos));

        particles[i].vel = vec3_mul(vec3_length(particles[i].pos) / (max_pos.x + 5.0), vec3_mul(100, vec3_normalize(vec3_cross(particles[i].pos, (vec3_t){0, 1, 0}))));

        particles[i].mass = 1000 + rand() % 50;
        particles[i].mass = particles[i].mass == 0.0 ? 1.0 : particles[i].mass;
    }
    particles[n - 1].pos.x = 0;
    particles[n - 1].pos.y = 0;
    particles[n - 1].pos.z = 0;

    particles[n - 1].vel.x = 0;
    particles[n - 1].vel.y = 0;
    particles[n - 1].vel.z = 0;

    particles[n - 1].mass = 50099999;
}

void update_particles(Particle *particles, int n, int start, int end, double dt)
{
    for (int i = start; i < end; i++)
    {
        particles[i].pos = vec3_add(particles[i].pos, vec3_mul(dt, particles[i].vel));
    }
}