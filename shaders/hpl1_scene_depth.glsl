// hpl1_scene_depth.glsl
//
// The depth buffer of the 3D scene, for passes that draw into something
// other than the scene while they read it. Of a multisampled scene it is
// a copy with one sample a pixel, as it was when
// iLowLevelGraphics::ResolveSceneDepth() or ResolveScene() was last called.

#include "hpl1_frame.glsl"

uniform sampler2D hpl_SceneDepth;

// The depth buffer's value (0 near, 1 far) of a pixel of the scene.
float hpl_SceneDepthAt(ivec2 avPixel)
{
	return texelFetch(hpl_SceneDepth, avPixel, 0).r;
}

// How far in front of the camera a depth buffer value is, along the
// direction the camera looks in. Works for a far plane at infinity too.
float hpl_ViewDistance(float afDepth)
{
	return hpl_Projection[3][2] / (afDepth * 2.0 - 1.0 + hpl_Projection[2][2]);
}

// How far in front of the camera the scene is at a pixel. Where nothing
// was drawn (the depth buffer's far end, where hpl_ViewDistance() may
// give anything) it is far away.
float hpl_SceneDistanceAt(ivec2 avPixel)
{
	float fDepth = hpl_SceneDepthAt(avPixel);
	if(fDepth >= 1.0) return 1e9;
	float fDistance = hpl_ViewDistance(fDepth);
	return fDistance > 0.0 ? fDistance : 1e9;
}
