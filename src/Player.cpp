#include "Player.h"

#include <cmath>
#include <algorithm>

using namespace std;
using namespace glm;

Player::Player(WindowManager *windowManager) : windowManager(windowManager)
{
}

Player::~Player()
{
}

void Player::update(float dt, vec3 ballPos)
{
    int width, height;
    glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);

    // Handle movement input
    float speed_ = speed;
    vec3 velocity = vec3(0, 0, 0);
    vec3 strafe = normalize(cross(lookAtPoint - eye, upVec));
    vec3 dolly = normalize(lookAtPoint - eye);
    if (flying)
    {
        dolly = normalize(lookAtPoint - eye);
    }
    else
    {
        eye = ballPos + vec3(0,10,20);
        lookAtPoint = ballPos;
        dolly = lookAtPoint - eye;
        dolly.y = 0;
        dolly = normalize(dolly); 
    }

    if (glfwGetKey(windowManager->getHandle(), GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS ||
        glfwGetKey(windowManager->getHandle(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    {
        speed_ *= sprintFactor;
    }
    if (glfwGetKey(windowManager->getHandle(), GLFW_KEY_W) == GLFW_PRESS)
    {
        velocity += dolly;
    }
    if (glfwGetKey(windowManager->getHandle(), GLFW_KEY_S) == GLFW_PRESS)
    {
        velocity -= dolly;
    }
    if (glfwGetKey(windowManager->getHandle(), GLFW_KEY_D) == GLFW_PRESS)
    {
        velocity += strafe;
    }
    if (glfwGetKey(windowManager->getHandle(), GLFW_KEY_A) == GLFW_PRESS)
    {
        velocity -= strafe;
    }
    if (glfwGetKey(windowManager->getHandle(), GLFW_KEY_E) == GLFW_PRESS)
    {
        velocity += vec3(0, 1, 0);
    }
    if (glfwGetKey(windowManager->getHandle(), GLFW_KEY_Q) == GLFW_PRESS)
    {
        velocity -= vec3(0, 1, 0);
    }
    if (length(velocity) != 0 && length(velocity) != NAN)
    {
        eye += normalize(velocity) * speed_ * (float) dt;
        eye.y = std::max(0.025f, eye.y);
    }
    if (flying) {
        // Mouse
        double xpos, ypos;
        glfwGetCursorPos(windowManager->getHandle(), &xpos, &ypos);
        double dx = xpos - prevXpos;
        double dy = -(ypos - prevYpos);
        prevXpos = xpos;
        prevYpos = ypos;
        double radPerPx = M_PI / height;
        yaw += dx * radPerPx;
        pitch = std::max(std::min(pitch + dy * radPerPx, radians(80.0)), -radians(80.0));
        lookAtPoint.x = eye.x + cos(pitch) * sin(yaw);
        lookAtPoint.y = eye.y + sin(pitch);
        lookAtPoint.z = eye.z + cos(pitch) * cos(M_PI - yaw);
    }
}

void Player::init()
{
    glfwGetCursorPos(windowManager->getHandle(), &prevXpos, &prevYpos);
    eye = vec3(0, this->height, 0);
    lookAtPoint = eye + vec3(1, 0, 0);
    upVec = vec3(0, 1, 0);
    pitch = 0;
    yaw = 0;
}