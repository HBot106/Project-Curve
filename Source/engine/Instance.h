#pragma once

#include "Shape.h"
#include "Material.h"
#include "PhysicsObject.h"
#include "../gameobjects/Ball.h"
#include "../gameobjects/Enemy.h"
#include <vector>
#include <glm/glm.hpp>

using namespace std;
using namespace glm;

class Prefab;

class Instance {
public:
    Prefab& definition;
    shared_ptr<Material> material;
    shared_ptr<PhysicsObject> physicsObject;

    Instance(Prefab& definition);
};