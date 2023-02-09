#version 330 core

layout (location = 0) out vec3 gPosition;
layout (location = 1) out vec3 gNormal;
layout (location = 2) out vec4 gColorSpec;

struct Material
{
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float shininess;
};

in vec3 tFragPos;
in vec3 tNormal;

uniform Material uMaterial;

uniform float uSpec;

void main()
{    
    gNormal = normalize(tNormal);
    gPosition = tFragPos;
    
    gColorSpec = vec4(uMaterial.diffuse.rgb, uSpec);
}  