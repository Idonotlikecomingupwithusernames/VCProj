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
    ShaderProgram shaderGBuffer;

    int width = 1280;
    int height = 720;
} sScene;

// Global variables rule
GLuint gBuffer, gPosition, gNormal, gColorSpec, gDepth;
GLuint vao_quad = 0, vbo_quad = 0, ebo_quad = 0;

/* Define information for quad, sourced from example 09_framebuffer */
enum eIndex {position = 0, color = 1, uV = 2};

// Names changed to avoid conflict with definitions in mesh.h
struct Vex
{
    GLfloat position[3];
    GLfloat color[3];
    GLfloat uv[2];
};

// Color values should not be needed, as quad should be filled using texture (will change this when we get the rest working)
const std::vector<Vex> vertices_quad =
{
    /* position */          /* color */         /* uv */
    {{-1.0, -1.0, 0.0},     {1.0, 1.0, 1.0},    {0.0, 0.0}},
    {{-1.0,  1.0, 0.0},     {0.0, 1.0, 1.0},    {0.0, 1.0}},
    {{ 1.0,  1.0, 0.0},     {1.0, 1.0, 1.0},    {1.0, 1.0}},
    {{ 1.0, -1.0, 0.0},     {0.0, 0.0, 1.0},    {1.0, 0.0}}
};

const std::vector<unsigned int> indices_quad =
{
    0, 1, 2,
    2, 3, 0
};

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

    // GBuffer and (future) SSR fragment shaders should be able to share the same vertex shader (for now)
    sScene.shaderColor = shaderLoad("shader/default.vert", "shader/color.frag");
    sScene.shaderGBuffer = shaderLoad("shader/default.vert", "shader/gShader.frag");

    /* Create gBuffer, attach textures for position, normals, color + spec and depth */
    glGenFramebuffers(1, &gBuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
    
    glGenTextures(1, &gPosition);
    glBindTexture(GL_TEXTURE_2D, gPosition);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);

    glGenTextures(1, &gNormal);
    glBindTexture(GL_TEXTURE_2D, gNormal);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
    
    glGenTextures(1, &gColorSpec);
    glBindTexture(GL_TEXTURE_2D, gColorSpec);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gColorSpec, 0);

    glGenTextures(1, &gDepth);
    glBindTexture(GL_TEXTURE_2D, gDepth);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gDepth, 0);

    GLuint buffers[3] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2};
    glDrawBuffers(3, buffers);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        printf("Framebuffer incomplete \n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    /* Code for binding vertex buffer objects, sourced from example 09_framebuffer */

    /* generate vertex array object, and buffer */
    glGenVertexArrays(1, &vao_quad);
    glGenBuffers(1, &vbo_quad);
    glGenBuffers(1, &ebo_quad);

    /* bind buffer to vertex array object (remembers location of attribute data) */
    glBindVertexArray(vao_quad);
    {
        /* upload data to buffer */
        glBindBuffer(GL_ARRAY_BUFFER, vbo_quad);
        glBufferData(GL_ARRAY_BUFFER, vertices_quad.size() * sizeof(Vex), vertices_quad.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_quad);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_quad.size() * sizeof(unsigned int), indices_quad.data(), GL_STATIC_DRAW);

        /* enable attribute locations (see vertex shader) */
        glEnableVertexAttribArray(eIndex::position);
        glEnableVertexAttribArray(eIndex::color);
        glEnableVertexAttribArray(eIndex::uV);
        /* specify location of vertex attributes (see vertex shader) */
        glVertexAttribPointer(eIndex::position, 3, GL_FLOAT, GL_FALSE, sizeof(Vex), (void*) offsetof(Vex, position));
        glVertexAttribPointer(eIndex::color,    3, GL_FLOAT, GL_FALSE, sizeof(Vex), (void*) offsetof(Vex, color));
        glVertexAttribPointer(eIndex::uV,       2, GL_FLOAT, GL_FALSE, sizeof(Vex), (void*) offsetof(Vex, uv));
    }
    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void sceneUpdate(float dt)
{
    helicopterMove(sScene.heli, sInput.keyPressed, dt);

    if (sScene.cameraFollowHeli)
        cameraFollow(sScene.camera, sScene.heli.position);
}

void sceneDraw()
{
    // Don't overdo it
    //glClearColor(135.0 / 255, 206.0 / 255, 235.0 / 255, 1.0);
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /*------------ render scene -------------*/
    {
        /* setup camera and model matrices */
        Matrix4D proj = cameraProjection(sScene.camera);
        Matrix4D view = cameraView(sScene.camera);

        /* Draw scene to gBuffer */
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gBuffer);
        {
            glClearColor(135.0 / 255, 206.0 / 255, 235.0 / 255, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glEnable(GL_DEPTH_TEST);
            glDepthFunc(GL_LESS); 

            glUseProgram(sScene.shaderGBuffer.id);

            shaderUniform(sScene.shaderGBuffer, "uProj",  proj);
            shaderUniform(sScene.shaderGBuffer, "uView",  view);
            shaderUniform(sScene.shaderGBuffer, "uModel",  sScene.heli.transformation);

            /* render heli */
            for(unsigned int i = 0; i < sScene.heli.partModel.size(); i++)
            {
                auto& model = sScene.heli.partModel[i];
                auto& transform = sScene.heli.partTransformations[i];
                glBindVertexArray(model.mesh.vao);

                shaderUniform(sScene.shaderGBuffer, "uModel", sScene.heli.transformation * transform);

                for(auto& material : model.material)
                {
                    /* set material properties */
                    shaderUniform(sScene.shaderGBuffer, "uMaterial.diffuse", material.diffuse);
                    // Specular component hardcoded until we get it working
                    shaderUniform(sScene.shaderGBuffer, "uSpec", 0.0f);

                    glDrawElements(GL_TRIANGLES, material.indexCount, GL_UNSIGNED_INT, (const void*) (material.indexOffset*sizeof(unsigned int)) );
                }
            }

            /* render ground */
            shaderUniform(sScene.shaderGBuffer, "uModel", Matrix4D::scale(4.0, 4.0, 4.0));
            glBindVertexArray(sScene.modelGround.mesh.vao);

            for(auto& material : sScene.modelGround.material)
            {
                /* set material properties */
                shaderUniform(sScene.shaderGBuffer, "uMaterial.diffuse", material.diffuse);
                // Specular component hardcoded until we get it working
                shaderUniform(sScene.shaderGBuffer, "uSpec", 0.7f);

                glDrawElements(GL_TRIANGLES, material.indexCount, GL_UNSIGNED_INT, (const void*) (material.indexOffset*sizeof(unsigned int)) );
            }

            
        }

        /* Switch draw buffer back to screen */
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        {
            //glClearColor(135.0 / 255, 206.0 / 255, 235.0 / 255, 1.0);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glDisable(GL_DEPTH_TEST);

            /*
            glBindFramebuffer(GL_READ_FRAMEBUFFER, gBuffer);

            GLsizei HalfWidth = (GLsizei)(sScene.width / 2.0f);
            GLsizei HalfHeight = (GLsizei)(sScene.height / 2.0f);

            // Blit position, normals and color information to screen (this works and displays the correct images)

            glReadBuffer(GL_COLOR_ATTACHMENT0);
            glBlitNamedFramebuffer(gBuffer, 0, 0, 0, sScene.width, sScene.height, 0, 0, HalfWidth, HalfHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);

            glReadBuffer(GL_COLOR_ATTACHMENT0 + 1);
            glBlitNamedFramebuffer(gBuffer, 0, 0, 0, sScene.width, sScene.height, 0, HalfHeight, HalfWidth, sScene.height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

            glReadBuffer(GL_COLOR_ATTACHMENT0 + 2);
            glBlitFramebuffer(0, 0, sScene.width, sScene.height, HalfWidth, HalfHeight, sScene.width, sScene.height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

            // This does not work and is likely incorrect, as glDrawBuffer only works with COLOR_ATTACHMENTs and glReadBuffer likely does too

            glReadBuffer(GL_DEPTH_COMPONENT);
            glBlitFramebuffer(0, 0, sScene.width, sScene.height, HalfWidth, 0, sScene.width, HalfHeight, GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            */

            glUseProgram(sScene.shaderColor.id);

            // Just get one texture through the fragment shader for now
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, gPosition);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, gNormal);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, gColorSpec);
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, gDepth);

            shaderUniform(sScene.shaderColor, "uProj",  proj);

            // This may not be necessary but it doesn't work either way
            //glUniform1i(glGetUniformLocation(sScene.shaderColor.id, "texDepth"), GL_TEXTURE0);

            /* draw content in vertex array */
            glBindVertexArray(vao_quad);
            {
                glDrawElements(GL_TRIANGLES, indices_quad.size(), GL_UNSIGNED_INT, (void*) 0);
            }
            glBindVertexArray(0);

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
    GLFWwindow* window = windowCreate("Why it no work", width, height);
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
    shaderDelete(sScene.shaderGBuffer);
    windowDelete(window);

    return EXIT_SUCCESS;
}
