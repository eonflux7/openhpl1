// Copyright 2006-2010 (C) - Frictional Games
//
// This file is part of HPL1 Engine
//
// For conditions of distribution and use, see copyright notice in LICENSE-shaders
//

///////////////////////////////////////////////////////
/// LIGHT PASS VARYINGS ////////////////////////////////
///////////////////////////////////////////////////////

// Shared by hpl1_light.vertex and hpl1_light.fragment.
// SPOT: the light is a spotlight. SPECULAR: specular term.

VARYING vec4 vLightColor;
VARYING vec2 vUv;
VARYING vec3 vLightDir;
#ifdef SPECULAR
VARYING vec3 vHalfVec;
// Towards the eye, in tangent space, not normalised (improved shading)
VARYING vec3 vEyeVec;
// A tube light (cLightLook): to its middle, and from there to one end, in
// tangent space, not normalised
VARYING vec3 vTubeCentre;
VARYING vec3 vTubeAxis;
#endif
#ifdef SPOT
VARYING vec4 vSpotlightUv;
#endif
// For the shadow map: in the spotlight's view, or relative to a point light
// along the world's axes (hpl1_shadow.glsl)
VARYING vec3 vShadowPos;
