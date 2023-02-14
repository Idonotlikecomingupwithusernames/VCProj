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

float linearize(float depth){
    return (2.0*0.1*200.0) / (0.1 + 200.0 - (depth * 2.0 - 1.0) * (200.0 - 0.1));
}

void main(void)
{   
    float maxDistance = 15;
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

    // If no hit is found, do not add any color
    FragColor = vec4(Color, 1.0f);

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

      vec3 currentFragment  = startFrag.xyz;
      uv.xy = currentFragment.xy / texSize;

      bool Pass1Hit = false;
      bool Pass2Hit = false;

      /* 
       * rayDepth : the z coordinate of our ray's current position
       * depth : the depth value in our depth buffer at the current Pixel's position
       * dDepth : the difference in depth values between rayDepth and depth, necessary for determining whether a hit has occurred
       */
      float rayDepth;
      float depth;
      float dDepth;

      int Progress;
      
      /* First pass */
      /* Bresenham's line algorithm, sourced from rosettacode.org */

      float dx = abs(endFrag.x-startFrag.x), sx = startFrag.x<endFrag.x ? 1 : -1;
      float dy = abs(endFrag.y-startFrag.y), sy = startFrag.y<endFrag.y ? 1 : -1; 
      float err = (dx>dy ? dx : -dy)/2, e2;

      for(Progress = 0; Progress < maxDistance; Progress++){
        
        uv.xy = currentFragment.xy / texSize;
        depth = texture(texPos, uv.xy).z;
        rayDepth = (uProj * (startView + Progress * vec4(Reflected, 1.0f))).z;

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
        vec3 startPos = startView.xyz + (Progress - 1) * Reflected;
        vec3 endPos = startView.xyz + Progress * Reflected;

        vec3 currentPos = startPos;

        for(int i = 0; i < steps; i++){
          uv.xy = currentPos.xy / texSize;
          depth = texture(texPos, uv.xy).z;
          rayDepth = currentPos.z;

          dDepth = rayDepth - depth;

          /* The worst binary search you've ever seen */
          if(dDepth > 0 && dDepth < thickness){
            Pass2Hit = true;
            currentPos -= Reflected * 1/pow(2, i+1);
          } else{
            currentPos += Reflected * 1/pow(2, i+1);
          }
          
        }

        /* If a more fine-grained hit was found in the second pass, pass texture at those coordinates */
        if(Pass2Hit){
          FragColor += texture(texColSpec, currentPos.xy);
        }
      }
    }
}
