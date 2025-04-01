//
// Created by Leonard Chan on 3/9/25.
//

#include "Cloth.h"

#include "Integrators/ExplicitEulerIntegrator.h"
#include "Integrators/RK4Integrator.h"
#include "Integrators/VerletIntegrator.h"

Cloth::Cloth()
    : numX(0), numY(0), totalPoints(0), spacing(0.2f), mass(1.0f), gravity(0.f, -0.00981f, 0.f),
      maxSpeed(20.0f), structureSpringConstant(75.0f), structureDamperConstant(0.5f),
      shearSpringConstant(50.0f), shearDamperConstant(0.3f), bendingSpringConstant(10.0f),
      bendingDamperConstant(0.1f) {}

Cloth::Cloth(uint32_t numX, uint32_t numY, float spacing) : Cloth() { init(numX, numY, spacing); }

void Cloth::init(uint32_t numX, uint32_t numY, float spacing) {
	this->numX = numX;
	this->numY = numY;
	this->spacing = spacing;
	totalPoints = (numX + 1) * (numY + 1);

	// Clear any existing data
	particles.clear();
	springs.clear();
	X.clear();
	V.clear();

	// 1) Create grid of Particles
	particles.resize(totalPoints);
	X.resize(totalPoints);
	V.resize(totalPoints);
	for (uint32_t y = 0; y <= numY; y++) {
		for (uint32_t x = 0; x <= numX; x++) {
			// index in 1D array
			int idx = y * (numX + 1) + x;

			Particle &p = particles[idx];
			// place cloth in the XZ plane, at Y=0
			p.pos = glm::vec3(x * spacing, 0.0f, -static_cast<float>(y) * spacing);
			p.velocity = glm::vec3(0.0f);
			p.forceAccumulator = glm::vec3(0.0f);
			p.mass = mass; // use the cloth's mass field
		}
	}

	// 2) Create structural springs (horizontal + vertical)

	// Horizontal structural
	for (uint32_t row = 0; row <= numY; row++) {
		for (uint32_t col = 0; col < numX; col++) {
			int i1 = row * (numX + 1) + col;
			int i2 = i1 + 1; // next in x direction
			addSpring(i1, i2, Spring::SpringType::STRUCTURE);
		}
	}

	// Vertical structural
	for (uint32_t col = 0; col <= numX; col++) {
		for (uint32_t row = 0; row < numY; row++) {
			int i1 = row * (numX + 1) + col;
			int i2 = (row + 1) * (numX + 1) + col; // next in y direction
			addSpring(i1, i2, Spring::SpringType::STRUCTURE);
		}
	}

	// 3) Create shear (diagonal) springs
	for (uint32_t row = 0; row < numY; row++) {
		for (uint32_t col = 0; col < numX; col++) {
			int i0 = row * (numX + 1) + col;
			int i1 = i0 + 1;
			int i2 = i0 + (numX + 1);
			int i3 = i2 + 1;
			// diag #1: i0 -> i3
			addSpring(i0, i3, Spring::SpringType::SHEAR);
			// diag #2: i1 -> i2
			addSpring(i1, i2, Spring::SpringType::SHEAR);
		}
	}

	// 4) Create bend springs (two steps away in x or y)

	// Horizontal bend
	for (uint32_t row = 0; row <= numY; row++) {
		for (uint32_t col = 0; col + 2 <= numX; col += 1) {
			int i1 = row * (numX + 1) + col;
			int i2 = i1 + 2;
			addSpring(i1, i2, Spring::SpringType::BEND);
		}
	}

	// Vertical bend
	for (uint32_t col = 0; col <= numX; col++) {
		for (uint32_t row = 0; row + 2 <= numY; row += 1) {
			int i1 = row * (numX + 1) + col;
			int i2 = (row + 2) * (numX + 1) + col;
			addSpring(i1, i2, Spring::SpringType::BEND);
		}
	}
}

//------------------------------------
// Simple function to pin corners
//------------------------------------
void Cloth::pinCorners(PinMode mode) {
	// Make sure we have pinned storage
	if (pinned.size() != particles.size()) {
		pinned.resize(particles.size(), false);
	}

	// Clear pinned
	std::fill(pinned.begin(), pinned.end(), false);

	switch (mode) {
	case PinMode::NONE:
		return;
	case PinMode::FOUR_CORNERS:
		if (numX > 0 && numY > 0) {
			// top-left => index 0
			pinned[0] = true;
			// top-right => numX
			pinned[numX] = true;
			// bottom-left => row = numY => idx= numY*(numX+1)
			int bl = numY * (numX + 1);
			pinned[bl] = true;
			// bottom-right => bl + numX
			pinned[bl + numX] = true;
		}
		break;
	case PinMode::TOP_CORNERS:
		pinned[0] = true;
		pinned[numX] = true;
		break;
	}
}

void Cloth::setIntegrator(IntegrationMethod method){
	switch (method) {
	case IntegrationMethod::EXPLICIT_EULER:
		integrator = std::move(std::make_unique<ExplicitEulerIntegrator>());
		break;
	case IntegrationMethod::RUNGE_KUTTA:
		integrator = std::move(std::make_unique<RK4Integrator>());
		break;
	case IntegrationMethod::VERLET:
		integrator = std::move(std::make_unique<VerletIntegrator>());
		break;
	}
}

void Cloth::addSpring(int p1Index, int p2Index, Spring::SpringType type) {
	Spring s;
	s.type = type;

	switch (type) {
	case Spring::SpringType::STRUCTURE:
		s.springConstant = structureSpringConstant;
		s.damperConstant = structureDamperConstant;
		break;
	case Spring::SpringType::SHEAR:
		s.springConstant = shearSpringConstant;
		s.damperConstant = shearDamperConstant;
		break;
	case Spring::SpringType::BEND:
		s.springConstant = bendingSpringConstant;
		s.damperConstant = bendingDamperConstant;
		break;
	}

	s.p1 = p1Index;
	s.p2 = p2Index;

	// Compute rest length from the difference of the two Particles' positions
	glm::vec3 dp = particles[p1Index].pos - particles[p2Index].pos;
	s.restLength = glm::length(dp);
	s.currentLength = s.restLength;

	// Finally push into the springs array
	springs.push_back(s);
}

//------------------------------------
// Update cloth by dt
//------------------------------------
void Cloth::update(float dt) {
	if (!integrator)
		return; // if no integrator set, skip

	// 1) build local arrays X, V
	std::vector<glm::vec3> X(particles.size()), V(particles.size());
	for (size_t i = 0; i < particles.size(); i++) {
		X[i] = particles[i].pos;
		V[i] = particles[i].velocity;
	}

	// 2) define a lambda for computeForces
	auto forceFunc = [this](const std::vector<glm::vec3> &Xarr,
	                        const std::vector<glm::vec3> &Varr) -> std::vector<glm::vec3> {
		return this->computeForces(Xarr, Varr);
	};

	// 3) call integrator->integrate
	auto [Xout, Vout] = integrator->integrate(X, V, dt, mass, pinned, forceFunc);

	// 4) velocity clamp
	velocityClamp(Vout);

	// 5) store them back
	for (size_t i = 0; i < particles.size(); i++) {
		if (!pinned[i]) {
			particles[i].pos = Xout[i];
			particles[i].velocity = Vout[i];
		}
	}
}

void Cloth::setStructureSpringConstant(float ks) {
	structureSpringConstant = ks;
	// Update all existing STRUCTURE springs
	for (auto &s : springs) {
		if (s.type == Spring::SpringType::STRUCTURE) {
			s.springConstant = ks;
		}
	}
}

void Cloth::setShearSpringConstant(float ks) {
	shearSpringConstant = ks;
	// Update all existing SHEAR springs
	for (auto &s : springs) {
		if (s.type == Spring::SpringType::SHEAR) {
			s.springConstant = ks;
		}
	}
}

void Cloth::setBendingSpringConstant(float ks) {
	bendingSpringConstant = ks;
	// Update all existing BEND springs
	for (auto &s : springs) {
		if (s.type == Spring::SpringType::BEND) {
			s.springConstant = ks;
		}
	}
}

void Cloth::setStructureDamperConstant(float kd) {
	structureDamperConstant = kd;
	// Update all existing STRUCTURE springs
	for (auto &s : springs) {
		if (s.type == Spring::SpringType::STRUCTURE) {
			s.damperConstant = kd;
		}
	}
}

void Cloth::setShearDamperConstant(float kd) {
	shearDamperConstant = kd;
	// Update all existing SHEAR springs
	for (auto &s : springs) {
		if (s.type == Spring::SpringType::SHEAR) {
			s.damperConstant = kd;
		}
	}
}

void Cloth::setBendingDamperConstant(float kd) {
	bendingDamperConstant = kd;
	// Update all existing BEND springs
	for (auto &s : springs) {
		if (s.type == Spring::SpringType::BEND) {
			s.damperConstant = kd;
		}
	}
}

//------------------------------------
// Compute forces: gravity + damping + spring
//------------------------------------
std::vector<glm::vec3> Cloth::computeForces(const std::vector<glm::vec3> &positions,
                                            const std::vector<glm::vec3> &velocities) {
	std::vector<glm::vec3> forceAccumulators(particles.size(), glm::vec3(0.0f));

	// 1) Zero out all force accumulators
	for (size_t i = 0; i < forceAccumulators.size(); i++) {
		forceAccumulators[i] = glm::vec3(0.0f);
	}

	// 2) Apply gravity
	for (size_t i = 0; i < positions.size(); i++) {
		if (pinned[i]) {
			// If pinned, skip forces
			continue;
		}
		// add m*g
		forceAccumulators[i] += mass * gravity;
	}

	// 3) Spring forces (STRUCTURE, SHEAR, BEND all stored in springs)
	//    Each spring has its own damperConstant (s.damperConstant)

	// "biphasic" check for super-elastic
	const float biphasicFactor = 1.1f; // threshold
	const float superScale = 2.0f;     // how much stiffer it becomes

	for (auto &s : springs) {
		int iA = s.p1;
		int iB = s.p2;

		// If both endpoints pinned, skip
		if (pinned[iA] && pinned[iB]) {
			continue;
		}

		// Current positions & velocities
		glm::vec3 deltaP = positions[iA] - positions[iB];
		float dist = glm::length(deltaP);
		if (dist < 1e-7f)
			continue;                  // avoid division by zero
		glm::vec3 dir = deltaP / dist; // unit direction

		// Hooke’s law: F_spring = -k * (dist - restLen)
		float stretch = dist - s.restLength;

		// BIPHASIC LOGIC:
		// if dist > biphasicFactor * restLength => scale up springConstant
		float currKs = s.springConstant;
		// if (dist > s.restLength * biphasicFactor) {
		// 	currKs *= superScale;
		// }

		float springForceMag = -currKs * stretch;

		// Per-spring damping force along the line
		// F_damp = c * (relative velocity dot dir)
		glm::vec3 relVel = velocities[iA] - velocities[iB];
		float dampingMag = s.damperConstant * glm::dot(relVel, dir);

		// Net spring force
		glm::vec3 force = (springForceMag + dampingMag) * dir;

		// Accumulate force on each particle (skip if pinned)
		if (!pinned[iA])
			forceAccumulators[iA] += force;
		if (!pinned[iB])
			forceAccumulators[iB] -= force;
	}

	return forceAccumulators;
}

bool Cloth::isSpringLengthUnstable() {
	const float MAX_EXTENSION_RATIO = 3.0f; // Springs stretched to 3x their rest length

	for (auto &spring : springs) {
		glm::vec3 deltaP = particles[spring.p1].pos - particles[spring.p2].pos;
		spring.currentLength = glm::length(deltaP);

		if (spring.currentLength > spring.restLength * MAX_EXTENSION_RATIO) {
			return true;
		}
	}
	return false;
}

bool Cloth::isVelocityUnstable() {
	// For comparing with previous frame
	static std::vector<glm::vec3> previousVelocities;
	static bool firstFrame = true;
	const float MAX_VELOCITY_CHANGE_RATIO = 5.0f;

	// Initialize previous velocities if needed
	if (firstFrame || previousVelocities.size() != particles.size()) {
		previousVelocities.resize(particles.size());
		for (size_t i = 0; i < particles.size(); i++) {
			previousVelocities[i] = particles[i].velocity;
		}
		firstFrame = false;
		return false;
	}

	// Check for instability
	for (size_t i = 0; i < particles.size(); i++) {
		// Skip pinned particles
		if (pinned.size() > i && pinned[i])
			continue;

		const glm::vec3 &v = particles[i].velocity;
		float speed = glm::length(v);

		// Relative change check
		glm::vec3 prevV = previousVelocities[i];
		float prevSpeed = glm::length(prevV);

		if (prevSpeed > 0.01f) { // Only if previous velocity was significant
			float changeRatio = speed / prevSpeed;
			if (changeRatio > MAX_VELOCITY_CHANGE_RATIO) {
				return true;
			}
		}
	}

	// Update previous velocities for next frame
	for (size_t i = 0; i < particles.size(); i++) {
		previousVelocities[i] = particles[i].velocity;
	}

	return false;
}

void Cloth::velocityClamp(std::vector<glm::vec3> &velocities) {
	for (size_t i = 0; i < V.size(); i++) {
		float speed = glm::length(velocities[i]);
		if (speed > maxSpeed) {
			velocities[i] *= (maxSpeed / speed);
		}
	}
}
