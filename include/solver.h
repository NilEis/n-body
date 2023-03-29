#ifndef SOLVER_H
#define SOLVER_H

#include "particle.h"

void compute_forces_newtonian(Particle particles[], int n, int start, int end);
void compute_forces_schwarzschild_GR(Particle particles[], int n, int start, int end);

#endif // SOLVER_H
