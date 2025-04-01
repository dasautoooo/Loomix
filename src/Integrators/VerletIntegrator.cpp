//
// Created by Leonard Chan on 4/1/25.
//

#include "VerletIntegrator.h"

VerletIntegrator::~VerletIntegrator() {
	reset(); // Clear internal state when object is destroyed
}

std::pair<std::vector<glm::vec3>, std::vector<glm::vec3>> VerletIntegrator::integrate(
    const std::vector<glm::vec3> &X,
    const std::vector<glm::vec3> &V,
    float dt,
    float mass,
    const std::vector<bool> &pinned,
    std::function<std::vector<glm::vec3>(const std::vector<glm::vec3> &,
                                         const std::vector<glm::vec3> &)> computeForces) {

	size_t N = X.size();
	std::vector<glm::vec3> Xout = X;
	std::vector<glm::vec3> Vout = V;

	if (!initialized) {
		// Estimate previous positions using backward Euler for initialization
		prevPositions.resize(N);
		for (size_t i = 0; i < N; ++i) {
			prevPositions[i] = X[i] - V[i] * dt;
		}
		initialized = true;
	}

	auto F = computeForces(X, V);

	for (size_t i = 0; i < N; ++i) {
		if (pinned[i])
			continue;

		glm::vec3 a = F[i] / mass;
		glm::vec3 newX = 2.0f * X[i] - prevPositions[i] + a * dt * dt;
		Vout[i] = (newX - prevPositions[i]) / (2.0f * dt);
		Xout[i] = newX;
	}

	prevPositions = X;
	return {Xout, Vout};
}

void VerletIntegrator::reset() {
	initialized = false;
	prevPositions.clear();
}