#define _USE_MATH_DEFINES

#include <cmath>
#include <math.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <glad/glad.h>
#include <cmath>
#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <string>
#include <irrKlang.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Camera.h"
#include "GLSL.h"
#include "GLTextureWriter.h"
#include "MatrixStack.h"
#include "Object3D.h"
#include "Program.h"
#include "effects/Sound.h"
#include "Shape.h"
#include "Skybox.h"
#include "WindowManager.h"

#include "engine/Time.h"
#include "effects/ParticleSpark.h"
#include "gameobjects/Ball.h"
#include "gameobjects/Goal.h"
#include "gameobjects/Enemy.h"
#include "gameobjects/PowerUp.h"
#include "engine/Collider.h"
#include "engine/ColliderSphere.h"
#include "engine/GameObject.h"
#include "engine/Octree.h"
#include "engine/Frustum.h"
#include "engine/ParticleEmitter.h"
#include "engine/Prefab.h"
#include "engine/SceneManager.h"
#include "engine/ModelManager.h"
#include "engine/TextureManager.h"
#include "engine/SkyboxManager.h"
#include "engine/ShaderManager.h"
#include "engine/MaterialManager.h"
#include "engine/PrefabManager.h"
#include "engine/EmitterManager.h"
#include "engine/Preferences.h"

#define RESOURCE_DIRECTORY string("../Resources")

using namespace std;
using namespace glm;

TimeData Time;
shared_ptr<Sound> soundEngine;

class Application : public EventCallbacks
{
public:
    Preferences preferences = Preferences(RESOURCE_DIRECTORY + "/preferences.yaml");
    WindowManager *windowManager = new WindowManager();
    ModelManager modelManager = ModelManager(RESOURCE_DIRECTORY + "/models/");
    TextureManager textureManager = TextureManager(RESOURCE_DIRECTORY + "/textures/");
    SkyboxManager skyboxManager = SkyboxManager(RESOURCE_DIRECTORY + "/skyboxes/");
    ShaderManager shaderManager = ShaderManager(RESOURCE_DIRECTORY + "/shaders/");
    MaterialManager materialManager = MaterialManager();
    EmitterManager emitterManager = EmitterManager();
    PrefabManager prefabManager = PrefabManager(RESOURCE_DIRECTORY + "/prefabs/", shared_ptr<ModelManager>(&modelManager), shared_ptr<MaterialManager>(&materialManager));
    SceneManager sceneManager = SceneManager(shared_ptr<PrefabManager>(&prefabManager));

    // Game Info Globals
    bool editMode = false;
    float startTime = 0.0f;

    bool debugLight = 0;
    bool debugGeometry = 1;
    GLuint depthMapFBO = 0;
    GLuint depthMap = 0;
    
    // Camera
    shared_ptr<Camera> camera;
    Frustum viewFrustum;
    
    struct
    {
        shared_ptr<Ball> marble;
        shared_ptr<Enemy> enemy1;
        shared_ptr<Enemy> enemy2;
        shared_ptr<Enemy> sentry1;
        shared_ptr<Enemy> sentry2;
        shared_ptr<Goal> goal;
        shared_ptr<PowerUp> powerUp1;
        shared_ptr<PowerUp> powerUp2;
    } gameObjects;

    // Billboard for rendering a texture to screen (like the shadow map)
    GLuint fboQuadVertexArrayID;
    GLuint fboQuadVertexBuffer;

    /*
     * Loading
    */
    void loadWindow()
    {
        windowManager->init(preferences.window.resolutionX, preferences.window.resolutionY, preferences.window.maximized, preferences.window.fullscreen);
        windowManager->setEventCallbacks(this);
    }

    void loadCanvas()
    {
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        GLSL::checkVersion();

        // Enable z-buffer test.
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);

        // Initialize camera
        camera = make_shared<Camera>(windowManager, vec3(0, 0, 0));
        camera->init();
    }

    void loadSounds()
    {
        soundEngine = make_shared<Sound>();

        if (preferences.sound.music) soundEngine->playPauseMusic();

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    void loadShaders()
    {
        vector<string> pbrUniforms = { "P", "V", "M", "shadowResolution", "shadowAA", "shadowDepth", "LS", "albedoMap", "roughnessMap", "metallicMap", "aoMap", "lightPosition", "lightColor", "viewPos" };
        shared_ptr<Program> pbr = shaderManager.get("pbr", { "vertPos", "vertNor", "vertTex" }, pbrUniforms);
        materialManager.init(pbr, shared_ptr<TextureManager>(&textureManager));

        shaderManager.get("sky", {"vertPos"}, {"P", "V", "Texture0"});
        shaderManager.get("circle", {"vertPos"}, {"P", "V", "M", "radius"});
        shaderManager.get("cube_outline", {"vertPos"}, {"P", "V", "M", "edge"});
        shaderManager.get("depth", {"vertPos"}, {"LP", "LV", "M"});
        shaderManager.get("pass", {"vertPos"}, {"texBuf"});
        shaderManager.get("depth_debug", {"vertPos"}, {"LP", "LV", "M"});
        shaderManager.get("particle", {"vertPos", "vertTex"}, {"P", "V", "M", "pColor", "alphaTexture"});
    }

    void loadMaterials()
    {
        materialManager.get("marble_tiles", "png");
        materialManager.get("coal_matte_tiles", "png");
        materialManager.get("brown_rock", "png");
        materialManager.get("seaside_rocks", "png");
        materialManager.get("coal_matte_tiles", "png");
        materialManager.get("marble_tiles", "png");
        materialManager.get("rusted_metal", "jpg");
        materialManager.get("painted_metal", "png");
    }

    void loadSkybox()
    {
        skyboxManager.get("desert_hill", 1);
    }

    void loadParticleTextures()
    {
        textureManager.get("particles/star_07.png", 1);
        textureManager.get("particles/scorch_02.png", 1);
    }

    void loadShadows()
    {
        // Generate the FBO for the shadow depth
        glGenFramebuffers(1, &depthMapFBO);

        // Generate the texture
        glGenTextures(1, &depthMap);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, preferences.shadows.resolution, preferences.shadows.resolution, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

        // Bind with framebuffer's depth buffer
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void loadEffects() {
        emitterManager.get("sparks", modelManager.get("billboard.obj"), textureManager.get("particles/star_07.png"), 100);
        emitterManager.get("fireworks", modelManager.get("billboard.obj"), textureManager.get("particles/scorch_02.png"), 100);
    }

    void loadFBOQuad() {
        glGenVertexArrays(1, &fboQuadVertexArrayID);
        glBindVertexArray(fboQuadVertexArrayID);

        static const GLfloat g_quad_vertex_buffer_data[] = {
            -1.0f, -1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
            -1.0f, 1.0f, 0.0f,

            -1.0f, 1.0f, 0.0f,
            1.0f, -1.0f, 0.0f,
            1.0f, 1.0f, 0.0f,
        };

        glGenBuffers(1, &fboQuadVertexBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, fboQuadVertexBuffer);
        glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);
    }

    void loadModels()
    {
        modelManager.get("cube.obj", false, true);
        modelManager.get("Robot/RobotHead.obj", true);
        modelManager.get("Robot/RobotLeg.obj", true);
        modelManager.get("Robot/RobotFoot.obj", true);
        modelManager.get("billboard.obj", true);
        modelManager.get("goal.obj", true);
        modelManager.get("quadSphere.obj", true);
        modelManager.get("bunny.obj", true);
    }

    void loadLevel() {
        sceneManager.load(RESOURCE_DIRECTORY + "/levels/" + preferences.scenes.list[preferences.scenes.startup] + ".yaml");
    }

    void loadGameObjects()
    {
        // Marble
        gameObjects.marble = make_shared<Ball>(sceneManager.marbleStart, quat(1, 0, 0, 0), modelManager.get("quadSphere.obj"), 1);
        gameObjects.marble->init(windowManager, emitterManager.get("sparks"));
        sceneManager.octree.insert(gameObjects.marble);
        gameObjects.marble->addSkin(materialManager.get("brown_rock", "png"));
        gameObjects.marble->addSkin(materialManager.get("seaside_rocks", "png"));
        gameObjects.marble->addSkin(materialManager.get("coal_matte_tiles", "png"));
        gameObjects.marble->addSkin(materialManager.get("marble_tiles", "png"));

        if (preferences.scenes.startup == 0) {
            // Enemy 1
            vector<glm::vec3> enemyPath = {
                vec3{95.0, 2.0, 7.0},
                vec3{100.0, 2.0, 15.0},
                vec3{110.0, 2.0, -1.0},
                vec3{115.0, 2.0, 7.0} };
            gameObjects.enemy1 = make_shared<Enemy>(enemyPath, quat(1, 0, 0, 0), modelManager.get("Robot/RobotHead.obj"), modelManager.get("Robot/RobotLeg.obj"), modelManager.get("Robot/RobotFoot.obj"), 1.75f);
            gameObjects.enemy1->init(windowManager);
            sceneManager.octree.insert(gameObjects.enemy1);

            // Enemy 2
            enemyPath = {
                vec3{125.0, 8.0, 55.0},
                vec3{115.0, 20.0, 55.0},
                vec3{105.0, 5.0, 55.0},
                vec3{95.0, 8.0, 55.0} };
            gameObjects.enemy2 = make_shared<Enemy>(enemyPath, quat(1, 0, 0, 0), modelManager.get("Robot/RobotHead.obj"), modelManager.get("Robot/RobotLeg.obj"), modelManager.get("Robot/RobotFoot.obj"), 1.75f);
            gameObjects.enemy2->init(windowManager);
            sceneManager.octree.insert(gameObjects.enemy2);
        }

        // Goal functionality
        gameObjects.goal = make_shared<Goal>(preferences.scenes.startup == 0 ? vec3(0, 11.5, 0) : vec3(-4 * 8, 3 * 8, 5.4 * 8) + vec3(0, 1, 0), quat(1, 0, 0, 0), nullptr, 1.50f);
        gameObjects.goal->init(emitterManager.get("fireworks"), &startTime);
        sceneManager.octree.insert(gameObjects.goal);

        // Sentry 1
        enemyPath = { vec3{65.0, 7.0, 32.0} };
        gameObjects.sentry1 = make_shared<Enemy>(enemyPath, quat(1, 0, 0, 0), modelManager.get("Robot/RobotHead.obj"), modelManager.get("Robot/RobotLeg.obj"), modelManager.get("Robot/RobotFoot.obj"), 1.75f);
        gameObjects.sentry1->init(windowManager);
        sceneManager.octree.insert(gameObjects.sentry1);

        // Sentry 2
        enemyPath = { vec3{90.0, 2.0, 32.0} };
        gameObjects.sentry2 = make_shared<Enemy>(enemyPath, quat(1, 0, 0, 0), modelManager.get("Robot/RobotHead.obj"), modelManager.get("Robot/RobotLeg.obj"), modelManager.get("Robot/RobotFoot.obj"), 1.75f);
        gameObjects.sentry2->init(windowManager);
        sceneManager.octree.insert(gameObjects.sentry2);

        //Power Up 1
        gameObjects.powerUp1 = make_shared<PowerUp>(vec3(120, 2, 30), 0, quat(1, 0, 0, 0), modelManager.get("bunny.obj", true), 1, 1);
        gameObjects.powerUp1->init();
        sceneManager.octree.insert(gameObjects.powerUp1);


        // Power Up 2
        gameObjects.powerUp2 = make_shared<PowerUp>(vec3(80, 2, 7.0), 0, quat(1, 0, 0, 0), modelManager.get("bunny.obj", true), 1, 1);
        gameObjects.powerUp2->init();
        sceneManager.octree.insert(gameObjects.powerUp2);

        sceneManager.octree.init(modelManager.get("billboard.obj"), modelManager.get("cube.obj"));
    }

    /*
     * Rendering
     */
    void render()
    {
        // Get current frame buffer size.
        int width;
        int height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);

        mat4 LS;

        if (preferences.shadows.resolution > 0) drawShadowMap(&LS);

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        if (debugLight) drawDepthMap();
        else renderPlayerView(&LS);
    }

    void drawScene(shared_ptr<Program> shader)
    {
        // Create the model transformation matrix stack
        auto M = make_shared<MatrixStack>();
        M->pushMatrix();
        M->loadIdentity();

        shared_ptr<Program> pbr = shaderManager.get("pbr");

        if (shader == pbr)
        {
            glUniform1f(shader->getUniform("shadowResolution"), (float)preferences.shadows.resolution);
            glUniform1f(shader->getUniform("shadowAA"), (float)preferences.shadows.samples);

            glUniform3fv(shader->getUniform("viewPos"), 1, value_ptr(camera->eye));
        }

        // Draw marble
        if (shader == pbr) gameObjects.marble->getSkinMaterial()->bind();
        gameObjects.marble->draw(shader, M);

        // Draw enemies
        if (preferences.scenes.startup == 0) {
            if (shader == pbr) materialManager.get("rusted_metal", "jpg")->bind();
            gameObjects.enemy1->draw(shader, M);
            gameObjects.enemy2->draw(shader, M);
            gameObjects.sentry1->draw(shader, M);
            gameObjects.sentry2->draw(shader, M);
            if (!gameObjects.powerUp1->destroyed){
                gameObjects.powerUp1->draw(shader,M);
            }
            if (!gameObjects.powerUp2->destroyed){
                gameObjects.powerUp2->draw(shader,M);
            }
        }

        // Draw scene instances
        for (shared_ptr<Instance> instance : sceneManager.scene)
        {
            if (shader == pbr) instance->material->bind();
            instance->physicsObject->draw(shader, M);
        }

        // Cleanup
        M->popMatrix();
    }

    void drawShadowMap(mat4 *LS)
    {
        GameObject::setCulling(false);
        
        // set up light's depth map
        glViewport(0, 0, preferences.shadows.resolution, preferences.shadows.resolution); // shadow map width and height
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        glCullFace(GL_FRONT);

        shared_ptr<Program> depth = shaderManager.get("depth");

        depth->bind();
        // TODO you will need to fix these
        mat4 LP = SetOrthoMatrix(depth);
        mat4 LV = SetLightView(depth, sceneManager.light.direction, vec3(60, 0, 0), vec3(0, 1, 0));
        *LS = LP * LV;
        drawScene(depth);
        depth->unbind();
        glCullFace(GL_BACK);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void drawDepthMap()
    {
        // Code to draw the light depth buffer

        GameObject::setCulling(true);

        // geometry style debug on light - test transforms, draw geometry from light perspective
        if (debugGeometry)
        {
            shared_ptr<Program> depthDebug = shaderManager.get("depth_debug");

            depthDebug->bind();
            // render scene from light's point of view
            SetOrthoMatrix(depthDebug);
            SetLightView(depthDebug, sceneManager.light.direction, vec3(60, 0, 0), vec3(0, 1, 0));
            drawScene(depthDebug);
            depthDebug->unbind();
        }
        else
        {
            shared_ptr<Program> debug = shaderManager.get("debug");

            // actually draw the light depth map
            debug->bind();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, depthMap);
            glUniform1i(debug->getUniform("texBuf"), 0);
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, fboQuadVertexBuffer);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
            glDrawArrays(GL_TRIANGLES, 0, 6);
            glDisableVertexAttribArray(0);
            debug->unbind();
        }
    }

    void drawSkybox()
    {
        shared_ptr<Program> sky = shaderManager.get("sky");

        sky->bind();
        setProjectionMatrix(sky);
        setView(sky);

        skyboxManager.get("desert_hill", 1)->bind(sky->getUniform("Texture0"));
        glDepthMask(GL_FALSE);
        glDisable(GL_CULL_FACE);
        modelManager.get("cube.obj")->draw(sky);
        glEnable(GL_CULL_FACE);
        glDepthMask(GL_TRUE);

        sky->unbind();
    }

    void drawOctree() {
        shared_ptr<Program> circle = shaderManager.get("circle");

        circle->bind();
        setProjectionMatrix(circle);
        setView(circle);
        sceneManager.octree.drawDebugBoundingSpheres(circle);
        circle->unbind();

        shared_ptr<Program> cubeOutline = shaderManager.get("cube_outline");

        cubeOutline->bind();
        setProjectionMatrix(cubeOutline);
        setView(cubeOutline);
        sceneManager.octree.drawDebugOctants(cubeOutline);
        cubeOutline->unbind();
    }

    void renderPlayerView(mat4 *LS)
    {
        GameObject::setCulling(true);

        drawSkybox();

        shared_ptr<Program> pbr = shaderManager.get("pbr");
        pbr->bind();

        setLight(pbr);
        setProjectionMatrix(pbr);
        setView(pbr);

        // Send shadow map
        glActiveTexture(GL_TEXTURE30);
        glBindTexture(GL_TEXTURE_2D, depthMap);
        glUniform1i(shaderManager.get("pbr")->getUniform("shadowDepth"), 0);

        glUniformMatrix4fv(pbr->getUniform("LS"), 1, GL_FALSE, value_ptr(*LS));

        drawScene(pbr);

        pbr->unbind();

        if (sceneManager.octree.debug) drawOctree();

        shared_ptr<Program> particle = shaderManager.get("particle");
        particle->bind();
        setProjectionMatrix(particle);
        setView(particle);
        for (shared_ptr<ParticleEmitter> emitter : emitterManager.list()) {
            emitter->draw(particle);
        }
        particle->unbind();
    }

    /*
     * Render loop calls
     */
    void resetPlayer()
    {
        soundEngine->reset();

        gameObjects.marble->position = sceneManager.marbleStart;
        gameObjects.marble->setVelocity(vec3(0.0f));
        startTime = glfwGetTime();
        gameObjects.goal->reset();
        gameObjects.marble->frozen = 0;
    }

    void beforePhysics()
    {
        gameObjects.marble->update(camera->dolly, camera->strafe);
        gameObjects.goal->update();
        if (preferences.scenes.startup == 0) {
            gameObjects.enemy1->update(gameObjects.marble->position);
            gameObjects.enemy2->update(gameObjects.marble->position);
            gameObjects.sentry1->update(gameObjects.marble->position);
            gameObjects.sentry2->update(gameObjects.marble->position);

            if (!gameObjects.powerUp1->destroyed){
                gameObjects.powerUp1->update();
            }
            
            if (!gameObjects.powerUp2->destroyed){
                gameObjects.powerUp2->update();
            }
        }
    }

    void physicsTick()
    {
        sceneManager.octree.update();

        vector<shared_ptr<PhysicsObject>> instancesToCheck = sceneManager.octree.query(gameObjects.marble);
        for (shared_ptr<PhysicsObject> object : instancesToCheck)
        {
            if (object->collidable)
                object->checkCollision(gameObjects.marble.get());
        }

        for (shared_ptr<Instance> instance : sceneManager.scene)
        {
            instance->physicsObject->update();
        }
    }

    void beforeRender()
    {
        if (gameObjects.marble->position.y < sceneManager.deathBelow) resetPlayer();

        camera->update(gameObjects.marble);

        emitterManager.get("sparks")->update();
        emitterManager.get("fireworks")->update();

        viewFrustum.extractPlanes(setProjectionMatrix(nullptr), setView(nullptr));
        sceneManager.octree.markInView(viewFrustum);
    }

    /*
     * General
     */
    void setLight(shared_ptr<Program> prog)
    {
        glUniform3f(prog->getUniform("lightPosition"), sceneManager.light.direction.x, sceneManager.light.direction.y, sceneManager.light.direction.z);
        glUniform3f(prog->getUniform("lightColor"), sceneManager.light.brightness.x, sceneManager.light.brightness.y, sceneManager.light.brightness.z);
    }

    mat4 SetOrthoMatrix(shared_ptr<Program> curShade)
    {
        // shadow mapping helper

        mat4 ortho = glm::ortho(-96.0f, 96.0f, -96.0f, 96.0f, 0.1f, 500.0f);
        // fill in the glUniform call to send to the right shader!
        glUniformMatrix4fv(curShade->getUniform("LP"), 1, GL_FALSE, value_ptr(ortho));
        return ortho;
    }

    mat4 SetLightView(shared_ptr<Program> curShade, vec3 pos, vec3 LA, vec3 up)
    {
        // shadow mapping helper

        mat4 Cam = lookAt(pos, LA, up);
        // fill in the glUniform call to send to the right shader!
        glUniformMatrix4fv(curShade->getUniform("LV"), 1, GL_FALSE, value_ptr(Cam));
        return Cam;
    }

    mat4 setProjectionMatrix(shared_ptr<Program> curShade)
    {
        int width, height;
        glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
        float aspect = width / (float)height;
        mat4 Projection = perspective(radians(50.0f), aspect, 0.1f, 200.0f);
        if (curShade != nullptr)
        {
            glUniformMatrix4fv(curShade->getUniform("P"), 1, GL_FALSE, value_ptr(Projection));
        }
        return Projection;
    }

    mat4 setView(shared_ptr<Program> curShade)
    {
        mat4 Cam = lookAt(camera->eye, camera->lookAtPoint, camera->upVec);
        if (curShade != nullptr)
        {
            glUniformMatrix4fv(curShade->getUniform("V"), 1, GL_FALSE, value_ptr(Cam));
        }
        return Cam;
    }

    /*
     * User input
     */
    void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
    {
        if (key == GLFW_KEY_O && action == GLFW_PRESS)
        {
            gameObjects.marble->nextSkin();
        }
        else if (key == GLFW_KEY_P && action == GLFW_PRESS)
        {
            soundEngine->playPauseMusic();
        }
        else if (key == GLFW_KEY_L && action == GLFW_PRESS)
        {
            soundEngine->nextTrackMusic();
        }
        // other call backs
        else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        {
            glfwSetInputMode(windowManager->getHandle(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            glfwSetWindowShouldClose(window, GL_TRUE);
        }
        else if (key == GLFW_KEY_Z && action == GLFW_PRESS)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        }
        else if (key == GLFW_KEY_Z && action == GLFW_RELEASE)
        {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        else if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
        {
            editMode = !editMode;
            camera->cameraMode = editMode ? Camera::edit : Camera::marble;
            gameObjects.marble->frozen = editMode;
            if (editMode) {
                camera->saveMarbleView();
                gameObjects.marble->playPosition = gameObjects.marble->position;
                gameObjects.marble->position = gameObjects.marble->startPosition;
            }
            else {
                camera->restoreMarbleView();
                gameObjects.marble->position = gameObjects.marble->playPosition;
            }
        }
        else if (key == GLFW_KEY_U && action == GLFW_PRESS)
        {
            debugLight = !debugLight;
        }
        else if (key == GLFW_KEY_H && action == GLFW_PRESS)
        {
            sceneManager.octree.debug = !sceneManager.octree.debug;
        }
        else if (key == GLFW_KEY_R && action == GLFW_PRESS)
        {
            resetPlayer();
            gameObjects.powerUp1->destroyed = false;
            gameObjects.powerUp2->destroyed = false;
            gameObjects.powerUp1->collidable = true;
            gameObjects.powerUp2->collidable = true;
        }
        // else if (key == GLFW_KEY_C && action == GLFW_PRESS)
        // {
        //    camera->previewLvl = !camera->previewLvl;
        //    if (camera->previewLvl)
        //    {
        //        camera->startLvlPreview(CENTER_LVL_POSITION);
        //    }
        // }
        else if (key == GLFW_KEY_M && action == GLFW_PRESS)
        {   // just a test since super bounce has no trigger yet
            soundEngine->superBounce();
        }
    }

    void scrollCallback(GLFWwindow *window, double deltaX, double deltaY)
    {
        deltaY *= -1;
        float newDistance = deltaY + camera->distToBall;
        if (newDistance < 20 && newDistance > 2.5)
        {
            camera->distToBall += deltaY;
        }
    }

    void mouseCallback(GLFWwindow *window, int button, int action, int mods)
    {
        double posX, posY;

        if (action == GLFW_PRESS)
        {
            glfwGetCursorPos(window, &posX, &posY);
            cout << "Pos X " << posX << " Pos Y " << posY << endl;
            cout << "" << gameObjects.marble->position.x << ", " << gameObjects.marble->position.y << ", " << gameObjects.marble->position.z << endl;
        }

        if (action == GLFW_RELEASE)
        {
        }

        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        {
            camera->freeViewing = true;
        }
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
        {
            camera->freeViewing = false;
        }
    }

    void updateAudio()
    {
        soundEngine->updateMusic();
    }

    void resizeCallback(GLFWwindow *window, int width, int height)
    {
        glViewport(0, 0, width, height);
    }
};

int main(int argc, char **argv)
{
    Application *application = new Application();

    // Load game
    application->loadWindow();
    application->loadCanvas();
    application->loadShaders();
    application->loadMaterials();
    application->loadSkybox();
    application->loadParticleTextures();
    application->loadShadows();
    application->loadEffects();
    application->loadFBOQuad();
    application->loadModels();
    application->loadLevel();
    application->loadGameObjects();
    application->loadSounds();

    application->startTime = glfwGetTime();

    float startTimestamp = glfwGetTime();
    float lastFrameTimestamp = 0;
    float lastPhysicsTimestamp = 0;
    float rollingAverageFrameTime = 0;
    float accumulator = 0;
    Time.physicsDeltaTime = 0.02;

    // Loop until the user closes the window.
    while (!glfwWindowShouldClose(application->windowManager->getHandle()))
    {
        float t = glfwGetTime();
        Time.deltaTime = t - Time.timeSinceStart - startTimestamp;
        accumulator += Time.deltaTime;
        Time.timeSinceStart = t - startTimestamp;

        while (accumulator >= Time.physicsDeltaTime)
        {
            // Call gameobject physics update and ropagate physics
            application->beforePhysics();
            application->physicsTick();
            accumulator -= Time.physicsDeltaTime;
        }

        rollingAverageFrameTime = rollingAverageFrameTime * (20 - 1) / 20 + (Time.deltaTime / 20);
        // printf("FPS: %i\n", (int)(1.0 / rollingAverageFrameTime));

        // Get user input, call gameobject update, and render visuals
        glfwPollEvents();
        application->beforeRender();
        application->render();
        glfwSwapBuffers(application->windowManager->getHandle());

    }

    application->windowManager->shutdown();
    return 0;
}
