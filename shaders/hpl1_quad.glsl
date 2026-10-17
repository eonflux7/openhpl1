// Shared static sheet topology. All material/depth/fog programs consume the
// same expanded attributes; ordinary mesh/stream/fullscreen VAOs have kind 0.
layout(location = 3) in float hpl_QuadKind;
layout(location = 6) in vec3 hpl_QuadCentre;
layout(location = 7) in vec3 hpl_QuadRight;
layout(location = 8) in vec3 hpl_QuadUp;
layout(location = 9) in vec4 hpl_QuadColor;
layout(location = 10) in vec4 hpl_QuadEndColor;
layout(location = 11) in vec4 hpl_QuadUV;
const vec2 hpl_QuadCorners[4] = vec2[4](
    vec2(1,-1), vec2(-1,-1), vec2(-1,1), vec2(1,1));
vec4 hpl_ExpandedVertex()
{
    if(hpl_QuadKind < 0.5) return hpl_RawVertex;
    vec2 c = hpl_QuadCorners[gl_VertexID];
    return vec4(hpl_QuadCentre + (hpl_QuadRight * c.x + hpl_QuadUp * c.y), 1);
}
vec4 hpl_ExpandedColor()
{
    if(hpl_QuadKind < 0.5) return hpl_RawColor;
    return hpl_QuadCorners[gl_VertexID].y < 0.0 ? hpl_QuadColor : hpl_QuadEndColor;
}
vec4 hpl_ExpandedUV()
{
    if(hpl_QuadKind < 0.5) return hpl_RawUV;
    vec2 c = hpl_QuadCorners[gl_VertexID] * 0.5 + 0.5;
    return vec4(mix(hpl_QuadUV.xy, hpl_QuadUV.zw, c), 0, 1);
}
#define hpl_Vertex (hpl_ExpandedVertex())
#define hpl_Color (hpl_ExpandedColor())
#define hpl_TexCoord (hpl_ExpandedUV())
#define hpl_Normal ((hpl_QuadKind > 0.5 && hpl_QuadKind < 2.5) ? vec3(0,0,1) : hpl_RawNormal)
#define hpl_Tangent ((hpl_QuadKind > 0.5 && hpl_QuadKind < 2.5) ? vec4(1,0,0,hpl_QuadKind < 1.5 ? 1 : -1) : hpl_RawTangent)
