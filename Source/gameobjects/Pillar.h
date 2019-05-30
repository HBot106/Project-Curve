#pragma once

#include "../engine/PhysicsObject.h"
#include <glm/glm.hpp>
#include <memory>
#include "../Shape.h"

class Pillar: public PhysicsObject
{
public:
	Pillar(glm::vec3 position, glm::quat orientation, std::shared_ptr<Shape> model);
};