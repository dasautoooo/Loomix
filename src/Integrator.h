//
// Created by Leonard Chan on 3/24/25.
//

#ifndef INTEGRATOR_H
#define INTEGRATOR_H

#include <functional>
#include <glm/glm.hpp>
#include <utility>
#include <vector>

class Integrator {
  public:
	virtual ~Integrator() = default;

	virtual std::pair<std::vector<glm::vec3>, std::vector<glm::vec3>> integrate(
	    const std::vector<glm::vec3> &X,
	    const std::vector<glm::vec3> &V,
	    float dt,
	    float mass,
	    const std::vector<bool> &pinned,
	    std::function<std::vector<glm::vec3>(const std::vector<glm::vec3> &,
	                                         const std::vector<glm::vec3> &)> computeForces) = 0;
};

#endif // INTEGRATOR_H
