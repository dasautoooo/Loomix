//
// Created by Leonard Chan on 3/9/25.
//

#ifndef CLOTH_H
#define CLOTH_H

#include <glm/glm.hpp>
#include <iostream>
#include <vector>

struct Particle {
	glm::vec3 pos;
	glm::vec3 velocity;
	glm::vec3 forceAccumulator;
	float mass;
};

struct Spring {
	// The indices of particles
	int p1, p2;
	float springConstant;
	float damperConstant;
	float currentLength;
	float restLength;

	enum class SpringType { STRUCTURE, SHEAR, BEND };

	SpringType type;
};

// The cloth system
class Cloth {
  public:
	Cloth();

	Cloth(uint32_t numX, uint32_t numY, float spacing);

	~Cloth() {};

	void init(uint32_t numX, uint32_t numY, float spacing);

	const std::vector<Particle> &getParticles() const { return particles; };

	// Step the cloth simulation by dt
	void update(float dt);

	// Access for changing parameters
	void setStructureSpringConstant(float ks);
	void setShearSpringConstant(float ks);
	void setBendingSpringConstant(float ks);

	void setStructureDamperConstant(float kd);
	void setShearDamperConstant(float kd);
	void setBendingDamperConstant(float kd);

	void setMaxSpeed(float mv) {maxSpeed = mv;};

	void setGravity(const glm::vec3 &g) { gravity = g; }
	void setMass(float m) { mass = m; }

	enum class PinMode {
		NONE,
		FOUR_CORNERS,
		TOP_CORNERS,
	};

	void pinCorners(PinMode mode);

	uint32_t getClothWidth() const { return numX + 1; }
	uint32_t getClothHeight() const { return numY + 1; }

  private:
	// Helper methods
	void addSpring(int p1Index, int p2Index, Spring::SpringType type);

	std::vector<glm::vec3> computeForces(const std::vector<glm::vec3> &positions,
	                                     const std::vector<glm::vec3> &velocities);
	void integrateRK4(float dt);
	// void integrateVerlet(float dt);

  private:
	// Grid resolution
	uint32_t numX, numY;
	uint32_t totalPoints;

	// Cloth geometry
	float spacing;
	float mass; // each node's mass
	glm::vec3 gravity;
	float maxSpeed;

	float structureSpringConstant;
	float shearSpringConstant;
	float bendingSpringConstant;

	float structureDamperConstant;
	float shearDamperConstant;
	float bendingDamperConstant;

	std::vector<bool> pinned;

	// Particles and Springs
	std::vector<Particle> particles;
	std::vector<Spring> springs;
	std::vector<glm::vec3> X; // positions
	std::vector<glm::vec3> V; // velocities
};

#endif // CLOTH_H
