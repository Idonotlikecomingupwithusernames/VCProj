#version 420 core

in vec2 tUV;

out vec4 FragColor;

// This was the only way we could get the textures to work
layout(binding = 0) uniform sampler2D texPos;
layout(binding = 1) uniform sampler2D texNorm;
layout(binding = 2) uniform sampler2D texColSpec;
layout(binding = 3) uniform sampler2D texDepth;

uniform mat4 uProj;
uniform mat4 uInvProj;

/* Neat linearization that may or may not work, sourced from github.com/pissang */
float linearDepth(float depth)
{
    return uProj[3][2] / (depth * uProj[2][3] - uProj[2][2]);
}

/* Reflections change drastically, depending on the linearization function used, which probably means we're doing it wrong */
float linearize(float depth){
    return (2.0*0.01*200.0) / (0.01 + 200.0 - (depth * 2.0 - 1.0) * (200.0 - 0.01));
}

void main(void)
{   
    float maxDistance = 15;
    int   steps       = 10;
    float thickness   = 0.5;

    vec2 texSize  = textureSize(texPos, 0).xy;

    // Storage for our reflected uv coordinates (why is it a vec4? god is weeping)
    vec4 uv = vec4(0.0);        

    // De-projecting would resolve the volatility issue, but stretches all reflections along one axis (given the lack of z division)
    //vec3 Position = (texture(texPos, tUV) * uInvProj).xyz; <- Probably wrong
    
    vec3 Position = texture(texPos, tUV).xyz;

    // De-projecting the normal resolves the circularity issue, but makes the reflections far more volatile and orients them all along the same axis (it's hard to tell whether this is correct or not)
    //vec3 Normal = normalize(texture(texNorm, tUV).xyz); <- Previously (circular reflections)
    vec3 Normal = normalize(texture(texNorm, tUV) * uInvProj).xyz;

    vec4 ColorSpec = texture(texColSpec, tUV);

    // Using the depth buffer results in no reflections for some reason (probably reading it wrong)

    vec3 Color = ColorSpec.rgb;
    // Wouldn't want the sky to be reflective (because w=1) (just don't set the specularity of objects to 1.0)
    float Spec = ColorSpec.w == 1.0f? 0.0 : ColorSpec.w;

    // If no hit is found, do not add any color
    FragColor = vec4(Color, 1.0f);

    if(Spec > 0.2f){
      bool Pass1Hit = false;
      bool Pass2Hit = false;

      vec3 Reflected = normalize(reflect(normalize(Position), Normal));

      // Ray endpoints in viewspace
      vec4 startView = vec4(Position, 1);
      vec4 endView = vec4(Position + (Reflected * maxDistance), 1);

      // Ray endpoints in screenspace
      vec4 startFrag = uProj * startView;
      startFrag.xyz /= startFrag.w;
      startFrag.xy = startFrag.xy * 0.5 + 0.5;
      startFrag.xy *= texSize;

      vec4 endFrag = uProj * endView;
      endFrag.xyz /= endFrag.w;
      endFrag.xy   = endFrag.xy * 0.5 + 0.5;
      endFrag.xy  *= texSize;

      // There's a lot of noobish back-and forth between vec types, which we can tidy up once it works
      vec3 currentFragment  = startFrag.xyz;
      uv.xy = currentFragment.xy / texSize;

      /* 
       * rayDepth : the z coordinate of our ray's current position
       * depth : the depth value in our "depth buffer" at the current pixel's position (except actually using the depth buffer values results in no reflections at all(bug))
       * dDepth : the difference in depth values between rayDepth and depth, necessary for determining whether a hit has occurred
       */
      float rayDepth;
      float depth;
      float dDepth;

      int Progress;
      
      /* First pass */
      /* Bresenham's line algorithm, sourced from rosettacode.org */

      float dx = abs(endFrag.x-startFrag.x);
      float dy = abs(endFrag.y-startFrag.y); 
      float err = (dx>dy ? dx : -dy)/2, e2;

      int sx = startFrag.x<endFrag.x ? 1 : -1, sy = startFrag.y<endFrag.y ? 1 : -1;

      /* The problem isn't in the 2/3 Bresenham (I've checked) */
      for(Progress = 0; Progress < maxDistance; Progress++){
        
        uv.xy = currentFragment.xy / texSize;
        depth = linearize(texture(texPos, uv.xy).z);
        // Discrete calculations make for blocky reflections, but it's not like they work anyway (also: yes this makes the line algorithm less efficient, we know)
        rayDepth = linearize((uProj * (startView + Progress * vec4(Reflected, 1.0f))).z);

        dDepth = rayDepth - depth;

        // If a hit is found, continue to pass 2
        if(dDepth > 0 && dDepth < thickness){
          Pass1Hit = true;
          break;
        }
        
        // We've arrived at the end of the ray
        if (currentFragment.x==endFrag.x && currentFragment.y==endFrag.y) break;

        e2 = err;
        if (e2 >-dx) { err -= dy; currentFragment.x += sx; }
        if (e2 < dy) { err += dx; currentFragment.y += sy; }
      }

      /* Second pass */
      if(Pass1Hit){
        // Look for a hit between the last position where there was no hit and the position where there was one
        // Maybe we should center this on the hit point instead?  [1/2] Hitpoint [1/2]
        vec3 startPos = startView.xyz + (Progress - 1) * Reflected;
        vec3 endPos = startView.xyz + Progress * Reflected;

        vec3 currentPos = startPos;

        for(int i = 0; i < steps; i++){
          uv.xy = currentPos.xy / texSize;
          depth = linearize(texture(texPos, uv.xy).z);
          rayDepth = linearize(currentPos.z);

          dDepth = rayDepth - depth;

          /* The worst binary search you've ever seen */
          if(dDepth > 0 && dDepth < thickness){
            Pass2Hit = true;
            uv.xy = currentPos.xy;
            currentPos -= Reflected * 1/pow(2, i+1);
          } else{
            currentPos += Reflected * 1/pow(2, i+1);
          }
        }

        /* This is where we'd put a visibility check, if the reflections did what they were supposed to */

        /* If a more fine-grained hit was found in the second pass, pass texture at those coordinates 
         * X((Currently, this produces funny shapes and white pixels, centered around the origin))
         * X((Reflections currently go the wrong way (?), reflecting objects toward the screen (???)))
         * De-projecting the normal resolves this issue and shows reflections that repeat periodically
         * X((De-projecting the position resolves this issue, but stretches reflections into infinity along the z axis))
         * These issues are likely caused by some combination of: Incorrect use of projections, Not using the depth buffer
         * Things that are reflected:
         * The rotation of the helicopter's rotor,
         * The helicopter's coloration, if the helicopter is close to the ground and moved forward a little
         * The background color
         */
        if(Pass2Hit){
          FragColor += clamp(texture(texColSpec, uv.xy), 0, 1);
        }
      }
    }
}
