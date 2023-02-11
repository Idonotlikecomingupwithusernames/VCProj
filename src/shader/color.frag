#version 330 core

in vec2 tUV;

out vec4 FragColor;

uniform sampler2D texDepth;

void main(void)
{
    FragColor = vec4(texture(texDepth, tUV).rgb, 1.0);
}
