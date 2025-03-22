//
// Created by Leonard Chan on 3/9/25.
//

#include "Cloth.h"

Cloth::Cloth(int width, int height, float spacing, float massPerParticle)
        : width(width), height(height), spacing(spacing) {
    // Create particles in a grid
    initParticles(massPerParticle);

    createSprings();
}

void Cloth::pinCorners()  {
    // top-left
    int tl = 0;
    // top-right
    int tr = width - 1;
    // bottom-left
    int bl = (height - 1) * width;
    // bottom-right
    int br = (height - 1) * width + (width - 1);

    particles[tl].pinned = true;
    particles[tr].pinned = true;
    particles[bl].pinned = true;
    particles[br].pinned = true;
}

void Cloth::update(float dt, const glm::vec3 &gravity)  {

    const float MAX_DT = 0.001f; // 1ms max timestep
    dt = std::min(dt, MAX_DT);

    // 1) Compute forces on each particle
    std::vector<glm::vec3> forces(particles.size(), glm::vec3(0.0f));

    // Gravity
    for (size_t i = 0; i < particles.size(); i++) {
        if (!particles[i].pinned) {
            forces[i] += particles[i].mass * gravity;
        }
    }

    // Springs
    for (auto& s : springs) {
        auto& pA = particles[s.p1];
        auto& pB = particles[s.p2];

        glm::vec3 delta = pB.position - pA.position;
        float dist = glm::length(delta);
        if (dist < 0.0001f) continue; // avoid divide by zero with a small epsilon

        glm::vec3 dir = delta / dist;  // unit direction from A to B
        float stretch = dist - s.restLength;

        // Calculate spring force but with a force limiter to prevent extreme forces
        const float MAX_FORCE_MAGNITUDE = 50.0f; // Limit maximum force
        float forceMagnitude = -s.stiffness * stretch;

        // Clamp the force to prevent explosions
        forceMagnitude = std::max(-MAX_FORCE_MAGNITUDE, std::min(MAX_FORCE_MAGNITUDE, forceMagnitude));

        // Hooke's law: F = -k * (dist - restLength) in direction
        glm::vec3 fs = forceMagnitude * dir;

        // Damping: proportional to relative velocity
        glm::vec3 dv = pB.velocity - pA.velocity;
        // Project the velocity difference onto the spring direction
        float relVelInDir = glm::dot(dv, dir);
        glm::vec3 fd = -s.damping * relVelInDir * dir;

        // Net spring force
        glm::vec3 f = fs + fd;

        if (!pA.pinned) forces[s.p1] += f;
        if (!pB.pinned) forces[s.p2] -= f;  // opposite on B
    }

    // 2) Integrate (simple Euler)
    for (size_t i = 0; i < particles.size(); i++) {
        auto& p = particles[i];
        if (p.pinned) {
            // pinned => no movement
            p.velocity = glm::vec3(0.0f);
            continue;
        }

        glm::vec3 accel = forces[i] / p.mass;
        // Velocity
        p.velocity += accel * dt;
        // Position
        p.position += p.velocity * dt;
    }

    // 3) Apply constraint enforcement to prevent excessive stretching
    // This is important for stability
    for (int iter = 0; iter < 2; iter++) { // Multiple iterations for better convergence
        for (auto &s: springs) {
            if (particles[s.p1].pinned && particles[s.p2].pinned)
                continue;

            glm::vec3 delta = particles[s.p2].position - particles[s.p1].position;
            float currentLength = glm::length(delta);

            if (currentLength < 0.0001f)
                continue; // Avoid division by zero

            // The maximum allowed stretch factor (1.1 = 10% stretch)
            const float MAX_STRETCH = 1.1f;

            if (currentLength > s.restLength * MAX_STRETCH) {
                glm::vec3 dir = delta / currentLength;
                float correction = currentLength - (s.restLength * MAX_STRETCH);

                // Apply position correction
                if (particles[s.p1].pinned) {
                    particles[s.p2].position -= dir * correction;
                } else if (particles[s.p2].pinned) {
                    particles[s.p1].position += dir * correction;
                } else {
                    // Distribute correction between both particles
                    particles[s.p1].position += dir * (correction * 0.5f);
                    particles[s.p2].position -= dir * (correction * 0.5f);
                }
            }
        }
    }
}

void Cloth::setStiffness(float k) {
    // Iterate over all springs and set the new stiffness
    for (auto& s : springs) {
        s.stiffness = k;
    }
}

void Cloth::setDamping(float c){
    // Iterate over all springs and set the new damping
    for (auto& s : springs) {
        s.damping = c;
    }
}

void Cloth::setMass(float massPerParticle) {
    // Iterate over all particles and set the new mass
    for (auto& p : particles) {
        p.mass = massPerParticle;
    }
}

int Cloth::getClothWidth() const{
    return width;
}

int Cloth::getClothHeight() const{
    return height;
}

void Cloth::initParticles(float massPerParticle)  {
    particles.reserve(width * height);
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // create a particle at (x * spacing, 0, y * spacing)
            glm::vec3 pos(x * spacing, 0.0f, -y * spacing);
            particles.emplace_back(pos, massPerParticle);
        }
    }
}

void Cloth::createSprings()  {
    // constants for demonstration
    float k_struct = 200.0f;
    float c_struct = 0.5f;
    float k_shear  = 120.0f;
    float c_shear  = 0.5f;
    float k_bend   = 100.0f;
    float c_bend   = 0.5f;

    // structural: horizontal + vertical
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;

            // horizontal neighbor
            if (x + 1 < width) {
                int neighbor = y * width + (x + 1);
                addSpring(idx, neighbor, k_struct, c_struct);
            }
            // vertical neighbor
            if (y + 1 < height) {
                int neighbor = (y + 1) * width + x;
                addSpring(idx, neighbor, k_struct, c_struct);
            }
        }
    }

    // shear: diagonal neighbors
    for (int y = 0; y < height - 1; y++) {
        for (int x = 0; x < width - 1; x++) {
            int p00 = y * width + x;
            int p10 = y * width + (x + 1);
            int p01 = (y + 1) * width + x;
            int p11 = (y + 1) * width + (x + 1);

            addSpring(p00, p11, k_shear, c_shear);
            addSpring(p10, p01, k_shear, c_shear);
        }
    }

    // bend: connect two edges away
    // (bending springs for cloth wrinkling)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = y * width + x;

            // horizontal bend
            if (x + 2 < width) {
                int neighbor = y * width + (x + 2);
                addSpring(idx, neighbor, k_bend, c_bend);
            }
            // vertical bend
            if (y + 2 < height) {
                int neighbor = (y + 2) * width + x;
                addSpring(idx, neighbor, k_bend, c_bend);
            }
        }
    }
}

void Cloth::addSpring(int i1, int i2, float k, float c)  {
    glm::vec3 diff = particles[i2].position - particles[i1].position;
    float length = glm::length(diff);
    Spring s(i1, i2, length, k, c);
    springs.push_back(s);
}
