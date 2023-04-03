#include "cuda_tick.h"
#ifndef USE_OCTREE
#define USE_OCTREE 0
#endif
#if USE_OCTREE
#include "octree.h"
#endif

#define G 0.06

typedef struct BoundingBox
{
    vec3_m_t xmin;
    vec3_m_t ymin;
    vec3_m_t zmin;
    vec3_m_t xmax;
    vec3_m_t ymax;
    vec3_m_t zmax;
} BoundingBox;

static struct
{
    int threads_per_block_solver;
    int threads_per_block_update;
    int grid_size_solver;
    int grid_size_update;
} cuda_state;

BoundingBox compute_bounding_box(Particle *p, int n);

__global__ void newtonianSolver(Particle *particles, const double dt, int n);
__global__ void update_particle(Particle *particles, const double dt, int n);

Particle *device_particles;

void init_cuda_tick(Particle *p, int n)
{
    cudaMalloc((void **)&device_particles, n * sizeof(Particle));
    cudaMemcpy(device_particles, p, n * sizeof(Particle), cudaMemcpyHostToDevice);
    cudaOccupancyMaxPotentialBlockSize(&cuda_state.grid_size_solver, &cuda_state.threads_per_block_solver, newtonianSolver, 0, 0);
    cudaOccupancyMaxPotentialBlockSize(&cuda_state.grid_size_update, &cuda_state.threads_per_block_update, update_particle, 0, 0);
    cuda_state.grid_size_solver = n / cuda_state.threads_per_block_solver;
    cuda_state.grid_size_solver = n % cuda_state.threads_per_block_solver != 0 ? cuda_state.grid_size_solver + 1 : cuda_state.grid_size_solver;
    cuda_state.grid_size_solver = cuda_state.grid_size_solver == 0 ? 1 : cuda_state.grid_size_solver;
    cuda_state.grid_size_update = n / cuda_state.threads_per_block_update;
    cuda_state.grid_size_update = n % cuda_state.threads_per_block_update != 0 ? cuda_state.grid_size_update + 1 : cuda_state.grid_size_update;
    cuda_state.grid_size_update = cuda_state.grid_size_update == 0 ? 1 : cuda_state.grid_size_update;
    LOG(LOG_INFO, "Using %d blocks with %d threads = %d for solver\n", cuda_state.grid_size_solver, cuda_state.threads_per_block_solver, cuda_state.threads_per_block_solver * cuda_state.grid_size_solver);
    LOG(LOG_INFO, "Using %d blocks with %d threads = %d for update\n", cuda_state.grid_size_update, cuda_state.threads_per_block_update, cuda_state.threads_per_block_update * cuda_state.grid_size_update);
}

void free_cuda_tick(void)
{
    cudaFree(device_particles);
}

void cuda_tick(Particle *p, volatile int *running, int n, double dt)
{
    BoundingBox b_box = compute_bounding_box(p, n);
    //LOG(LOG_INFO, "(%f, %f, %f) <-> (%f, %f, %f)\n", b_box.xmin, b_box.ymin, b_box.zmin, b_box.xmax, b_box.ymax, b_box.zmax);
    for (int i = 0; i < TIME_STEPS && *running; i++)
    {
        newtonianSolver<<<cuda_state.grid_size_solver, cuda_state.threads_per_block_solver>>>(device_particles, dt, n);
        update_particle<<<cuda_state.grid_size_update, cuda_state.threads_per_block_update>>>(device_particles, dt, n);
    }
    cudaMemcpy(p, device_particles, n * sizeof(Particle), cudaMemcpyDeviceToHost);
}

BoundingBox compute_bounding_box(Particle *p, int n)
{
    BoundingBox ret;
    ret.xmin = p[0].pos.x;
    ret.ymin = p[0].pos.y;
    ret.zmin = p[0].pos.z;
    ret.xmax = p[0].pos.x;
    ret.ymax = p[0].pos.y;
    ret.zmax = p[0].pos.z;
    for (int i = 0; i < n; i++)
    {
        ret.xmin = p[i].pos.x < ret.xmin ? p[i].pos.x : ret.xmin;
        ret.ymin = p[i].pos.y < ret.ymin ? p[i].pos.y : ret.ymin;
        ret.zmin = p[i].pos.z < ret.zmin ? p[i].pos.z : ret.zmin;
        ret.xmax = p[i].pos.x > ret.xmax ? p[i].pos.x : ret.xmax;
        ret.ymax = p[i].pos.y > ret.ymax ? p[i].pos.y : ret.ymax;
        ret.zmax = p[i].pos.z > ret.zmax ? p[i].pos.z : ret.zmax;
    }
    return ret;
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

#if USE_OCTREE

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

#else

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
#endif