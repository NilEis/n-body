#include "solver.h"

const static vec3_m_t G = 6.6743e-5;   // gravitational constant in m^3/(kg s^2)
const static vec3_m_t c = 299792458.0; // speed of light in m/s

void compute_forces_newtonian(Particle particles[], int n, int start, int end)
{
    int i, j;
    vec3_t r;
    vec3_m_t dist, force_mag;

    for (i = start; i < end; i++)
    {
        particles[i].vel.x = 0.0;
        particles[i].vel.y = 0.0;
        particles[i].vel.z = 0.0;

        for (j = 0; j < n; j++)
        {
            if (i == j)
            {
                continue;
            }

            r = vec3_sub(particles[j].pos, particles[i].pos);

            dist = vec3_length(r);
            force_mag = G * particles[i].mass * particles[j].mass / (dist * dist);

            particles[i].vel = vec3_add(particles[i].vel, vec3_div(vec3_mul(force_mag, r), particles[i].mass * dist));
        }
    }
}

void compute_forces_schwarzschild_GR(Particle particles[], int n, int start, int end)
{
    int i, j;
    vec3_t r;
    vec3_m_t dist, force_mag, force_factor;
    for (i = start; i < end; i++)
    {
        particles[i].vel.x = particles[i].vel.y = particles[i].vel.z = 0.0;

        for (j = 0; j < n; j++)
        {
            if (i == j)
            {
                continue;
            }

            r = vec3_sub(particles[j].pos, particles[i].pos);

            dist = vec3_length(r);
            force_factor = 1.0 - 2.0 * G * particles[j].mass / (c * c * dist);
            force_mag = G * particles[i].mass * particles[j].mass / (dist * dist * force_factor);
            particles[i].vel = vec3_add(particles[i].vel, vec3_div(vec3_mul(force_mag, r), particles[i].mass * c * c * force_factor));
        }
        //printf("%f,%f,%f\n", particles[i].vel.x, particles[i].vel.y, particles[i].vel.z);
    }
}