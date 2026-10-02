// hpl1_scene_depth.glsl
//
// The depth buffer of the 3D scene, for passes that draw into something
// other than the scene while they read it. A multisampled scene has a
// multisampled depth texture, of which the first sample is taken.

#include "hpl1_frame.glsl"

#ifdef HPL_SCENE_MULTISAMPLE
uniform sampler2DMS hpl_SceneDepth;
#else
uniform sampler2D hpl_SceneDepth;
#endif

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
