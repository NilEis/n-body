#include "particle.h"
#include <stdlib.h>
#include <time.h>

void fill_particles_random(Particle *particles, int n, vec3_t min_pos, vec3_t max_pos)
{
    srand(time(NULL)); // Seed the random number generator with the current time
    for (int i = 0; i < n - 1; i++)
    {
        particles[i].pos[0] = min_pos[0] + (max_pos[0] - min_pos[0]) * ((double)rand() / RAND_MAX);
        particles[i].pos[1] = (min_pos[1] + (max_pos[1] - min_pos[1]) * ((double)rand() / RAND_MAX)) / 10.0;
        particles[i].pos[2] = min_pos[2] + (max_pos[2] - min_pos[2]) * ((double)rand() / RAND_MAX);
        particles[i].pos[3] = 0;
        particles[i].pos = vec3_mul((((double)rand() / RAND_MAX) * max_pos[0]) + 5.0, vec3_normalize(particles[i].pos));
        particles[i].pos[1] /= 5.0;

        if (vec3_length(particles[i].pos) <= 15)
        {
            particles[i].vel = vec3_mul(500, vec3_normalize(vec3_cross(vec3_normalize(particles[i].pos), (vec3_t){0.0, 1.0, 0.0})));
        }
        else
        {
            particles[i].vel = vec3_mul(100, vec3_normalize(vec3_cross(vec3_normalize(particles[i].pos), (vec3_t){0.0, 1.0, 0.0})));
        }
        particles[i].mass = 100 + rand() % 50;
        particles[i].mass = particles[i].mass == 0.0 ? 1.0 : particles[i].mass;
    }
    particles[n - 1].pos[0] = 0;
    particles[n - 1].pos[1] = 0;
    particles[n - 1].pos[2] = 0;
    particles[n - 1].pos[3] = 0;

    particles[n - 1].vel[0] = 0;
    particles[n - 1].vel[1] = 0;
    particles[n - 1].vel[2] = 0;
    particles[n - 1].vel[3] = 0;

    particles[n - 1].mass = 50099999;
}

void update_particles(Particle *particles, int n, int start, int end, double dt)
{
    for (int i = start; i < end; i++)
    {
        particles[i].pos = vec3_add(particles[i].pos, vec3_mul(dt, particles[i].vel));
    }
}