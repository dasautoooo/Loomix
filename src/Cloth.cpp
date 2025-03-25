//
// Created by Leonard Chan on 3/9/25.
//

#include "Cloth.h"

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

	// 4) velocity clamp or collisions if you want
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
		forceAccumulators[i] += particles[i].mass * gravity;
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

void Cloth::velocityClamp(std::vector<glm::vec3> &velocities) {
	for (size_t i = 0; i < V.size(); i++) {
		float speed = glm::length(velocities[i]);
		if (speed > maxSpeed) {
			velocities[i] *= (maxSpeed / speed);
		}
	}
}
//
// void Cloth::integrateRK4(float dt) {
// 	// Gather X, V from particles
// 	for (size_t i = 0; i < particles.size(); i++) {
// 		X[i] = particles[i].pos;
// 		V[i] = particles[i].velocity;
// 	}
//
// 	// Define temporary arrays for the four derivative stages
// 	std::vector<glm::vec3> k1x(X.size()), k1v(X.size());
// 	std::vector<glm::vec3> k2x(X.size()), k2v(X.size());
// 	std::vector<glm::vec3> k3x(X.size()), k3v(X.size());
// 	std::vector<glm::vec3> k4x(X.size()), k4v(X.size());
//
// 	std::vector<glm::vec3> Ftemp;
//
// 	// ---- k1
// 	// derivative at the start
// 	// dx/dt = V, dv/dt = a = F/m
// 	// 1) compute forces with current X, V
// 	Ftemp = computeForces(X, V);
// 	for (size_t i = 0; i < X.size(); i++) {
// 		k1x[i] = V[i];            // derivative of X is velocity
// 		k1v[i] = Ftemp[i] / mass; // derivative of V is acceleration
// 	}
//
// 	// ---- k2
// 	// Evaluate derivative at the midpoint (X + dt/2*k1x, V + dt/2*k1v)
// 	std::vector<glm::vec3> X2(X.size()), V2(X.size());
// 	for (size_t i = 0; i < X.size(); i++) {
// 		X2[i] = X[i] + 0.5f * dt * k1x[i];
// 		V2[i] = V[i] + 0.5f * dt * k1v[i];
// 	}
// 	Ftemp = computeForces(X2, V2);
// 	for (size_t i = 0; i < X.size(); i++) {
// 		k2x[i] = V2[i];
// 		k2v[i] = Ftemp[i] / mass;
// 	}
//
// 	// ---- k3
// 	// another midpoint with k2
// 	for (size_t i = 0; i < X.size(); i++) {
// 		X2[i] = X[i] + 0.5f * dt * k2x[i];
// 		V2[i] = V[i] + 0.5f * dt * k2v[i];
// 	}
// 	Ftemp = computeForces(X2, V2);
// 	for (size_t i = 0; i < X.size(); i++) {
// 		k3x[i] = V2[i];
// 		k3v[i] = Ftemp[i] / mass;
// 	}
//
// 	// ---- k4
// 	// derivative at the end of the interval (X + dt*k3x, V + dt*k3v)
// 	std::vector<glm::vec3> X4(X.size()), V4(X.size());
// 	for (size_t i = 0; i < X.size(); i++) {
// 		X4[i] = X[i] + dt * k3x[i];
// 		V4[i] = V[i] + dt * k3v[i];
// 	}
// 	Ftemp = computeForces(X4, V4);
// 	for (size_t i = 0; i < X.size(); i++) {
// 		k4x[i] = V4[i];
// 		k4v[i] = Ftemp[i] / mass;
// 	}
//
// 	// Now combine them:
// 	// X_{n+1} = X_n + dt/6 ( k1x + 2k2x + 2k3x + k4x )
// 	// V_{n+1} = V_n + dt/6 ( k1v + 2k2v + 2k3v + k4v )
// 	for (size_t i = 0; i < X.size(); i++) {
// 		glm::vec3 dx = (k1x[i] + 2.f * k2x[i] + 2.f * k3x[i] + k4x[i]) * (dt / 6.f);
// 		glm::vec3 dv = (k1v[i] + 2.f * k2v[i] + 2.f * k3v[i] + k4v[i]) * (dt / 6.f);
//
// 		X[i] += dx;
// 		V[i] += dv;
//
// 		// e.g. floor collision
// 		// if (X[i].y < 0.f) {
// 		//     X[i].y = 0.f;
// 		//     V[i].y = 0.f; // zero vertical velocity if you want inelastic collisions
// 		// }
// 	}
//
// 	// Velocity clamping
// 	for (size_t i = 0; i < V.size(); i++) {
// 		float speed = glm::length(V[i]);
// 		if (speed > maxSpeed) {
// 			V[i] *= (maxSpeed / speed);
// 		}
// 	}
//
// 	// Put them back into the cloth’s Particle array
// 	for (size_t i = 0; i < particles.size(); i++) {
// 		particles[i].pos = X[i];
// 		particles[i].velocity = V[i];
// 	}
// }

//------------------------------------
// Integrate using classical position-based Verlet
//------------------------------------
// void Cloth::integrateVerlet(float dt)
// {
//     float dt2mass = (dt * dt) / mass;
//     for (size_t i = 0; i < X.size(); i++) {
//         // if pinned, skip
//         if (pinned[i]) {
//             continue;
//         }
//         // otherwise, standard Verlet
//         glm::vec3 temp = X[i];
//         X[i] = X[i] + (X[i] - X_last[i]) + F[i] * dt2mass;
//         X_last[i] = temp;
//
//         // e.g. floor collision
//         if (X[i].y < 0.f) {
//             X[i].y = 0.f;
//         }
//     }
// }

//------------------------------------
// Ellipsoid collision
//------------------------------------
// void Cloth::ellipsoidCollision()
// {
//     // e.g. from your code, we do a transform approach
//     for(size_t i=0; i< X.size(); i++){
//         glm::vec4 xp= inverse_ellipsoid* glm::vec4(X[i],1.f);
//         glm::vec3 delta= glm::vec3(xp)- center;
//         float dist= glm::length(delta);
//         if(dist< radius){
//             float diff= radius- dist;
//             glm::vec3 fix= (diff* delta)/ dist;
//             xp.x+= fix.x;
//             xp.y+= fix.y;
//             xp.z+= fix.z;
//
//             // transform back
//             glm::vec3 newPos= glm::vec3(ellipsoid* xp);
//             X[i]= newPos;
//             X_last[i]= newPos;
//         }
//     }
// }

//------------------------------------
// Provot dynamic inverse
//------------------------------------
// void Cloth::applyProvotInverse()
// {
//     for(auto &s: springs){
//         glm::vec3 p1= X[s.p1];
//         glm::vec3 p2= X[s.p2];
//         glm::vec3 dp= p1- p2;
//         float dist= glm::length(dp);
//         if(dist> s.rest_length){
//             float diff= (dist- s.rest_length)*0.5f;
//             dp= glm::normalize(dp)* diff;
//
//             X[s.p1]-= dp;
//             X[s.p2]+= dp;
//         }
//     }
// }
