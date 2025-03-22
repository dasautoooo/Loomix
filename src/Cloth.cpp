//
// Created by Leonard Chan on 3/9/25.
//

#include "Cloth.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static const int STRUCTURAL=0;
static const int SHEAR=1;
static const int BEND=2;

Cloth::Cloth()
    : numX(0), numY(0), total_points(0),
      spacing(0.2f), mass(1.0f),
      gravity(0.f, -0.00981f, 0.f),
      defaultDamping(-0.0125f),
      isEllipsoidCollisionOn(false),
      useProvotInverse(false)
{
    ellipsoid = glm::translate(glm::mat4(1), glm::vec3(0,2,0));
    ellipsoid = glm::rotate(ellipsoid, glm::radians(45.0f), glm::vec3(1,0,0));
    ellipsoid = glm::scale(ellipsoid, glm::vec3(1.f,1.f,0.5f));
    inverse_ellipsoid = glm::inverse(ellipsoid);

    center= glm::vec3(0,0,0);
    radius=1.0f;
}

Cloth::Cloth(int Nx, int Ny, float sp)
    : Cloth() // call default constructor first
{
    init(Nx, Ny, sp);
}

Cloth::~Cloth()
{
    // no dynamic pointers, so it’s fine
}

void Cloth::init(int Nx, int Ny, float sp)
{
    this->numX= Nx;
    this->numY= Ny;
    this->spacing= sp;

    total_points= (numX+1)*(numY+1);

    pinned.resize(total_points, false);

    X.clear();
    X_last.clear();
    F.clear();
    springs.clear();
    indices.clear();

    X.resize(total_points);
    X_last.resize(total_points);
    F.resize(total_points, glm::vec3(0));

    // We can also fill a triangle index buffer if we want
    // or just do line rendering in "render()"

    // build grid positions
    int count=0;
    for(int y=0; y<= numY; y++){
        for(int x=0; x<= numX; x++){
            glm::vec3 P( x*spacing, 2.0f, -y*spacing );
            X[count] = P;
            X_last[count] = P;
            count++;
        }
    }

    // create structural, shear, bend springs
    // structural horizontally
    float KsStruct= 50.75f, KdStruct= -0.25f;
    float KsShear=  50.75f, KdShear=  -0.25f;
    float KsBend=   50.95f, KdBend=  -0.25f;

    for(int row=0; row<=Ny; row++){
        for(int col=0; col< numX; col++){
            int i1= row*(numX+1)+ col;
            int i2= i1+1;
            addSpring(i1, i2, KsStruct, KdStruct, STRUCTURAL);
        }
    }
    // structural vertically
    for(int col=0; col<= numX; col++){
        for(int row=0; row< numY; row++){
            int i1= row*(numX+1)+ col;
            int i2= (row+1)*(numX+1)+ col;
            addSpring(i1, i2, KsStruct, KdStruct, STRUCTURAL);
        }
    }

    // shear
    for(int row=0; row< numY; row++){
        for(int col=0; col< numX; col++){
            int i0= row*(numX+1)+ col;
            int i1= i0+1;
            int i2= i0+ (numX+1);
            int i3= i2+1;
            addSpring(i0, i3, KsShear, KdShear, SHEAR);
            addSpring(i1, i2, KsShear, KdShear, SHEAR);
        }
    }

    // bend
    for(int row=0; row<= numY; row++){
        for(int col=0; col< (numX-1); col++){
            int i1= row*(numX+1)+ col;
            int i2= i1+2;
            addSpring(i1, i2, KsBend, KdBend, BEND);
        }
    }
    for(int col=0; col<= numX; col++){
        for(int row=0; row< (numY-1); row++){
            int i1= row*(numX+1)+ col;
            int i2= (row+2)*(numX+1)+ col;
            addSpring(i1, i2, KsBend, KdBend, BEND);
        }
    }
}

//------------------------------------
// Simple function to pin corners
//------------------------------------
void Cloth::pinCorners()
{
    if (total_points == 0) return;

    // top-left => index 0
    int tl = 0;
    pinned[tl] = true;

    // top-right => index numX
    int tr = numX;
    pinned[tr] = true;

    // bottom-left => row = numY, col=0 => index = numY*(numX+1)
    int bl = numY * (numX + 1);
    pinned[bl] = true;

    // bottom-right => bottom-left + numX
    int br = bl + numX;
    pinned[br] = true;
}

//------------------------------------
// Add a spring
//------------------------------------
void Cloth::addSpring(int a, int b, float Ks, float Kd, int type)
{
    Spring s;
    s.p1= a;
    s.p2= b;
    s.type= type;
    s.Ks= Ks;
    s.Kd= Kd;
    // measure rest length from X
    glm::vec3 dp= X[a]- X[b];
    s.rest_length= glm::length(dp);
    springs.push_back(s);
}

//------------------------------------
// Update cloth by dt
//------------------------------------
void Cloth::update(float dt)
{
    // 1) compute forces
    computeForces(dt);
    // 2) integrate (Verlet)
    integrateVerlet(dt);
    // 3) collisions
    if(isEllipsoidCollisionOn){
        ellipsoidCollision();
    }
    // 4) optional provot
    if(useProvotInverse){
        applyProvotInverse();
    }
}

//------------------------------------
// Render cloth as immediate-mode lines
//------------------------------------
void Cloth::render()
{
    // We'll do structural + shear + bend lines
    // For each spring, draw a line from X[p1] to X[p2]
    glColor3f(1,1,1);
    glBegin(GL_LINES);
    for(auto &s : springs){
        glm::vec3 pA= X[s.p1];
        glm::vec3 pB= X[s.p2];
        glVertex3fv(glm::value_ptr(pA));
        glVertex3fv(glm::value_ptr(pB));
    }
    glEnd();

    // You can also draw points:
    glPointSize(5.f);
    glBegin(GL_POINTS);
    for(size_t i=0; i< X.size(); i++){
        glVertex3fv(glm::value_ptr(X[i]));
    }
    glEnd();
}

//------------------------------------
// Compute forces: gravity + damping + spring
//------------------------------------
void Cloth::computeForces(float dt)
{
    // zero out
    for(auto &f: F){
        f= glm::vec3(0.f);
    }

    // 1) gravity + global damping
    for(size_t i=0; i< X.size(); i++){
        glm::vec3 vel= (X[i] - X_last[i])/ dt;
        F[i]+= gravity* mass;  // gravity
        F[i]+= defaultDamping* vel; // global damping
    }

    // 2) spring forces
    for(auto &s: springs){
        int iA= s.p1;
        int iB= s.p2;

        glm::vec3 pA= X[iA];
        glm::vec3 pB= X[iB];
        glm::vec3 vA= (pA- X_last[iA])/ dt;
        glm::vec3 vB= (pB- X_last[iB])/ dt;

        glm::vec3 deltaP= pA- pB;
        glm::vec3 deltaV= vA- vB;
        float dist= glm::length(deltaP);
        if(dist < 1e-6f) continue;

        float leftTerm= -s.Ks*(dist- s.rest_length);
        float rightTerm= s.Kd* (glm::dot(deltaV, deltaP)/ dist);

        glm::vec3 force= (leftTerm+ rightTerm)* (deltaP/dist);

        F[iA]+= force;
        F[iB]-= force;
    }
}

//------------------------------------
// Integrate using classical position-based Verlet
//------------------------------------
void Cloth::integrateVerlet(float dt)
{
    float dt2mass = (dt * dt) / mass;
    for (size_t i = 0; i < X.size(); i++) {
        // if pinned, skip
        if (pinned[i]) {
            continue;
        }
        // otherwise, standard Verlet
        glm::vec3 temp = X[i];
        X[i] = X[i] + (X[i] - X_last[i]) + F[i] * dt2mass;
        X_last[i] = temp;

        // e.g. floor collision
        if (X[i].y < 0.f) {
            X[i].y = 0.f;
        }
    }
}

//------------------------------------
// Ellipsoid collision
//------------------------------------
void Cloth::ellipsoidCollision()
{
    // e.g. from your code, we do a transform approach
    for(size_t i=0; i< X.size(); i++){
        glm::vec4 xp= inverse_ellipsoid* glm::vec4(X[i],1.f);
        glm::vec3 delta= glm::vec3(xp)- center;
        float dist= glm::length(delta);
        if(dist< radius){
            float diff= radius- dist;
            glm::vec3 fix= (diff* delta)/ dist;
            xp.x+= fix.x;
            xp.y+= fix.y;
            xp.z+= fix.z;

            // transform back
            glm::vec3 newPos= glm::vec3(ellipsoid* xp);
            X[i]= newPos;
            X_last[i]= newPos;
        }
    }
}

//------------------------------------
// Provot dynamic inverse
//------------------------------------
void Cloth::applyProvotInverse()
{
    for(auto &s: springs){
        glm::vec3 p1= X[s.p1];
        glm::vec3 p2= X[s.p2];
        glm::vec3 dp= p1- p2;
        float dist= glm::length(dp);
        if(dist> s.rest_length){
            float diff= (dist- s.rest_length)*0.5f;
            dp= glm::normalize(dp)* diff;

            X[s.p1]-= dp;
            X[s.p2]+= dp;
        }
    }
}