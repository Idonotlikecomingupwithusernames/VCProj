# VCProj

## Desc
The purpose of this Project is the implementation of screen-space reflections in opengl.
Formerly, we aspired to implementing refractions as well, but SSR has proven to be difficult enough.

## Members
Our team currently consists of the following members:
* Matthias Feichtinger
* David Kranebitter
* Dominik Schäfer

## Status
Our project is currently capable of displaying wildly incorrect, but still discernible reflections.
All reflections created in SSR.frag are centered around the origin of the reflective object (the center of our ground plane).
We have yet to find a way to fix this.
Furthermore, all reflections appear on the wrong side of the screen, as though reflected 180° in the wrong direction.
This seems easier to fix, however, with everything being circular, it's hard to tell what the problem is.
There are also plenty of blind spots in our reflections, the source of which may also become more apparent once the circulrity issue is resolved.

Reflections that do work are the following:
* Reflections of the sky (background color) in the strange shapes on the ground, if the camera is far away from the centere of the ground plane.
* Reflections of the Helicopter's blue and red colors, if the helictoper is close to the ground and the camera (helicopter at the bottom of the screen) (camera mode 1).
* Reflections of the Helicopter's moving rotors, spinning the wrong way.

## Build Instructions
Our current build process is that, which was outlined in one of the earlier exercise sheets
* \ProjectDir > cd build
* \ProjectDir > cmake ..
* \ProjectDir > make
* \ProjectDir > cd bin
* \ProjectDir > ./vcproj.exe
