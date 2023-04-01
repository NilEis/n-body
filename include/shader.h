#ifndef SHADER_H
#define SHADER_H

#include <stddef.h>

#define SHADER_fs "#version 330 core\n"\
"\n"\
"out vec4 FragColor;\n"\
"\n"\
"void main() {\n"\
"    FragColor = vec4(gl_FragCoord.xyz,1.0);\n"\
"}"
const char const *basic_shader_fs = SHADER_fs;
const size_t basic_shader_fs_size = sizeof(SHADER_fs) / sizeof(SHADER_fs[0]);
#undef SHADER_fs

#define SHADER_vs "#version 330 core\n"\
"\n"\
"in vec4 vertexPosition;\n"\
"\n"\
"uniform mat4 model;\n"\
"uniform mat4 view;\n"\
"uniform mat4 projection;\n"\
"\n"\
"void main()\n"\
"{\n"\
"    gl_Position = projection * view * model * vertexPosition;;\n"\
"}"
const char const *basic_shader_vs = SHADER_vs;
const size_t basic_shader_vs_size = sizeof(SHADER_vs) / sizeof(SHADER_vs[0]);
#undef SHADER_vs

#define SHADER_SOLVE_comp "#version 450\n"\
"\n"\
"struct Particle {\n"\
"    dvec3 pos;\n"\
"    dvec3 vel;\n"\
"    double mass;\n"\
"};\n"\
"\n"\
"uniform double dt = 0.1;\n"\
"\n"\
"layout(std430, binding = 0) buffer Particles {\n"\
"    Particle particles[];\n"\
"};\n"\
"\n"\
"layout (local_size_x = 64, local_size_y = 1, local_size_z = 1) in;\n"\
"\n"\
"const double G = 0.06;//6.67408e-11;\n"\
"\n"\
"void main() {\n"\
"    uint tid = gl_GlobalInvocationID.x;\n"\
"\n"\
"    if (tid >= particles.length() || tid == particles.length() - 1)\n"\
"        return;\n"\
"\n"\
"    Particle p;\n"\
"    dvec3 accel = dvec3(0.0, 0.0, 0.0);\n"\
"\n"\
"    for (int i = 0; i < particles.length(); i++) {\n"\
"        if (i == tid)\n"\
"            continue;\n"\
"\n"\
"        Particle other = particles[i];\n"\
"\n"\
"        dvec3 dist = other.pos - p.pos;\n"\
"        double r2 = dot(dist, dist);\n"\
"        double r = sqrt(r2);\n"\
"\n"\
"        double f = other.mass / (r2 * r);\n"\
"\n"\
"        accel += G * f * dist;\n"\
"    }\n"\
"\n"\
"    p.vel += accel * dt;\n"\
"    p.pos += p.vel * dt;\n"\
"\n"\
"    particles[tid] = p;\n"\
"}"
const char const *shader_solve_comp = SHADER_SOLVE_comp;
const size_t shader_solve_comp_size = sizeof(SHADER_SOLVE_comp) / sizeof(SHADER_SOLVE_comp[0]);
#undef SHADER_SOLVE_comp

#define SHADER_UPDATE_comp "#version 450\n"\
"\n"\
"struct Particle {\n"\
"    dvec3 pos;\n"\
"    dvec3 vel;\n"\
"    double mass;\n"\
"};\n"\
"\n"\
"uniform double dt = 0.1;\n"\
"\n"\
"layout(std430, binding = 0) buffer Particles {\n"\
"Particle particles[];\n"\
"};\n"\
"\n"\
"layout(local_size_x = 64, local_size_y = 1, local_size_z = 1) in;\n"\
"\n"\
"const double G = 0.06;//6.67408e-11;\n"\
"\n"\
"void main() {\n"\
"uint tid = gl_GlobalInvocationID.x;\n"\
"if(tid >= particles.length()) return;\n"\
"Particle p = particles[tid];\n"\
"p.pos += p.vel * dt;\n"\
"particles[tid].pos = dvec3(p.pos.xy, gl_GlobalInvocationID.x);\n"\
"}"
const char const *shader_update_comp = SHADER_UPDATE_comp;
const size_t shader_update_comp_size = sizeof(SHADER_UPDATE_comp) / sizeof(SHADER_UPDATE_comp[0]);
#undef SHADER_UPDATE_comp

#endif // SHADER_H
