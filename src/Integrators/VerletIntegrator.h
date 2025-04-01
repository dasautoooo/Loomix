//
// Created by Leonard Chan on 4/1/25.
//

#ifndef VERLETINTEGRATOR_H
#define VERLETINTEGRATOR_H

#include "Integrator.h"

class VerletIntegrator : public Integrator {
  public:
	~VerletIntegrator() override;

	std::pair<std::vector<glm::vec3>, std::vector<glm::vec3>>
	integrate(const std::vector<glm::vec3> &X,
	          const std::vector<glm::vec3> &V,
	          float dt,
	          float mass,
	          const std::vector<bool> &pinned,
	          std::function<std::vector<glm::vec3>(const std::vector<glm::vec3> &,
	                                               const std::vector<glm::vec3> &)> computeForces)
	    override;

  private:
	// Internal storage for previous positions
	std::vector<glm::vec3> prevPositions;
	bool initialized = false;

  private:
	void reset();
};

#endif // VERLETINTEGRATOR_H
