//
// Created by Leonard Chan on 3/25/25.
//

#include "ExplicitEulerIntegrator.h"

std::pair<std::vector<glm::vec3>, std::vector<glm::vec3>> ExplicitEulerIntegrator::integrate(
    const std::vector<glm::vec3> &X,
    const std::vector<glm::vec3> &V,
    float dt,
    float mass,
    const std::vector<bool> &pinned,
    std::function<std::vector<glm::vec3>(const std::vector<glm::vec3> &,
                                         const std::vector<glm::vec3> &)> computeForces) {
	size_t N = X.size();
	std::vector<glm::vec3> Xout = X, Vout = V;

	// 1) compute forces
	auto F = computeForces(X, V);

	// 2) do Euler
	for (size_t i = 0; i < N; i++) {
		if (pinned[i])
			continue;

		glm::vec3 a = F[i] / mass;
		// v += a dt
		Vout[i] += a * dt;
		// x += v dt
		Xout[i] += Vout[i] * dt;
	}

	// return updated X, V
	return {Xout, Vout};
}