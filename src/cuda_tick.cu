#include "cuda_tick.h"

#define DELTA_TIME 0.001

__global__ void newtonianSolver(Particle *particles, const double dt);
__global__ void update_particle(Particle *particles, const double dt);

Particle *device_particles;

void init_cuda_tick(Particle *p)
{
    cudaMalloc((void **)&device_particles, NUM_PARTICLES * sizeof(Particle));
    cudaMemcpy(device_particles, p, NUM_PARTICLES * sizeof(Particle), cudaMemcpyHostToDevice);
}

void free_cuda_tick(void)
{
    cudaFree(device_particles);
}

void cuda_tick(Particle *p, volatile int *running)
{
    for (int i = 0; i < TIME_STEPS && *running; i++)
    {
        newtonianSolver<<<NUM_PARTICLES / CUDA_THREADS_PER_BLOCK, CUDA_THREADS_PER_BLOCK>>>(device_particles, DELTA_TIME);
        update_particle<<<NUM_PARTICLES / CUDA_THREADS_PER_BLOCK, CUDA_THREADS_PER_BLOCK>>>(device_particles, DELTA_TIME);
    }
    cudaMemcpy(p, device_particles, NUM_PARTICLES * sizeof(Particle), cudaMemcpyDeviceToHost);
}

__global__ void update_particle(Particle *particles, const double dt)
{
    const int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= NUM_PARTICLES)
        return;
    Particle p = particles[tid];
    p.pos.x += p.vel.x * dt;
    p.pos.y += p.vel.y * dt;
    p.pos.z += p.vel.z * dt;
    particles[tid] = p;
}

__global__ void newtonianSolver(Particle *particles, const double dt)
{
    const int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= NUM_PARTICLES)
        return;

    Particle p = particles[tid];
    vec3_t accel = {0.0, 0.0, 0.0};

    for (int i = 0; i < NUM_PARTICLES; i++)
    {
        if (i == tid)
            continue;

        Particle other = particles[i];

        vec3_t dist;
        dist.x = other.pos.x - p.pos.x;
        dist.y = other.pos.y - p.pos.y;
        dist.z = other.pos.z - p.pos.z;

        double r2 = dist.x * dist.x + dist.y * dist.y + dist.z * dist.z;
        double r = sqrt(r2);

        double f = other.mass / (r2 * r);

        accel.x += f * dist.x;
        accel.y += f * dist.y;
        accel.z += f * dist.z;
    }

    p.vel.x += accel.x * dt;
    p.vel.y += accel.y * dt;
    p.vel.z += accel.z * dt;

    particles[tid] = p;
}