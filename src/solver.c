#include "solver.h"

const static double G = 6.6743e-11;  // gravitational constant in m^3/(kg s^2)
const static double c = 299792458.0; // speed of light in m/s

void compute_forces_newtonian(Particle particles[], int n)
{
    int i, j;
    vec3 r;
    double dist, force_mag;

    for (i = 0; i < n; i++)
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
            force_mag = G * particles[i].mass * particles[j].mass / (dist * dist);

            particles[i].vel = vec3_add(particles[i].vel, vec3_div(vec3_mul(force_mag, r), particles[i].mass * dist));
        }
    }
}

void compute_forces_schwarzschild_GR(Particle particles[], int n)
{
    int i, j;
    vec3 r;
    double dist, force_mag, force_factor;

    for (i = 0; i < n; i++)
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
    }
}