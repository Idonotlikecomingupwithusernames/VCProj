#version 330 core

layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gColorSpec;

struct Material
{
    vec4 ambient;
    vec3 diffuse;
    vec4 specular;
    float shininess;
};

in vec3 tFragPos;
in vec3 tNormal;

uniform Material uMaterial;

uniform float uSpec;

// Where is our Blinn-Phong? yes

void main()
{    
    gNormal = vec4(normalize(tNormal), 1.0f);
    gPosition = vec4(tFragPos, 1.0f);
    
    gColorSpec = vec4(uMaterial.diffuse.rgb, uSpec);
}  

