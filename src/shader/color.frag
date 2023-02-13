#version 330 core

in vec3 tColor;
in vec2 tUV;

out vec4 FragColor;

uniform sampler2D texPos;
uniform sampler2D texNorm;
uniform sampler2D texColSpec;
uniform sampler2D texDepth;

uniform mat4 uProj;

/* Bresenham's line algorithm, sourced from rosettacode.org */
void line(int x0, int y0, int x1, int y1) {

  int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
  int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1; 
  int err = (dx>dy ? dx : -dy)/2, e2;

  for(;;){
    //setPixel(x0,y0);
    if (x0==x1 && y0==y1) break;
    e2 = err;
    if (e2 >-dx) { err -= dy; x0 += sx; }
    if (e2 < dy) { err += dx; y0 += sy; }
  }
}

/* Neat linearization that may or may not work, sourced from github.com/pissang */
float linearDepth(float depth)
{
    return uProj[3][2] / (depth * uProj[2][3] - uProj[2][2]);
}

void main(void)
{   
    float maxDistance = 15;
    float resolution  = 0.3;
    int   steps       = 10;
    float thickness   = 0.5;

    vec2 texSize  = textureSize(texPos, 0).xy;
    vec2 texCoord = gl_FragCoord.xy / texSize;

    vec4 uv = vec4(0.0);        

    vec3 Position = texture(texPos, tUV).rgb;
    vec3 Normal = normalize(texture(texNorm, tUV).rgb);
    vec4 ColorSpec = texture(texColSpec, tUV);
    vec3 Depth = texture(texDepth, tUV).rgb;

    vec3 Color = ColorSpec.rgb;
    float Spec = ColorSpec.w;

    vec3 Reflected = normalize(reflect(normalize(Position), Normal));

    vec4 startView = vec4(Position, 1);
    vec4 endView = vec4(Position + (Reflected * maxDistance), 1);

    vec4 startFrag = uProj * startView;
    startFrag.xyz /= startFrag.w;
    startFrag.xy = startFrag.xy * 0.5 + 0.5;
    startFrag.xy *= texSize;

    vec4 endFrag = uProj * endView;
    endFrag.xyz /= endFrag.w;
    endFrag.xy   = endFrag.xy * 0.5 + 0.5;
    endFrag.xy  *= texSize;

    //FragColor = vec4(Depth, 1.0);
    FragColor = ColorSpec;
}
