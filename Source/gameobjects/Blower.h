#pragma once

#include "../engine/PhysicsObject.h"
#include "../engine/TriggerSphere.h"
#include "Ball.h"

#include <glm/glm.hpp>
#include <memory>
#include <iostream>

using namespace std;
using namespace glm;

class Blower : public PhysicsObject
{
public:
    Blower(vec3 position, quat orientation, float radius, float length);
    void update(float dt);

    float radius;
    float length;
    float force;
};