// hpl1_frame.glsl
//
// Per-frame values, set when the 3D scene starts
// (iLowLevelGraphics::SetFrameUniforms). World space.

layout(std140) uniform hpl_Frame
{
	mat4 hpl_View;
	mat4 hpl_Projection;
	mat4 hpl_ViewProjection;
	vec4 hpl_EyePos;		// xyz: camera position
	vec4 hpl_RenderSize;	// xy: 3D scene size in pixels, zw: 1 / size
	vec4 hpl_DepthRange;	// x: near plane, y: far plane
	vec4 hpl_Time;			// x: seconds of rendering
	vec4 hpl_Shading0;		// cShadingSettings. x: 1 for the improved shading,
							// y: scale of highlights, z: reflectance of a white
							// specular map, w: ambient sheen
	vec4 hpl_Shading1;		// x: roughness where the specular map is black,
							// y: where it is white, z: ambient from above,
							// w: ambient towards the viewer
};
