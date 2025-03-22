//
// Created by Leonard Chan on 3/9/25.
//

#ifndef CLOTH_H
#define CLOTH_H

#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>

// Simple struct for a particle
struct Particle {
    glm::vec3 position;
    glm::vec3 velocity;
    float mass;
    bool pinned;

    Particle(const glm::vec3& pos, float m)
        : position(pos), velocity(0.0f), mass(m), pinned(false) {}
};

// Struct for a spring connecting two particles
struct Spring {
    int p1, p2;         // indices of the connected particles
    float restLength;   // rest length (distance at rest)
    float stiffness;    // Hooke's law constant, k
    float damping;      // velocity damping factor

    Spring(int i1, int i2, float length, float k, float c)
        : p1(i1), p2(i2), restLength(length), stiffness(k), damping(c) {}
};

// The cloth system
class Cloth {
public:
    Cloth(int width, int height, float spacing, float massPerParticle);

    void pinCorners();

    // Update the cloth system by dt (Euler method)
    void update(float dt, const glm::vec3& gravity);

    // Access to particles for rendering
    const std::vector<Particle>& getParticles() const { return particles; }

    void setStiffness(float k);       // Adjust all spring stiffness
    void setDamping(float c);         // Adjust all spring damping
    void setMass(float massPerParticle);

    int getClothWidth() const;
    int getClothHeight() const;

private:
    int width, height;
    float spacing;

    std::vector<Particle> particles;
    std::vector<Spring> springs;

    // Initialize particles in a 2D grid
    void initParticles(float massPerParticle);

    // Create springs: structural + shear (+ optionally bend)
    void createSprings();

    // Add a spring between i1 and i2
    void addSpring(int i1, int i2, float k, float c);
};


#endif //CLOTH_H
