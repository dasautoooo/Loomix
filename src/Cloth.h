//
// Created by Leonard Chan on 3/9/25.
//

#ifndef CLOTH_H
#define CLOTH_H

#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

// Spring struct
struct Spring {
    int p1, p2;           // Indices of connected particles
    float rest_length;
    float Ks, Kd;
    int type;             // 0=structural, 1=shear, 2=bend
};

class Cloth {
public:
    Cloth();
    // Initialize the cloth with desired resolution and spacing
    Cloth(int numX, int numY, float spacing);

    ~Cloth();

    // Initialize cloth data structures (positions, springs, etc.)
    void init(int numX, int numY, float spacing);

    // Step the cloth simulation by dt
    void update(float dt);

    // Render the cloth (immediate mode for debugging)
    void render();

    // Access for changing parameters
    void setDamping(float d)             { defaultDamping = d; }
    void setGravity(const glm::vec3& g)  { gravity = g; }
    void setMass(float m)                { mass = m; }

    // Collision toggles
    void enableEllipsoidCollision(bool enable) { isEllipsoidCollisionOn = enable; }
    void enableProvotInverse(bool enable)      { useProvotInverse = enable; }

    // Pin the four corners of the cloth
    void pinCorners();

    const std::vector<glm::vec3>& getPositions() const { return X; }
    // (Optionally) if you want the total grid dimension
    int getClothWidth()  const { return numX+1; }  // e.g. # of points along X
    int getClothHeight() const { return numY+1; }  // e.g. # of points along Y

private:
    // Helper methods
    void addSpring(int a, int b, float Ks, float Kd, int type);
    void computeForces(float dt);
    void integrateVerlet(float dt);
    void ellipsoidCollision();
    void applyProvotInverse();

private:
    // Grid resolution
    int numX, numY;
    int total_points;

    // Cloth geometry
    float spacing;
    float mass;          // each node's mass
    glm::vec3 gravity;
    float defaultDamping;
    std::vector<bool> pinned;

    // State arrays
    std::vector<glm::vec3> X;        // current positions
    std::vector<glm::vec3> X_last;   // previous positions
    std::vector<glm::vec3> F;        // forces array

    // Springs
    std::vector<Spring> springs;

    // For optional triangles or rendering
    std::vector<GLushort> indices;

    // Optional collision & constraints
    bool isEllipsoidCollisionOn;
    bool useProvotInverse;

    // For ellipsoid
    glm::mat4 ellipsoid;         // transform
    glm::mat4 inverse_ellipsoid; // inverse
    glm::vec3 center;            // ellipsoid center in object space
    float radius;                // radius
};

#endif //CLOTH_H
