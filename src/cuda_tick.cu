#include "cuda_tick.h"

#define G 0.06

static int threads_per_block_solver = 0;
static int threads_per_block_update = 0;
static int grid_size_solver = 0;
static int grid_size_update = 0;

__global__ void newtonianSolver(Particle *particles, const double dt, int n);
__global__ void update_particle(Particle *particles, const double dt, int n);

Particle *device_particles;

void init_cuda_tick(Particle *p, int n)
{
    cudaMalloc((void **)&device_particles, n * sizeof(Particle));
    cudaMemcpy(device_particles, p, n * sizeof(Particle), cudaMemcpyHostToDevice);
    cudaOccupancyMaxPotentialBlockSize(&grid_size_solver, &threads_per_block_solver, newtonianSolver, 0, 0);
    cudaOccupancyMaxPotentialBlockSize(&grid_size_update, &threads_per_block_update, update_particle, 0, 0);
    grid_size_solver = n / threads_per_block_solver;
    grid_size_solver = n % threads_per_block_solver != 0 ? grid_size_solver + 1 : grid_size_solver;
    grid_size_solver = grid_size_solver == 0 ? 1 : grid_size_solver;
    grid_size_update = n / threads_per_block_update;
    grid_size_update = n % threads_per_block_update != 0 ? grid_size_update + 1 : grid_size_update;
    grid_size_update = grid_size_update == 0 ? 1 : grid_size_update;
    LOG(LOG_INFO, "Using %d blocks with %d threads = %d for solver\n", grid_size_solver, threads_per_block_solver, threads_per_block_solver * grid_size_solver);
    LOG(LOG_INFO, "Using %d blocks with %d threads = %d for update\n", grid_size_update, threads_per_block_update, threads_per_block_update * grid_size_update);
}

void free_cuda_tick(void)
{
    cudaFree(device_particles);
}

void cuda_tick(Particle *p, volatile int *running, int n, double dt)
{
    for (int i = 0; i < TIME_STEPS && *running; i++)
    {
        newtonianSolver<<<grid_size_solver, threads_per_block_solver>>>(device_particles, dt, n);
        update_particle<<<grid_size_update, threads_per_block_update>>>(device_particles, dt, n);
    }
    cudaMemcpy(p, device_particles, n * sizeof(Particle), cudaMemcpyDeviceToHost);
}

__global__ void update_particle(Particle *particles, const double dt, int n)
{
    const int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= n)
        return;
    Particle p = particles[tid];
    p.pos.x += p.vel.x * dt;
    p.pos.y += p.vel.y * dt;
    p.pos.z += p.vel.z * dt;
    particles[tid] = p;
}

__global__ void newtonianSolver(Particle *particles, const double dt, int n)
{
    const int tid = blockIdx.x * blockDim.x + threadIdx.x;
    if (tid >= n || tid == n - 1)
        return;

    Particle p = particles[tid];
    vec3_t accel = {0.0, 0.0, 0.0};

    for (int i = 0; i < n; i++)
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

        accel.x += G * f * dist.x;
        accel.y += G * f * dist.y;
        accel.z += G * f * dist.z;
    }

    p.vel.x += accel.x * dt;
    p.vel.y += accel.y * dt;
    p.vel.z += accel.z * dt;

    particles[tid] = p;
}