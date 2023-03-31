#define G 0.06

typedef double vec3_m_t;

typedef struct vec3_t {
  vec3_m_t x;
  vec3_m_t y;
  vec3_m_t z;
} vec3_t;

typedef struct Particle {
  vec3_t pos;  // Position of the particle
  vec3_t vel;  // Velocity of the particle
  double mass; // The mass of the particle
} Particle;
__kernel void update_particle(__global Particle *particles, const double dt,
                              int n) {
  const int tid = get_global_id(0);
  if (tid >= n)
    return;
  Particle p = particles[tid];
  p.pos.x += p.vel.x * dt;
  p.pos.y += p.vel.y * dt;
  p.pos.z += p.vel.z * dt;
  particles[tid] = p;
}

__kernel void newtonianSolver(__global Particle *particles, const double dt,
                              int n) {
  const int tid = get_global_id(0);
  if (tid >= n || tid == n - 1)
    return;

  Particle p = particles[tid];
  vec3_t accel = {0.0, 0.0, 0.0};

  for (int i = 0; i < n; i++) {
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

  // p.vel.x -= p.pos.x*0.01 * dt;
  // p.vel.y -= p.pos.y*0.01 * dt;
  // p.vel.z -= p.pos.z*0.01 * dt;

  particles[tid] = p;
}