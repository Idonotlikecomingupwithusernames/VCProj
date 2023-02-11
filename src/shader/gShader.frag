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

/*
float linearize(float depth){
    return (2.0*0.1*100.0) / (0.1 + 100.0 - (depth * 2.0 - 1.0) * (100.0 - 0.1));
}
*/

void main()
{    
    gNormal = vec4(normalize(tNormal), 1.0f);
    gPosition = vec4(tFragPos, 1.0f);
    
    vec4 gColorSpec = vec4(uMaterial.diffuse.rgb, uSpec);

    //gColorSpec = vec4(vec3(float(linearize(gl_FragCoord.z)) / 100.0), 1.0f);
}  

