//
// Created by Leonard Chan on 3/24/25.
//

#include "RK4Integrator.h"

std::pair<std::vector<glm::vec3>, std::vector<glm::vec3>> RK4Integrator::integrate(
    const std::vector<glm::vec3> &X,
    const std::vector<glm::vec3> &V,
    float dt,
    float mass,
    const std::vector<bool> &pinned,
    std::function<std::vector<glm::vec3>(const std::vector<glm::vec3> &,
                                         const std::vector<glm::vec3> &)> computeForces) {

	size_t N = X.size();

	std::vector<glm::vec3> Xout = X, Vout = V;

	// Define temporary arrays for the four derivative stages
	std::vector<glm::vec3> k1x(N), k1v(N);
	std::vector<glm::vec3> k2x(N), k2v(N);
	std::vector<glm::vec3> k3x(N), k3v(N);
	std::vector<glm::vec3> k4x(N), k4v(N);

	std::vector<glm::vec3> Ftemp;

	// ---- k1
	// derivative at the start
	// dx/dt = V, dv/dt = a = F/m
	// 1) compute forces with current X, V
	Ftemp = computeForces(Xout, Vout);
	for (size_t i = 0; i < N; i++) {
		k1x[i] = V[i];            // derivative of X is velocity
		k1v[i] = Ftemp[i] / mass; // derivative of V is acceleration
	}

	// ---- k2
	// Evaluate derivative at the midpoint (X + dt/2*k1x, V + dt/2*k1v)
	std::vector<glm::vec3> X2(N), V2(N);
	for (size_t i = 0; i < N; i++) {
		X2[i] = X[i] + 0.5f * dt * k1x[i];
		V2[i] = V[i] + 0.5f * dt * k1v[i];
	}
	Ftemp = computeForces(X2, V2);
	for (size_t i = 0; i < N; i++) {
		k2x[i] = V2[i];
		k2v[i] = Ftemp[i] / mass;
	}

	// ---- k3
	// another midpoint with k2
	for (size_t i = 0; i < N; i++) {
		X2[i] = X[i] + 0.5f * dt * k2x[i];
		V2[i] = V[i] + 0.5f * dt * k2v[i];
	}
	Ftemp = computeForces(X2, V2);
	for (size_t i = 0; i < N; i++) {
		k3x[i] = V2[i];
		k3v[i] = Ftemp[i] / mass;
	}

	// ---- k4
	// derivative at the end of the interval (X + dt*k3x, V + dt*k3v)
	std::vector<glm::vec3> X4(N), V4(N);
	for (size_t i = 0; i < N; i++) {
		X4[i] = X[i] + dt * k3x[i];
		V4[i] = V[i] + dt * k3v[i];
	}
	Ftemp = computeForces(X4, V4);
	for (size_t i = 0; i < N; i++) {
		k4x[i] = V4[i];
		k4v[i] = Ftemp[i] / mass;
	}

	// Now combine them:
	// X_{n+1} = X_n + dt/6 ( k1x + 2k2x + 2k3x + k4x )
	// V_{n+1} = V_n + dt/6 ( k1v + 2k2v + 2k3v + k4v )
	for (size_t i = 0; i < N; i++) {
		glm::vec3 dx = (k1x[i] + 2.f * k2x[i] + 2.f * k3x[i] + k4x[i]) * (dt / 6.f);
		glm::vec3 dv = (k1v[i] + 2.f * k2v[i] + 2.f * k3v[i] + k4v[i]) * (dt / 6.f);

		Xout[i] += dx;
		Vout[i] += dv;

		// e.g. floor collision
		// if (X[i].y < 0.f) {
		//     X[i].y = 0.f;
		//     V[i].y = 0.f; // zero vertical velocity if you want inelastic collisions
		// }
	}

	return std::make_pair(Xout, Vout);
}