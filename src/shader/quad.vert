#version 330 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;

out vec3 tNormal;
out vec3 tFragPos;
out vec2 tUV;

void main(void)
{
    gl_Position = vec4(aPosition, 1.0);
    tFragPos = aPosition;
    tNormal = aNormal;
    tUV = aUV;
}
