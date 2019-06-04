#pragma once

#include "../engine/PhysicsObject.h"
#include "../engine/ParticleEmitter.h"
#include "../Shape.h"
#include "../WindowManager.h"
#include "PowerUp.h"
#include <memory>
#include <glm/glm.hpp>
#include <vector>

class Ball : public PhysicsObject
{
public:
    Ball(glm::vec3 position, glm::quat orientation, std::shared_ptr<Shape> model, float radius);
    void init(WindowManager *windowManager, std::shared_ptr<ParticleEmitter> sparkEmitter);
    void update(float dt, glm::vec3 dolly, glm::vec3 strafe);
    void activatePowerUp();
    void prepNextPowerUp();
    virtual void onHardCollision(float impactVel, Collision &collision);

	WindowManager *windowManager;
    PowerUp *activePowerUp;
    std::shared_ptr<ParticleEmitter> sparkEmitter;
    float radius;
    float moveForce;
    float jumpForce;
    std::vector<PowerUp *> storedPowerUp;
    bool hasPowerUp;
    bool powerUpReady;
    bool frozen;
};