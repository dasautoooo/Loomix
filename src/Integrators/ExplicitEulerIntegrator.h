//
// Created by Leonard Chan on 3/25/25.
//

#ifndef EXPLICITEULERINTEGRATOR_H
#define EXPLICITEULERINTEGRATOR_H

#include "Integrator.h"

class ExplicitEulerIntegrator : public Integrator {
  public:
	std::pair<std::vector<glm::vec3>, std::vector<glm::vec3>>
	integrate(const std::vector<glm::vec3> &X,
	          const std::vector<glm::vec3> &V,
	          float dt,
	          float mass,
	          const std::vector<bool> &pinned,
	          std::function<std::vector<glm::vec3>(const std::vector<glm::vec3> &,
	                                               const std::vector<glm::vec3> &)> computeForces)
	    override;
};

#endif // EXPLICITEULERINTEGRATOR_H
