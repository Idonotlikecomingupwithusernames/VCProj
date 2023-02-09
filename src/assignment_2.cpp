#include <cstdlib>
#include <iostream>

#include "mygl/shader.h"
#include "mygl/model.h"
#include "mygl/camera.h"

#include "helicopter.h"

struct
{
    Camera camera;
    bool cameraFollowHeli;
    float zoomSpeedMultiplier;

    Helicopter heli;
    Model modelGround;

    ShaderProgram shaderColor;
} sScene;

struct
{
    bool mouseButtonPressed = false;
    Vector2D mousePressStart;
    bool keyPressed[Helicopter::eControl::CONTROL_COUNT] = {false, false, false, false, false, false, false, false};
} sInput;


void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    /* input for camera control */
    if(key == GLFW_KEY_0 && action == GLFW_PRESS)
    {
        sScene.cameraFollowHeli = false;
        sScene.camera.lookAt = {0.0f, 0.0f, 0.0f};
        cameraUpdateOrbit(sScene.camera, {0.0f, 0.0f}, 0.0f);
    }
    if(key == GLFW_KEY_1 && action == GLFW_PRESS)
    {
        sScene.cameraFollowHeli = false;
    }
    if(key == GLFW_KEY_2 && action == GLFW_PRESS)
    {
        sScene.cameraFollowHeli = true;
    }

    /* input for helicopter control */
    if(key == GLFW_KEY_W)
    {
        sInput.keyPressed[Helicopter::eControl::PITCH_DOWN] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
    if(key == GLFW_KEY_S)
    {
        sInput.keyPressed[Helicopter::eControl::PITCH_UP] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }

    if(key == GLFW_KEY_A)
    {
        sInput.keyPressed[Helicopter::eControl::ROLL_LEFT] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
    if(key == GLFW_KEY_D)
    {
        sInput.keyPressed[Helicopter::eControl::ROLL_RIGHT] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }

    if(key == GLFW_KEY_Q)
    {
        sInput.keyPressed[Helicopter::eControl::YAW_LEFT] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
    if(key == GLFW_KEY_E)
    {
        sInput.keyPressed[Helicopter::eControl::YAW_RIGHT] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }

    if(key == GLFW_KEY_LEFT_SHIFT)
    {
        sInput.keyPressed[Helicopter::eControl::THROTTLE_UP] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }
    if(key == GLFW_KEY_SPACE)
    {
        sInput.keyPressed[Helicopter::eControl::THROTTLE_DOWN] = (action == GLFW_PRESS || action == GLFW_REPEAT);
    }

    /* close window on escape */
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    /* make screenshot and save in work directory */
    if(key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        screenshotToPNG("screenshot.png");
    }
}

void mousePosCallback(GLFWwindow* window, double x, double y)
{
    if(sInput.mouseButtonPressed)
    {
        Vector2D diff = sInput.mousePressStart - Vector2D(x, y);
        cameraUpdateOrbit(sScene.camera, diff, 0.0f);
        sInput.mousePressStart = Vector2D(x, y);
    }
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if(button == GLFW_MOUSE_BUTTON_LEFT)
    {
        sInput.mouseButtonPressed = (action == GLFW_PRESS);

        double x, y;
        glfwGetCursorPos(window, &x, &y);
        sInput.mousePressStart = Vector2D(x, y);
    }
}

void mouseScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    cameraUpdateOrbit(sScene.camera, {0, 0}, sScene.zoomSpeedMultiplier * yoffset);
}

void windowResizeCallback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    sScene.camera.width = width;
    sScene.camera.height = height;
}

void sceneInit(float width, float height)
{
    sScene.camera = cameraCreate(width, height, to_radians(45.0), 0.01, 500.0, {10.0, 10.0, 10.0}, {0.0, 0.0, 0.0});
    sScene.cameraFollowHeli = true;
    sScene.zoomSpeedMultiplier = 0.05f;

    sScene.heli = helicopterLoad("assets/heli_low_poly/helicopter.obj");
    sScene.modelGround = modelLoad("assets/ground/ground.obj").front();

    sScene.shaderColor = shaderLoad("shader/default.vert", "shader/color.frag");
}

void sceneUpdate(float dt)
{
    helicopterMove(sScene.heli, sInput.keyPressed, dt);

    if (sScene.cameraFollowHeli)
        cameraFollow(sScene.camera, sScene.heli.position);
}

void sceneDraw()
{
    glClearColor(135.0 / 255, 206.0 / 255, 235.0 / 255, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /*------------ render scene -------------*/
    {
        /* setup camera and model matrices */
        Matrix4D proj = cameraProjection(sScene.camera);
        Matrix4D view = cameraView(sScene.camera);

        glUseProgram(sScene.shaderColor.id);
        shaderUniform(sScene.shaderColor, "uProj",  proj);
        shaderUniform(sScene.shaderColor, "uView",  view);
        shaderUniform(sScene.shaderColor, "uModel",  sScene.heli.transformation);

        /* render heli */
        for(unsigned int i = 0; i < sScene.heli.partModel.size(); i++)
        {
            auto& model = sScene.heli.partModel[i];
            auto& transform = sScene.heli.partTransformations[i];
            glBindVertexArray(model.mesh.vao);

            shaderUniform(sScene.shaderColor, "uModel", sScene.heli.transformation * transform);

            for(auto& material : model.material)
            {
                /* set material properties */
                shaderUniform(sScene.shaderColor, "uMaterial.diffuse", material.diffuse);

                glDrawElements(GL_TRIANGLES, material.indexCount, GL_UNSIGNED_INT, (const void*) (material.indexOffset*sizeof(unsigned int)) );
            }
        }

        /* render ground */
        shaderUniform(sScene.shaderColor, "uModel", Matrix4D::scale(4.0, 4.0, 4.0));
        glBindVertexArray(sScene.modelGround.mesh.vao);

        for(auto& material : sScene.modelGround.material)
        {
            /* set material properties */
            shaderUniform(sScene.shaderColor, "uMaterial.diffuse", material.diffuse);

            glDrawElements(GL_TRIANGLES, material.indexCount, GL_UNSIGNED_INT, (const void*) (material.indexOffset*sizeof(unsigned int)) );
        }

    }

    /* cleanup opengl state */
    glBindVertexArray(0);
    glUseProgram(0);
}

int main(int argc, char** argv)
{
    /*---------- init window ------------*/
    int width = 1280;
    int height = 720;
    GLFWwindow* window = windowCreate("Assignment 2 - Shading", width, height);
    if(!window) { return EXIT_FAILURE; }

    /* set window callbacks */
    glfwSetKeyCallback(window, keyCallback);
    glfwSetCursorPosCallback(window, mousePosCallback);
    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetScrollCallback(window, mouseScrollCallback);
    glfwSetFramebufferSizeCallback(window, windowResizeCallback);

    /*---------- init opengl stuff ------------*/
    glEnable(GL_DEPTH_TEST);

    /* setup scene */
    sceneInit(width, height);

    /*-------------- main loop ----------------*/
    double timeStamp = glfwGetTime();
    double timeStampNew = 0.0;
    while(!glfwWindowShouldClose(window))
    {
        /* poll and process input and window events */
        glfwPollEvents();

        /* update scene */
        timeStampNew = glfwGetTime();
        sceneUpdate(timeStampNew - timeStamp);
        timeStamp = timeStampNew;

        /* draw all objects in the scene */
        sceneDraw();

        /* swap front and back buffer */
        glfwSwapBuffers(window);
    }


    /*-------- cleanup --------*/
    helicopterDelete(sScene.heli);
    shaderDelete(sScene.shaderColor);
    windowDelete(window);

    return EXIT_SUCCESS;
}
