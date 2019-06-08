#pragma once

#define _USE_MATH_DEFINES

#include <memory>
#include <glm/glm.hpp>
#include <cmath>
#include <math.h>
#include <iostream>

#include "../engine/PhysicsObject.h"
#include "../engine/ColliderSphere.h"
#include "../Pathing.h"
#include "../Shape.h"
#include "../WindowManager.h"
#include "../engine/Time.h"

using namespace glm;
using namespace std;

class Enemy : public PhysicsObject
{
public:
    Enemy(vector<vec3> enemyPath, quat orientation, shared_ptr<Shape> model,
          shared_ptr<Shape> legmodel, shared_ptr<Shape> footmodel, float radius);
    void init(WindowManager *windowManager);
    void update();
    void draw(shared_ptr<Program> prog, shared_ptr<MatrixStack> M);
    MatrixStack setlLeg(MatrixStack uLeg, float offset, float t);
    MatrixStack setFoot(MatrixStack lLeg, float offset, float t);
    Pathing *curvePath;
    WindowManager *windowManager;
    float moveSpeed;
    float radius;
    float t = 0.1f;
    float targetX;
    float targetZ;
    float targetY;
    bool forward;
    bool pointReached;
    vec3 direction;
    shared_ptr<Shape> legModel;
    shared_ptr<Shape> footModel;
    //GameObject uleg1, uleg2, uleg3, uleg4, lleg1, lleg2, lleg3, lleg4, foot1, foot2, foot3, foot4;
};