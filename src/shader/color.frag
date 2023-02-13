#version 420 core

//in vec3 tColor;
in vec2 tUV;

out vec4 FragColor;

/*
uniform sampler2D texPos;
uniform sampler2D texNorm;
uniform sampler2D texColSpec;
uniform sampler2D texDepth;
*/

layout(binding = 0) uniform sampler2D texPos;
layout(binding = 1) uniform sampler2D texNorm;
layout(binding = 2) uniform sampler2D texColSpec;
layout(binding = 3) uniform sampler2D texDepth;

uniform mat4 uProj;

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

    vec3 Position = texture(texPos, tUV).xyz;
    vec3 Normal = normalize(texture(texNorm, tUV).xyz);
    vec4 ColorSpec = texture(texColSpec, tUV);
    float Depth = texture(texDepth, tUV).x;

    vec3 Color = ColorSpec.rgb;
    float Spec = ColorSpec.w;

    if(Spec > 0.2f){
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

      vec3 currentFragment  = startFrag.xy;
      uv.xy = currentFragment / texSize;

      bool Pass1Hit = false;
      bool Pass2Hit = false;

      float rayDepth;
      float depth;

      int Progress;
      
      /* First pass */
      /* Bresenham's line algorithm, sourced from rosettacode.org */

      int dx = abs(endFrag.x-startFrag.x), sx = startFrag.x<endFrag.x ? 1 : -1;
      int dy = abs(endFrag.y-startFrag.y), sy = startFrag.y<endFrag.y ? 1 : -1; 
      int err = (dx>dy ? dx : -dy)/2, e2;

      for(Progress = 0; Progress < maxSteps; Progress++){
        
        uv.xy = currentFragment.xy / texSize;
        depth = texture(texPos, uv.xy).z;
        rayDepth = (uProj * (startView + Progress * Reflected)).z;

        dDepth = rayDepth - depth;

        if(dDepth > 0 && dDepth < thickness){
          Pass1Hit = true;
          break;
        }

        if (currentFragment.x==endFrag.x && currentFragment.y==endFrag.y) break;

        e2 = err;
        if (e2 >-dx) { err -= dy; currentFragment.x += sx; }
        if (e2 < dy) { err += dx; currentFragment.y += sy; }
      }

      /* Second pass */
      if(Pass1Hit){
        vec3 startPos = startView + (Progress - 1) * Reflected;
        vec3 endPos = startView + Progress * Reflected;

        vec3 currentPos = startPos;

        for(int i = 0; i < steps; i++){
          uv.xy = currentPos.xy / texSize;
          depth = texture(texPos, uv.xy).z;
          rayDepth = CurrentPos.z;

          dDepth = rayDepth - depth;

          /* The worst binary search you've ever seen */
          if(dDepth > 0 && dDepth < thickness){
            Pass2Hit = true;
            CurrentPos -= Reflected * 1/pow(2, i+1);
          } else{
            CurrentPos += Reflected * 1/pow(2, i+1);
          }
          
        }
      }


    }

    FragColor = ColorSpec;
}
