#pragma once

#include "mygl/base.h"
#include "mygl/model.h"

#include <vector>

struct Helicopter
{
    enum eControl
    {
        PITCH_DOWN = 0,
        PITCH_UP,
        YAW_LEFT,
        YAW_RIGHT,
        ROLL_LEFT,
        ROLL_RIGHT,
        THROTTLE_UP,
        THROTTLE_DOWN,
        CONTROL_COUNT
    };

    enum ePart
    {
        BODY = 0,
        ROTOR,
        SLIDES,
        TAIL_ROTOR,
        WINDOWS,
        STROBE,
        SPOTLIGHT,
        LIGHTS,
        PART_COUNT
    };


    std::vector<Matrix4D> partTransformations;
    std::vector<Model> partModel;

    Matrix4D transformation = Matrix4D::identity();
    Matrix4D rotation = Matrix4D::identity();

    Vector3D position = {0.0, 0.0, 0.0};
    Vector3D angles = {0.0, 0.0, 0.0};

    float rotorRotation = 0.0f;

    float velocity = 10.0f;
    float lift = 3.0f;
};

Helicopter helicopterLoad(const std::string& filepath);
void helicopterDelete(Helicopter& heli);
void helicopterMove(Helicopter& heli, bool control[], float dt);
