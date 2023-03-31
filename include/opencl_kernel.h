#ifndef OPENCL_KERNEL_H
#define OPENCL_KERNEL_H

#include <stddef.h>

#define OPENCL_KERNEL "#define G 0.06\n"\
"\n"\
"typedef double vec3_m_t;\n"\
"\n"\
"typedef struct vec3_t {\n"\
"  vec3_m_t x;\n"\
"  vec3_m_t y;\n"\
"  vec3_m_t z;\n"\
"} vec3_t;\n"\
"\n"\
"typedef struct Particle {\n"\
"  vec3_t pos;  // Position of the particle\n"\
"  vec3_t vel;  // Velocity of the particle\n"\
"  double mass; // The mass of the particle\n"\
"} Particle;\n"\
"__kernel void update_particle(__global Particle *particles, const double dt,\n"\
"                              int n) {\n"\
"  const int tid = get_global_id(0);\n"\
"  if (tid >= n)\n"\
"    return;\n"\
"  Particle p = particles[tid];\n"\
"  p.pos.x += p.vel.x * dt;\n"\
"  p.pos.y += p.vel.y * dt;\n"\
"  p.pos.z += p.vel.z * dt;\n"\
"  particles[tid] = p;\n"\
"}\n"\
"\n"\
"__kernel void newtonianSolver(__global Particle *particles, const double dt,\n"\
"                              int n) {\n"\
"  const int tid = get_global_id(0);\n"\
"  if (tid >= n || tid == n - 1)\n"\
"    return;\n"\
"\n"\
"  Particle p = particles[tid];\n"\
"  vec3_t accel = {0.0, 0.0, 0.0};\n"\
"\n"\
"  for (int i = 0; i < n; i++) {\n"\
"    if (i == tid)\n"\
"      continue;\n"\
"\n"\
"    Particle other = particles[i];\n"\
"\n"\
"    vec3_t dist;\n"\
"    dist.x = other.pos.x - p.pos.x;\n"\
"    dist.y = other.pos.y - p.pos.y;\n"\
"    dist.z = other.pos.z - p.pos.z;\n"\
"\n"\
"    double r2 = dist.x * dist.x + dist.y * dist.y + dist.z * dist.z;\n"\
"    double r = sqrt(r2);\n"\
"\n"\
"    double f = other.mass / (r2 * r);\n"\
"\n"\
"    accel.x += G * f * dist.x;\n"\
"    accel.y += G * f * dist.y;\n"\
"    accel.z += G * f * dist.z;\n"\
"  }\n"\
"\n"\
"  p.vel.x += accel.x * dt;\n"\
"  p.vel.y += accel.y * dt;\n"\
"  p.vel.z += accel.z * dt;\n"\
"\n"\
"  // p.vel.x -= p.pos.x*0.01 * dt;\n"\
"  // p.vel.y -= p.pos.y*0.01 * dt;\n"\
"  // p.vel.z -= p.pos.z*0.01 * dt;\n"\
"\n"\
"  particles[tid] = p;\n"\
"}"
const char const *opencl_kernel_source = OPENCL_KERNEL;
const size_t opencl_kernel_source_size = (sizeof(OPENCL_KERNEL) / sizeof(OPENCL_KERNEL[0])) - 1;
#undef OPENCL_KERNEL

#endif // OPENCL_KERNEL_H
