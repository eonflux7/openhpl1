// This file is part of OpenHPL1, licensed under the GNU GPL v3 or
// later. See COPYING.
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <glad/gl.h>
#include <array>
#include <cmath>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <map>
#include <set>
#include <tuple>
#include "impl/GLGraphicsState.h"
#include "impl/GLRenderTargets.h"
#include "impl/GLStreamDrawing.h"
#include "impl/GLShadowTargets.h"
#include "impl/GLFinalPasses.h"
// Initialize the facade's resource owners under the external EGL context.
#define private public
#include "impl/LowLevelGraphicsSDL.h"
#undef private
#include "impl/SDLTexture.h"
#include "impl/GLSLProgram.h"
#include "math/Math.h"
#include "scene/Camera3D.h"
using namespace hpl;
static void Check(bool ok, const char* message) { if(!ok) throw std::runtime_error(message); }
static void NoErrors() { Check(glGetError() == GL_NO_ERROR, "OpenGL error in renderer storage regression"); }
static void Immutable(iTexture& texture, GLenum target, int levels)
{
	glBindTexture(target, texture.GetHandle());
	GLint immutable=0, count=0;
	glGetTexParameteriv(target, GL_TEXTURE_IMMUTABLE_FORMAT, &immutable);
	glGetTexParameteriv(target, GL_TEXTURE_IMMUTABLE_LEVELS, &count);
	Check(immutable && count == levels, "immutable storage and mip count");
}
static void TestQuadInstances(cLowLevelGraphicsSDL &gfx)
{
	Check(gfx.mState.Init(), "initialize facade draw state under external context");
	gfx.mStream.Init();
	const auto directory=std::filesystem::path(cGLSLProgram::GetShaderDirectory());
	// A probe makes UVs, colour, normal and tangent handedness visible as well
	// as position. It uses the same preamble and material/sampler path as game programs.
	std::ofstream(directory/"hpl1_QuadProbe.vertex") << R"(
out vec4 vColor;
out vec2 vUv;
out vec3 vBasis;
void main() {
 gl_Position=worldViewProj*hpl_Vertex;
 vColor=hpl_Color; vUv=hpl_TexCoord.xy;
 vBasis=hpl_Normal*0.2+hpl_Tangent.xyz*0.1+hpl_Tangent.w*0.05;
})";
	std::ofstream(directory/"hpl1_QuadProbe.fragment") << R"(
in vec4 vColor;
in vec2 vUv;
in vec3 vBasis;
HPL_TEXTURE_2D(0)
OUTPUT
void main() { outColor=vec4((vColor.rgb*0.5+vec3(fract(vUv),0)*0.2+vBasis)*hpl_Texture0(vUv).rgb,vColor.a); }
)";
	cGLSLProgram probe("QuadProbe",false);
	Check(probe.CreateFromFile("QuadProbe"),"compile compact geometry probe");
	cSDLPixelFormat format;
	cSDLBitmap2D white(SDL_CreateSurface(2,2,SDL_PIXELFORMAT_RGBA32),&format,"SDL");
	cSDLBitmap2D green(SDL_CreateSurface(2,2,SDL_PIXELFORMAT_RGBA32),&format,"SDL");
	white.FillRect(cRect2l(0,0,0,0),cColor(1,1));green.FillRect(cRect2l(0,0,0,0),cColor(0.2f,1,0.4f,1));
	cSDLTexture texture("quad texture",&format,&gfx,eTextureType_Normal,false,eTextureTarget_2D);
	Check(texture.CreateFromBitmap(&white),"quad static texture");
	cSDLTexture animated("quad animation",&format,&gfx,eTextureType_Normal,false,eTextureTarget_2D);
	tBitmap2DVec frames={&white,&green};Check(animated.CreateAnimFromBitmapVec(&frames),"quad animation array");
	cSDLTexture target("quad target",&format,&gfx,eTextureType_RenderTarget,false,eTextureTarget_2D);
	Check(target.Create(48,48,cColor(0)),"quad readback target");gfx.SetRenderTarget(&target);
	gfx.SetDrawMatrices(cDrawMatrices{});
	gfx.SetDepthTestActive(false);gfx.SetCullActive(false);gfx.SetAlphaCutoff(0);
	gfx.mState.SetCurrentAttrib(cGLSLProgram::eVertexAttrib_Normal,0,0,1,1);
	gfx.mState.SetCurrentAttrib(cGLSLProgram::eVertexAttrib_Tangent,0,0,0,1);

	std::vector<cQuadInstance> sheets(3);
	for(unsigned i=0;i<sheets.size();++i)
	{
		auto &sheet=sheets[i];sheet.mfKind=float(i+1);
		sheet.mvCentre=cVector3f(-0.2f+i*0.2f,0,0);
		sheet.mvRight=cVector3f(0.3f,0.1f,0.02f);
		sheet.mvUp=cVector3f(-0.1f,0.45f,0.04f);
		sheet.mColor=cColor(0.2f,0.4f,0.6f,0.4f);
		sheet.mEndColor=cColor(0.6f,0.3f,0.1f,0.7f);
		sheet.mvUV[0]=0.5f;sheet.mvUV[1]=1;sheet.mvUV[2]=1;sheet.mvUV[3]=i==1?-3.5f:0.5f;
	}
	std::unique_ptr<iVertexBuffer> instances(gfx.CreateVertexBuffer(0,eVertexBufferDrawType_Tri,eVertexBufferUsageType_Stream));
	instances->SetQuadInstances(sheets);Check(instances->Compile(0),"compile instance buffer");
	std::unique_ptr<iVertexBuffer> reference(gfx.CreateVertexBuffer(
		eVertexFlag_Position|eVertexFlag_Normal|eVertexFlag_Color0|eVertexFlag_TexCoord|eVertexFlag_Tangent,
		eVertexBufferDrawType_Tri,eVertexBufferUsageType_Static));
	for(unsigned i=0;i<sheets.size();++i)
	{
		const auto &s=sheets[i];
		for(int corner=0;corner<4;++corner)
		{
			const float x=corner==0||corner==3?1:-1,y=corner<2?-1:1;
			reference->AddVertex(eVertexFlag_Position,s.mvCentre+(s.mvRight*x+s.mvUp*y));
			reference->AddColor(eVertexFlag_Color0,corner<2?s.mColor:s.mEndColor);
			reference->AddVertex(eVertexFlag_TexCoord,cVector3f(x<0?s.mvUV[0]:s.mvUV[2],y<0?s.mvUV[1]:s.mvUV[3],0));
			reference->AddVertex(eVertexFlag_Normal,cVector3f(0,0,1));
			reference->AddColor(eVertexFlag_Tangent,i<2?cColor(1,0,0,i==0?1:-1):cColor(0,0,0,1));
		}
		for(unsigned corner:{0u,1u,2u,2u,3u,0u})reference->AddIndex(i*4+corner);
	}
	Check(reference->Compile(0),"compile previous expanded geometry reference");
	probe.Bind();
	auto render=[&](iVertexBuffer &buffer){
		gfx.SetClearColor(cColor(0.1f,0.2f,0.3f,0));gfx.ClearScreen();buffer.Bind();buffer.Draw();
		std::vector<unsigned char> pixels(48*48*4);glReadPixels(0,0,48,48,GL_RGBA,GL_UNSIGNED_BYTE,pixels.data());return pixels;
	};
	for(auto *sampled:{&texture,&animated})
	for(int layer=0;layer<(sampled==&animated?2:1);++layer)
	{
		if(layer) animated.NextFrame();
		gfx.SetTexture(0,sampled);
		for(auto blend:std::vector<std::pair<eBlendFunc,eBlendFunc>>{
			{eBlendFunc_One,eBlendFunc_Zero},{eBlendFunc_One,eBlendFunc_One},
			{eBlendFunc_Zero,eBlendFunc_SrcColor},{eBlendFunc_DestColor,eBlendFunc_SrcColor},
			{eBlendFunc_SrcAlpha,eBlendFunc_OneMinusSrcAlpha},{eBlendFunc_DestAlpha,eBlendFunc_One}})
		{
			gfx.SetBlendActive(true);gfx.SetBlendFunc(blend.first,blend.second);
			const auto expected=render(*reference),actual=render(*instances);
			int drawn=0;for(size_t i=0;i<actual.size();++i){Check(std::abs(int(actual[i])-int(expected[i]))<=1,"instanced output matches expanded sheets across blend modes/layers");if(i%4==3 && actual[i])++drawn;}
			if(blend.first==eBlendFunc_One && blend.second==eBlendFunc_Zero)
				Check(drawn>100,"quad comparison has visible coverage");
		}
	}
	// Returning from an instance array must restore kind zero for ordinary
	// indexed and streamed draws; repeat in both directions with debug lines.
	gfx.SetTexture(0,&texture);gfx.SetBlendActive(false);
	instances->Draw(eVertexBufferDrawType_Lines);reference->Draw();NoErrors();
	GLuint query=0;glGenQueries(1,&query);
	instances->SetElementNum(6);
	glBeginQuery(GL_PRIMITIVES_GENERATED,query);instances->Draw();glEndQuery(GL_PRIMITIVES_GENERATED);
	GLuint primitives=0;glGetQueryObjectuiv(query,GL_QUERY_RESULT,&primitives);
	Check(primitives==2,"element limit counts instances");
	instances->SetElementNum(-1);instances->SetQuadInstances({});
	glBeginQuery(GL_PRIMITIVES_GENERATED,query);instances->Draw();glEndQuery(GL_PRIMITIVES_GENERATED);
	glGetQueryObjectuiv(query,GL_QUERY_RESULT,&primitives);Check(primitives==0,"empty emitter issues no geometry");
	instances->SetQuadInstances(sheets);
	std::unique_ptr<iVertexBuffer> copy(instances->CreateCopy(eVertexBufferUsageType_Stream));
	Check(render(*copy)==render(*instances),"instance copies retain independent compact records");
	gfx.SetTexture(0,&animated);
	const auto include=directory/"hpl1_quad.glsl";
	std::ifstream file(include);std::string original((std::istreambuf_iterator<char>(file)),{});file.close();
	std::ofstream(include,std::ios::app)<<"\ninvalid quad shader edit\n";
	Check(!probe.Reload(),"invalid geometry include preserves linked sampler variants");
	Check(render(*instances)==render(*copy),"failed geometry reload retains usable instanced draws");
	std::ofstream(include)<<original;Check(probe.Reload(),"valid geometry include reloads sampler variants atomically");
	Check(render(*instances)==render(*copy),"successful reload keeps instance and material data");
	// A new array layout can fail even with a valid base program. PrepareDraw
	// must skip this instance draw and recover after an atomic reload.
	std::ofstream(include,std::ios::app)<<"\n#ifdef HPL_ARRAY_0\ninvalid array expansion\n#endif\n";
	{
		cGLSLProgram rejected("quad rejected",false);
		Check(rejected.CreateFromFile("QuadProbe"),"valid static layout before array failure");
		rejected.Bind();
		glBeginQuery(GL_PRIMITIVES_GENERATED,query);instances->Draw();glEndQuery(GL_PRIMITIVES_GENERATED);
		glGetQueryObjectuiv(query,GL_QUERY_RESULT,&primitives);Check(primitives==0,"unusable sampler layout skips instance draw");
		std::ofstream(include)<<original;Check(rejected.Reload(),"repair array expansion");
		glBeginQuery(GL_PRIMITIVES_GENERATED,query);instances->Draw();glEndQuery(GL_PRIMITIVES_GENERATED);
		glGetQueryObjectuiv(query,GL_QUERY_RESULT,&primitives);Check(primitives==6,"repaired layout draws all instances");
		rejected.UnBind();
	}

	// The production particle program must read unit 10 and apply soft depth
	// to alpha or RGB according to blending, including animated textures.
	cGLSLProgram particle("Particle",false);Check(particle.CreateFromFile("Particle"),"production particle program");
	particle.SetVec2f(eProgramParam::fogRange,0,100);
	particle.SetMatrixf(eProgramParam::partToWorld,cMatrixf::Identity);
	cCamera3D camera;camera.SetNearClipPlane(0.1f);camera.SetFarClipPlane(100);
	const cMatrixf projection=camera.GetProjectionMatrix();
	gfx.mState.SetFrameUniforms(cMatrixf::Identity,projection,cVector3f(0),0.1f,100,0,cVector2l(48));
	GLuint depth=0;glGenTextures(1,&depth);
	glActiveTexture(GL_TEXTURE0+10);glBindTexture(GL_TEXTURE_2D,depth);
	glTexStorage2D(GL_TEXTURE_2D,1,GL_DEPTH_COMPONENT32F,48,48);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glBindSampler(10,0);glActiveTexture(GL_TEXTURE0);
	particle.Bind();
	for(int added=0;added<2;++added)
	{
		particle.SetVec4f(eProgramParam::partLook,0.3f,0,float(added),0);
		int previous=-1;
		for(float distance:{0.5f,1.15f,3.0f})
		{
			const float value=0.5f+0.5f*(-projection.m[2][2]+projection.m[2][3]/distance);
			std::vector<float> depths(48*48,value);
			glActiveTexture(GL_TEXTURE0+10);glBindTexture(GL_TEXTURE_2D,depth);
			glTexSubImage2D(GL_TEXTURE_2D,0,0,0,48,48,GL_DEPTH_COMPONENT,GL_FLOAT,depths.data());glActiveTexture(GL_TEXTURE0);
			const auto expected=render(*reference),actual=render(*instances);
			int amount=0;for(size_t i=0;i<actual.size();++i){Check(std::abs(int(actual[i])-int(expected[i]))<=1,"soft particle output matches expanded geometry");if(added?i%4!=3:i%4==3)amount+=actual[i];}
			Check(amount>previous,"particle fades from hidden through intersecting to fully visible");previous=amount;
		}
	}
	particle.UnBind();glDeleteTextures(1,&depth);
	for(const char *name:{"Fog_Solid","Fog_Trans","Halo","Shaft"})
	{
		cGLSLProgram pass(name);Check(pass.CreateFromFile(name),"geometry expansion links with fog/halo/shaft passes");
	}
	instances->Bind();
	GLint instanceVao=0,instanceVbo=0,sheetElements=0;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING,&instanceVao);
	glGetVertexAttribiv(6,GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING,&instanceVbo);
	glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&sheetElements);
	copy->Bind();GLint sharedElements=0;glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING,&sharedElements);
	Check(sheetElements && sheetElements==sharedElements,"effect buffers share one static triangle topology");
	instances.reset();
	Check(!glIsVertexArray(instanceVao) && !glIsBuffer(instanceVbo),"instance resources released under external context");
	probe.UnBind();gfx.SetTexture(0,nullptr);gfx.SetRenderTarget(nullptr);
	glDeleteQueries(1,&query);NoErrors();
	gfx.mStream.Reset();Check(!glIsBuffer(sheetElements),"shared sheet topology released by stream owner");gfx.mState.Reset();
}

static void TestTextures(cLowLevelGraphicsSDL& gfx)
{
	cSDLPixelFormat format;
	cSDLBitmap2D red(SDL_CreateSurface(4,4,SDL_PIXELFORMAT_RGBA32), &format, "SDL");
	cSDLBitmap2D green(SDL_CreateSurface(4,4,SDL_PIXELFORMAT_RGBA32), &format, "SDL");
	red.FillRect(cRect2l(0,0,0,0), cColor(1,0,0,1));
	green.FillRect(cRect2l(0,0,0,0), cColor(0,1,0,1));
	cSDLTexture atlas("atlas", &format, &gfx, eTextureType_Normal, true, eTextureTarget_2D);
	Check(atlas.CreateFromBitmap(&red), "initial atlas upload");
	int atlasHandle=atlas.GetHandle();
	Check(atlas.CreateFromBitmap(&green) && atlas.GetHandle()==atlasHandle, "atlas updates reuse immutable storage");
	Immutable(atlas, GL_TEXTURE_2D, 3);
	std::array<unsigned char,64> pixels{};
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	Check(pixels[0]==0 && pixels[1]==255 && pixels[3]==255, "atlas contents updated");
	gfx.SetTexture(0, &atlas);
	unsigned char blue[4]={0,0,255,255};
	atlas.SetPixels2D(0, cVector2l(0), cVector2l(1), eColorDataFormat_RGBA, blue);
	glBindTexture(GL_TEXTURE_2D, atlas.GetHandle());
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	Check(pixels[2]==255 && pixels[5]==255, "atlas subimage update preserves neighbors");

	cSDLTexture anim("animation", &format, &gfx, eTextureType_Normal, true, eTextureTarget_2D);
	tBitmap2DVec frames={&red,&green};
	Check(anim.CreateAnimFromBitmapVec(&frames) && anim.HasAnimation(), "animation array upload");
	Immutable(anim, GL_TEXTURE_2D_ARRAY, 3);
	std::array<unsigned char,128> layers{};
	glGetTexImage(GL_TEXTURE_2D_ARRAY, 0, GL_RGBA, GL_UNSIGNED_BYTE, layers.data());
	Check(layers[0]==255 && layers[1]==0 && layers[64]==0 && layers[65]==255, "animation layers keep distinct frames");
	anim.SetWrapS(eTextureWrap_ClampToEdge);anim.SetWrapT(eTextureWrap_Repeat);
	GLuint sampler=anim.GetSampler();GLint wrap=0,filter=0;
	glGetSamplerParameteriv(sampler,GL_TEXTURE_WRAP_S,&wrap);
	glGetSamplerParameteriv(sampler,GL_TEXTURE_MIN_FILTER,&filter);
	Check(wrap==GL_CLAMP_TO_EDGE && filter==GL_LINEAR_MIPMAP_LINEAR, "array filtering and wrapping");
	glBindTexture(GL_TEXTURE_2D_ARRAY,anim.GetHandle());
	for(int i=0;i<16;++i) layers[i*4+3]= i<4 ? 255 : 0;
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY,0,0,0,0,4,4,2,GL_RGBA,GL_UNSIGNED_BYTE,layers.data());
	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);anim.KeepAlphaCoverage(0.6f);
	glBindTexture(GL_TEXTURE_2D_ARRAY,anim.GetHandle());
	unsigned char mips[32];glGetTexImage(GL_TEXTURE_2D_ARRAY,1,GL_RGBA,GL_UNSIGNED_BYTE,mips);
	Check(mips[3]>=153 && mips[19]==255, "alpha coverage retained independently per animation layer");
	// Restore the solid-colour frames for material sampling below.
	glTexSubImage3D(GL_TEXTURE_2D_ARRAY,0,0,0,0,4,4,1,GL_RGBA,GL_UNSIGNED_BYTE,red.GetSurface()->pixels);
	glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
	int handle=anim.GetHandle();
	anim.SetFrameTime(0.25f); anim.Update(0.125f);
	Check(anim.GetAnimationFrame()==0 && std::fabs(anim.GetT()-0.5f)<0.0001f, "fractional animation timing");
	anim.Update(0.125f); Check(anim.GetAnimationFrame()==1, "animation advances a layer");
	anim.NextFrame(); Check(anim.GetAnimationFrame()==0 && anim.GetHandle()==handle, "loop retains one texture object");
	anim.SetAnimMode(eTextureAnimMode_Oscillate);
	anim.NextFrame(); anim.NextFrame(); Check(anim.GetAnimationFrame()==1, "oscillation end");
	anim.NextFrame(); Check(anim.GetAnimationFrame()==0, "oscillation reverses");
	anim.NextFrame(); Check(anim.GetAnimationFrame()==1, "oscillation start");
	anim.PrevFrame(); Check(anim.GetAnimationFrame()==0, "previous animation frame");

	cSDLTexture volume("volume", &format, &gfx, eTextureType_Normal, true, eTextureTarget_3D);
	std::array<unsigned char,64> voxels{};
	Check(volume.CreateFromArray(voxels.data(),1,cVector3l(4)), "3D texture upload");
	Immutable(volume, GL_TEXTURE_3D, 1);
	cSDLTexture cube("cube", &format, &gfx, eTextureType_Normal, true, eTextureTarget_CubeMap);
	tBitmap2DVec faces(6,&red);
	Check(cube.CreateCubeFromBitmapVec(&faces), "cube upload"); Immutable(cube, GL_TEXTURE_CUBE_MAP, 3);
	cSDLTexture fog("fog", &format, &gfx, eTextureType_Normal, false, eTextureTarget_1D);
	unsigned char ramp[8]={1,2,3,4,5,6,7,8};
	Check(fog.CreateFromArray(ramp,2,cVector3l(4,1,1)), "luminance-alpha ramp"); Immutable(fog, GL_TEXTURE_1D, 1);
	GLint swizzle=0; glGetTexParameteriv(GL_TEXTURE_1D,GL_TEXTURE_SWIZZLE_A,&swizzle);
	Check(swizzle==GL_GREEN, "luminance-alpha swizzle");
	cSDLTexture target("target", &format, &gfx, eTextureType_RenderTarget, false, eTextureTarget_2D);
	Check(target.Create(4,4,cColor(0)), "render target"); Immutable(target, GL_TEXTURE_2D, 1);
	Check(target.Create(8,8,cColor(0)) && target.GetWidth()==8, "render-target resize"); Immutable(target, GL_TEXTURE_2D, 1);
	gfx.SetTexture(0,&target);gfx.SetTexture(2,&target);
	gfx.SetRenderTarget(&target);
	Check(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"render target after storage allocation");
	Check(target.Create(16,16,cColor(0)),"resize bound render target");gfx.SetRenderTarget(&target);
	for(int unit : {0,2}) {
		glActiveTexture(GL_TEXTURE0+unit);GLint bound=0;
		glGetIntegerv(GL_TEXTURE_BINDING_2D,&bound);
		Check(bound==target.GetHandle(),"storage replacement rebinds all cached texture units");
	}
	glActiveTexture(GL_TEXTURE0);gfx.SetTexture(0,nullptr);gfx.SetTexture(2,nullptr);
	GLint viewport[4];glGetIntegerv(GL_VIEWPORT,viewport);
	Check(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE && viewport[2]==16,
		"reattach resized immutable storage and update viewport for the same resource");
	gfx.SetRenderTarget(nullptr);
	// The size-quality path averages source texels before every array layer
	// is uploaded, using the same dimensions and full mip chain for each.
	cSDLBitmap2D large(SDL_CreateSurface(64,64,SDL_PIXELFORMAT_RGBA32),&format,"SDL");
	for(int y=0;y<64;++y) for(int x=0;x<64;++x) {
		auto* pixel=(unsigned char*)large.GetSurface()->pixels+y*large.GetSurface()->pitch+x*4;
		pixel[0]=(x%2)*255;pixel[1]=(y%2)*255;pixel[2]=0;pixel[3]=255;
	}
	cSDLTexture quality("quality",&format,&gfx,eTextureType_Normal,true,eTextureTarget_2D);
	quality.SetSizeLevel(1);tBitmap2DVec qualityFrames={&large,&large};
	Check(quality.CreateAnimFromBitmapVec(&qualityFrames) && quality.GetWidth()==32,"array texture quality dimensions");
	Immutable(quality,GL_TEXTURE_2D_ARRAY,6);
	std::vector<unsigned char> reduced(32*32*2*4);
	glGetTexImage(GL_TEXTURE_2D_ARRAY,0,GL_RGBA,GL_UNSIGNED_BYTE,reduced.data());
	Check(reduced[0]==128 && reduced[1]==128 && reduced[32*32*4]==128,"array texture quality averages each layer");
	gfx.SetTexture(0,nullptr);
	NoErrors();

	// Render each layer through the engine's actual linked sampler variants.
	cGLSLProgram program("sampling", false);
	Check(program.CreateFromFile("Default", "TEXTURE"), "2D sampler program");
	program.SetMatrixf(eProgramParam::worldViewProj, cMatrixf::Identity);
	GLuint fbo=0, vao=0, vbo=0;
	glGenFramebuffers(1,&fbo); glBindFramebuffer(GL_FRAMEBUFFER,fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,target.GetHandle(),0);
	Check(glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE,"immutable render-target attachment");
	glGenVertexArrays(1,&vao); glBindVertexArray(vao);
	glGenBuffers(1,&vbo); glBindBuffer(GL_ARRAY_BUFFER,vbo);
	float triangle[]={-1,-1,0, 3,-1,0, -1,3,0};
	glBufferData(GL_ARRAY_BUFFER,sizeof(triangle),triangle,GL_STATIC_DRAW);
	glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,0,nullptr); glEnableVertexAttribArray(0);
	glVertexAttrib4f(2,1,1,1,1); glVertexAttrib4f(4,0.5f,0.5f,0,1);
	glViewport(0,0,8,8);
	gfx.SetTexture(0,&anim);
	float currentLayers[8]={};
	for(int layer=0; layer<2; ++layer) {
		currentLayers[0]=layer;
		Check(program.Apply(1,currentLayers), "array sampler variant");
		glDrawArrays(GL_TRIANGLES,0,3);
		unsigned char pixel[4];glReadPixels(4,4,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
		Check(pixel[layer]==255 && pixel[1-layer]==0, "material samples selected animation layer");
	}
	gfx.SetTexture(0,&atlas); Check(program.Apply(),"return to static sampler variant");
	glDrawArrays(GL_TRIANGLES,0,3);
	unsigned char pixel[4];glReadPixels(4,4,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
	Check(pixel[1]>240,"static material samples original 2D target");
	Check(program.Reload() && program.Apply(1,currentLayers),"reload all used sampler variants");
	GLint workingArray=0;glGetIntegerv(GL_CURRENT_PROGRAM,&workingArray);
	GLuint workingBase=program.GetHandle();
	const auto include=std::filesystem::path(cGLSLProgram::GetShaderDirectory())/"hpl1_texture.glsl";
	std::ifstream input(include);std::string original((std::istreambuf_iterator<char>(input)),{});input.close();
	std::ofstream(include,std::ios::app) << "\n#ifdef HPL_ARRAY_0\ninvalid array variant source\n#endif\n";
	Check(!program.Reload() && program.GetHandle()==workingBase, "one failed variant keeps the entire working program set");
	Check(program.Apply(1,currentLayers), "failed reload retains array program");
	GLint retained=0;glGetIntegerv(GL_CURRENT_PROGRAM,&retained);
	Check(retained==workingArray, "failed array variant is not substituted");
	std::ofstream(include) << original;
	Check(program.Reload() && program.Apply(1,currentLayers), "valid include edit recovers all variants");
	glDrawArrays(GL_TRIANGLES,0,3);glReadPixels(4,4,1,1,GL_RGBA,GL_UNSIGNED_BYTE,pixel);
	Check(pixel[1]==255 && pixel[0]==0, "reload preserves material matrix and selected layer");
	for(const char* name : {"light", "ambient"}) {
		cGLSLProgram material(name);
		Check(material.CreateFromFile(name,"NORMAL_MAP SPECULAR SPECULAR_MAP SPOT"), "material sampler layout");
		for(unsigned int mask : {1u,2u,64u,67u})
			Check(material.Apply(mask,currentLayers), "independent diffuse/normal/specular animation arrays");
		Check(material.Reload(), "reload mixed material sampler layouts");
	}
	NoErrors();
	glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);glDeleteFramebuffers(1,&fbo);
	gfx.SetTexture(0,nullptr);
}
static GLint Integer(GLenum name)
{
	GLint value = 0;
	glGetIntegerv(name, &value);
	return value;
}
static void BoundTarget(GLuint framebuffer, int width, int height)
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	Check(Integer(GL_DRAW_FRAMEBUFFER_BINDING) == (GLint)framebuffer &&
			Integer(GL_READ_FRAMEBUFFER_BINDING) == (GLint)framebuffer,
		"restore both framebuffer bindings");
	Check(viewport[0] == 0 && viewport[1] == 0 && viewport[2] == width && viewport[3] == height,
		"restore target viewport");
	Check(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "complete draw target");
}
static void TestBackendComponents(cLowLevelGraphicsSDL &gfx)
{
	// Exercise all GL owners under the caller's EGL context, without an SDL window.
	cGLGraphicsState state;
	cGLRenderTargets targets(state);
	cGpuProfiler profiler;
	cGLStreamDrawing stream(state, profiler);
	cGLShadowTargets shadows(state, targets);
	cGLFinalPasses passes(state, targets, profiler);
	Check(state.Init(), "initialize draw state without SDL");
	stream.Init();
	Check(targets.Init(cVector2l(32, 24), 4), "initialize scene and output targets without SDL");
	Check(targets.GetSamples() >= 2, "exercise multisampled scene");
	shadows.Init();
	const GLuint output = targets.GetOutputFramebuffer();
	BoundTarget(output, 32, 24);
	state.SetDrawMatrices(cDrawMatrices{});
	state.SetFrameUniforms(cMatrixf::Identity, cMatrixf::Identity, cVector3f(0), 0.1f, 100, 1, targets.GetSize());
	if(auto *bound = cGLSLProgram::GetBoundProgram()) bound->UnBind();

	targets.BeginScene();
	const GLuint scene = Integer(GL_DRAW_FRAMEBUFFER_BINDING);
	glClearColor(0.25f, 0.5f, 0.75f, 1);
	glClearDepth(0.375);
	glDepthMask(GL_TRUE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_SCISSOR_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	state.SetAlphaCutoff(0.6f);
	Check(state.PrepareDraw(targets.GetDrawState(shadows.IsActive())) &&
			glIsEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE),
		"alpha to coverage in unresolved scene");
	shadows.BeginShadowMap(false, 0, 64);
	Check(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "spot shadow target");
	Check(state.PrepareDraw(targets.GetDrawState(shadows.IsActive())) &&
			!glIsEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE),
		"shadow draws disable alpha to coverage");
	shadows.EndShadowMap();
	BoundTarget(scene, 32, 24);
	Check(state.PrepareDraw(targets.GetDrawState(false)) && glIsEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE),
		"shadow exit invalidates shader state");
	for(int face = 0; face < 6; ++face)
	{
		shadows.BeginShadowMap(true, face, 32);
		Check(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "cube shadow face");
		shadows.EndShadowMap();
		BoundTarget(scene, 32, 24);
	}
	GLuint shadowTextures[2], shadowSamplers[2];
	for(int i = 0; i < 2; ++i)
	{
		glActiveTexture(GL_TEXTURE0 + cGLSLProgram::kShadowMapUnit + i);
		shadowTextures[i] = Integer(i ? GL_TEXTURE_BINDING_CUBE_MAP : GL_TEXTURE_BINDING_2D);
		GLint sampler = 0;
		glGetIntegeri_v(GL_SAMPLER_BINDING, cGLSLProgram::kShadowMapUnit + i, &sampler);
		shadowSamplers[i] = sampler;
		GLint compare = 0;
		glGetSamplerParameteriv(sampler, GL_TEXTURE_COMPARE_MODE, &compare);
		Check(compare == (i ? GL_NONE : GL_COMPARE_REF_TO_TEXTURE), "shadow comparison/raw sampler bindings");
	}
	glActiveTexture(GL_TEXTURE0);

	glEnable(GL_SCISSOR_TEST);
	glScissor(1, 2, 3, 4);
	glDepthMask(GL_FALSE);
	glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
	targets.ResolveSceneDepth();
	BoundTarget(scene, 32, 24);
	GLboolean mask[4];
	glGetBooleanv(GL_COLOR_WRITEMASK, mask);
	Check(glIsEnabled(GL_SCISSOR_TEST) && !Integer(GL_DEPTH_WRITEMASK) && !mask[0] && mask[1],
		"depth resolve restores scissor and write masks");
	targets.SetSceneColorOnly(true);
	targets.ResolveScene();
	BoundTarget(targets.GetResolvedColorFramebuffer(), 32, 24);
	Check(state.PrepareDraw(targets.GetDrawState(false)) && !glIsEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE),
		"resolved scene disables alpha to coverage");
	glDisable(GL_SCISSOR_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	float color[4];
	glReadPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, color);
	Check(std::fabs(color[0] - 0.25f) < 0.001f && std::fabs(color[2] - 0.75f) < 0.001f,
		"resolve retains scene color");
	targets.SetSceneColorOnly(false);
	float depth = 0;
	glReadPixels(0, 0, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
	Check(std::fabs(depth - 0.375f) < 0.001f, "resolve retains scene depth");
	const GLuint resolved = Integer(GL_DRAW_FRAMEBUFFER_BINDING);
	cSDLPixelFormat copyFormat;
	cSDLTexture copy("scene copy", &copyFormat, &gfx, eTextureType_RenderTarget, false, eTextureTarget_2D);
	Check(copy.Create(32, 24, cColor(0)), "scene copy texture");
	Check(passes.DrawSceneCopy(&copy, cVector2l(0), cVector2l(32, 24)), "copy resolved scene through its pass");
	BoundTarget(resolved, 32, 24);
	targets.SetRenderTarget(&copy);
	glReadPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, color);
	Check(std::fabs(color[0] - 0.25f) < 0.005f && std::fabs(color[2] - 0.75f) < 0.005f, "scene copy retains color");
	targets.SetRenderTarget(nullptr);
	shadows.BeginShadowMap(false, 0, 32);
	shadows.EndShadowMap();
	BoundTarget(resolved, 32, 24);

	cSDLPixelFormat format;
	cSDLTexture texture("component target", &format, &gfx, eTextureType_RenderTarget, false, eTextureTarget_2D);
	Check(texture.Create(12, 10, cColor(0)), "component resource target");
	targets.SetRenderTarget(&texture);
	const GLuint resource = Integer(GL_DRAW_FRAMEBUFFER_BINDING);
	shadows.BeginShadowMap(true, 3, 64);
	shadows.EndShadowMap();
	BoundTarget(resource, 12, 10);
	Check(texture.Create(18, 14, cColor(0)), "resize component resource target");
	targets.SetRenderTarget(&texture);
	BoundTarget(resource, 18, 14);
	cSDLTexture second("second target", &format, &gfx, eTextureType_RenderTarget, false, eTextureTarget_2D);
	Check(second.Create(18, 14, cColor(0)), "second render target");
	iTexture *attachments[] = {&texture, &second};
	targets.SetRenderTargets(attachments, 2);
	BoundTarget(resource, 18, 14);
	Check(Integer(GL_DRAW_BUFFER1) == GL_COLOR_ATTACHMENT1, "multiple draw buffers");
	targets.SetRenderTarget(&texture);
	BoundTarget(resource, 18, 14);
	GLint attachment = 0;
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &attachment);
	Check(attachment == 0 && Integer(GL_DRAW_BUFFER1) == GL_NONE, "single target detaches extra attachments");
	targets.SetRenderTarget(nullptr);
	BoundTarget(resolved, 32, 24);
	// The pass temporarily replaces a linked program, UBO, texture, sampler,
	// masks and capabilities. A following geometry draw must still use the caches.
	const GLuint sampler = state.GetSampler(false, GL_REPEAT, GL_REPEAT, GL_REPEAT);
	state.BindSampler(0, sampler);
	state.SetAlphaCutoff(0);
	Check(state.PrepareDraw(targets.GetDrawState(false)), "prepare draw before output");
	const GLint program = Integer(GL_CURRENT_PROGRAM);
	GLint material = 0;
	glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, 2, &material);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glEnable(GL_SCISSOR_TEST);
	glScissor(1, 2, 3, 4);
	glDepthMask(GL_FALSE);
	glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
	cSmaaSettings smaa;
	smaa.mbActive = true;
	passes.SetSmaa(smaa);
	passes.EndScene();
	BoundTarget(output, 32, 24);
	GLint restoredSampler = 0, restoredMaterial = 0;
	glGetIntegeri_v(GL_SAMPLER_BINDING, 0, &restoredSampler);
	glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, 2, &restoredMaterial);
	glGetBooleanv(GL_COLOR_WRITEMASK, mask);
	Check(Integer(GL_CURRENT_PROGRAM) == program && restoredMaterial == material && restoredSampler == (GLint)sampler,
		"fullscreen passes restore program, material and cached sampler");
	Check(glIsEnabled(GL_SCISSOR_TEST) && glIsEnabled(GL_DEPTH_TEST) && glIsEnabled(GL_BLEND) &&
			glIsEnabled(GL_CULL_FACE) && !Integer(GL_DEPTH_WRITEMASK) && !mask[0] && mask[1],
		"fullscreen passes restore capabilities and write masks");
	glDisable(GL_SCISSOR_TEST);
	glReadPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, color);
	Check(color[0] > 0 && color[2] > color[0], "output pass draws the resolved scene");
	shadows.BeginShadowMap(false, 0, 32);
	shadows.EndShadowMap();
	BoundTarget(output, 32, 24);

	// Repeated render-scale/FSAA changes rebuild scene/SMAA resources together.
	for(int samples : {0, 4, 0})
	{
		targets.SetMultisampling(samples);
		targets.SetRenderScale(samples ? 1.5f : 0.5f);
		targets.BeginScene();
		glClearColor(0.2f, 0.3f, 0.4f, 1);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glClear(GL_COLOR_BUFFER_BIT);
		passes.EndScene();
		BoundTarget(output, 32, 24);
		NoErrors();
	}
	// Stream draws after fullscreen passes must rebind their VAO, not trust a stale cache.
	state.SetDepthTestActive(false);
	state.SetCullActive(false);
	state.SetBlendActive(false);
	stream.DrawTriangles({cVertex(cVector3f(-1, -1, 0), cVector3f(0), cColor(1, 0, 0, 1)),
							 cVertex(cVector3f(3, -1, 0), cVector3f(0), cColor(1, 0, 0, 1)),
							 cVertex(cVector3f(-1, 3, 0), cVector3f(0), cColor(1, 0, 0, 1))},
		targets.GetDrawState(false));
	glReadPixels(10, 10, 1, 1, GL_RGBA, GL_FLOAT, color);
	Check(color[0] > 0.99f && color[1] < 0.01f, "streamed geometry after fullscreen draw");
	stream.DrawLine(cVector3f(-1, 0, 0), cVector3f(1, 0, 0), cColor(0, 1, 0, 1), targets.GetDrawState(false));
	const GLuint sceneColor = targets.GetSceneColor();
	const GLuint streamVao = Integer(GL_VERTEX_ARRAY_BINDING), streamBuffer = Integer(GL_ARRAY_BUFFER_BINDING);
	GLint frameBuffer = 0, stateBuffer = 0;
	glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, 0, &frameBuffer);
	glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, 1, &stateBuffer);
	glActiveTexture(GL_TEXTURE0 + cGLSLProgram::kShadowMapUnit);
	shadowTextures[0] = Integer(GL_TEXTURE_BINDING_2D);
	shadowTextures[1] = Integer(GL_TEXTURE_BINDING_CUBE_MAP);
	glActiveTexture(GL_TEXTURE0);
	passes.Reset();
	shadows.Reset();
	stream.Reset();
	targets.DeleteFramebuffers();
	state.Reset();
	cGLSLProgram::ExitPrograms();
	Check(!glIsFramebuffer(output) && !glIsFramebuffer(resource) && !glIsTexture(sceneColor), "framebuffer owner teardown");
	for(int i = 0; i < 2; ++i) Check(!glIsTexture(shadowTextures[i]) && !glIsSampler(shadowSamplers[i]), "shadow owner teardown");
	Check(!glIsSampler(sampler) && !glIsProgram(program) && !glIsBuffer(frameBuffer) && !glIsBuffer(stateBuffer), "draw-state owner teardown");
	Check(!glIsVertexArray(streamVao) && !glIsBuffer(streamBuffer), "stream owner teardown");
	NoErrors();
}

int hplMain(const hpl::tString &)
{
	EGLDisplay display = eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr);
	if(display == EGL_NO_DISPLAY || !eglInitialize(display, nullptr, nullptr)) return 77;
	eglBindAPI(EGL_OPENGL_API);
	const EGLint attributes[] = {EGL_CONTEXT_MAJOR_VERSION, 3, EGL_CONTEXT_MINOR_VERSION, 3,
		EGL_CONTEXT_OPENGL_PROFILE_MASK, EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT, EGL_NONE};
	EGLContext context = eglCreateContext(display, nullptr, EGL_NO_CONTEXT, attributes);
	if(context == EGL_NO_CONTEXT || !eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, context)) return 77;
	int result=0;
	try {
		Check(gladLoadGL((GLADloadfunc)eglGetProcAddress)!=0,"load OpenGL");
		// Match SDLGraphicsWindow's ARB entry-point loading under EGL. The
		// generated loader only populates these for core 4.2/4.3 contexts.
		auto extension=[](const std::string &name){
			GLint count=0;glGetIntegerv(GL_NUM_EXTENSIONS,&count);
			for(int i=0;i<count;++i) if(name==(const char*)glGetStringi(GL_EXTENSIONS,i)) return true;
			return false;
		};
		if(!GLAD_GL_VERSION_4_2)
		{
			Check(extension("GL_ARB_texture_storage"),"GL 3.3 immutable storage extension");
			glad_glTexStorage1D=(PFNGLTEXSTORAGE1DPROC)eglGetProcAddress("glTexStorage1D");
			glad_glTexStorage2D=(PFNGLTEXSTORAGE2DPROC)eglGetProcAddress("glTexStorage2D");
			glad_glTexStorage3D=(PFNGLTEXSTORAGE3DPROC)eglGetProcAddress("glTexStorage3D");
		}
		if(!GLAD_GL_VERSION_4_3)
		{
			Check(extension("GL_ARB_texture_storage_multisample"),"GL 3.3 immutable multisample extension");
			glad_glTexStorage2DMultisample=(PFNGLTEXSTORAGE2DMULTISAMPLEPROC)eglGetProcAddress("glTexStorage2DMultisample");
		}
		Check(glTexStorage1D && glTexStorage2D && glTexStorage3D && glTexStorage2DMultisample,"storage entry points");
		cLowLevelGraphicsSDL gfx;
		cGLSLProgram::InitPrograms(&gfx);
		auto directory=std::filesystem::temp_directory_path() / ("overture-storage-" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
		std::filesystem::copy(OVERTURE_TEST_SHADER_DIR,directory,std::filesystem::copy_options::recursive);
		cGLSLProgram::SetShaderDirectory(directory.string()+"/");
		TestQuadInstances(gfx);
		TestTextures(gfx);
		TestBackendComponents(gfx);
		std::filesystem::remove_all(directory);
		std::cout << "Renderer storage regressions passed on " << glGetString(GL_RENDERER) << " / GL " << glGetString(GL_VERSION) << '\n';
	} catch(const std::exception& e) { std::cerr << e.what() << '\n';result=1; }
	eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
	eglDestroyContext(display,context);eglTerminate(display);
	return result;
}
