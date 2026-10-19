// This file is part of OpenHPL1, licensed under the GNU GPL v3 or
// later. See COPYING.
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <glad/gl.h>
#include <algorithm>
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
// Initialize the facade's resource owners under the external EGL context.
// Final-pass internals are inspected for the SMAA scissor regression.
#define private public
#include "impl/GLFinalPasses.h"
#include "impl/LowLevelGraphicsSDL.h"
#include "graphics/Renderer3D.h"
#include "resources/Resources.h"
#undef private
#include "impl/GLTexture.h"
#include "impl/GLSLProgram.h"
#include "impl/LowLevelResourcesSDL.h"
#include "graphics/Graphics.h"
#include "graphics/RenderList.h"
#include "graphics/Renderable.h"
#include "graphics/MeshCreator.h"
#include "graphics/RendererPostEffects.h"
#include "resources/GpuProgramManager.h"
#include "resources/TextureManager.h"
#include "resources/ImageManager.h"
#include "resources/ResourceImage.h"
#include "resources/FrameBitmap.h"
#include "resources/FileSearcher.h"
#include <SDL3_image/SDL_image.h>
#include "scene/Light3DPoint.h"
#include "scene/Light3DSpot.h"
#include "math/Math.h"
#include "scene/Camera3D.h"
using namespace hpl;
static void Check(bool ok, const char* message) { if(!ok) throw std::runtime_error(message); }
static void CheckNoGLErrors()
{
	Check(glGetError() == GL_NO_ERROR, "OpenGL error in renderer regression");
}
static void CheckImmutableStorage(iTexture &texture, GLenum target, int levels)
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
	cGLTexture texture("quad texture", &format, &gfx, eTextureType_Normal, false, eTextureTarget_2D);
	Check(texture.CreateFromBitmap(&white),"quad static texture");
	cGLTexture animated("quad animation", &format, &gfx, eTextureType_Normal, false, eTextureTarget_2D);
	tBitmap2DVec frames={&white,&green};Check(animated.CreateAnimFromBitmapVec(&frames),"quad animation array");
	cGLTexture target("quad target", &format, &gfx, eTextureType_RenderTarget, false, eTextureTarget_2D);
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
	auto render = [&](iVertexBuffer &buffer)
	{
		gfx.SetClearColor(cColor(0.1f, 0.2f, 0.3f, 0));
		gfx.ClearScreen();
		buffer.Bind();
		buffer.Draw();
		std::vector<unsigned char> pixels(48 * 48 * 4);
		glReadPixels(0, 0, 48, 48, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
		return pixels;
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
			gfx.SetBlendActive(true);
			gfx.SetBlendFunc(blend.first, blend.second);
			const auto expected = render(*reference), actual = render(*instances);
			int coveredPixels = 0;
			for(size_t i = 0; i < actual.size(); ++i)
			{
				Check(std::abs(int(actual[i]) - int(expected[i])) <= 1,
					"instanced output matches expanded sheets across blend modes/layers");
				if(i % 4 == 3 && actual[i]) ++coveredPixels;
			}
			if(blend.first == eBlendFunc_One && blend.second == eBlendFunc_Zero)
				Check(coveredPixels > 100, "quad comparison has visible coverage");
		}
	}
	// Returning from an instance array must restore kind zero for ordinary
	// indexed and streamed draws; repeat in both directions with debug lines.
	gfx.SetTexture(0,&texture);gfx.SetBlendActive(false);
	instances->Draw(eVertexBufferDrawType_Lines);
	reference->Draw();
	CheckNoGLErrors();
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
	glDeleteQueries(1, &query);
	CheckNoGLErrors();
	gfx.mStream.Reset();Check(!glIsBuffer(sheetElements),"shared sheet topology released by stream owner");gfx.mState.Reset();
}

static GLint GetGLInteger(GLenum name);

// Count real driver calls only inside the submission being checked. Keep this
// instrumentation in the fixture so production draws need no counters.
struct ProgramCalls
{
	static ProgramCalls *active;
	PFNGLUSEPROGRAMPROC useProgram = glad_glUseProgram;
	PFNGLBINDBUFFERBASEPROC bindBufferBase = glad_glBindBufferBase;
	PFNGLBUFFERSUBDATAPROC bufferSubData = glad_glBufferSubData;
	unsigned programs = 0, materials = 0;
	std::vector<std::pair<GLintptr, GLsizeiptr>> uploads;
	ProgramCalls()
	{
		active = this;
		glad_glUseProgram = UseProgram;
		glad_glBindBufferBase = BindBufferBase;
		glad_glBufferSubData = BufferSubData;
	}
	~ProgramCalls()
	{
		glad_glUseProgram = useProgram;
		glad_glBindBufferBase = bindBufferBase;
		glad_glBufferSubData = bufferSubData;
		active = nullptr;
	}
	static void GLAD_API_PTR UseProgram(GLuint program)
	{
		++active->programs;
		active->useProgram(program);
	}
	static void GLAD_API_PTR BindBufferBase(GLenum target, GLuint index, GLuint buffer)
	{
		if(target == GL_UNIFORM_BUFFER && index == 2) ++active->materials;
		active->bindBufferBase(target, index, buffer);
	}
	static void GLAD_API_PTR BufferSubData(GLenum target, GLintptr offset, GLsizeiptr size, const void *data)
	{
		if(target == GL_UNIFORM_BUFFER) active->uploads.emplace_back(offset, size);
		active->bufferSubData(target, offset, size, data);
	}
};
ProgramCalls *ProgramCalls::active = nullptr;

static void TestProgramSubmission()
{
	const auto directory = std::filesystem::path(cGLSLProgram::GetShaderDirectory());
	std::ofstream(directory / "hpl1_SubmissionProbe.vertex") << R"(
void main() { gl_Position = worldViewProj * vec4(0, 0, 0, 1); }
)";
	const std::string fragment = R"(
OUTPUT
void main() { outColor = vec4(ambientColor + vec3(fogStart, fogEnd, gamma), 1); }
)";
	std::ofstream(directory / "hpl1_SubmissionProbe.fragment") << fragment;
	cGLSLProgram probe("submission probe", false), other("submission other", false);
	probe.SetFloat(eProgramParam::fogStart, 0.125f);
	Check(probe.CreateFromFile("SubmissionProbe") && other.CreateFromFile("SubmissionProbe"),
		"compile submission probes");
	Check(probe.Apply(), "initial submission");
	GLint material = 0;
	glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, 2, &material);
	auto checkData = [&] {
		std::array<float, static_cast<unsigned>(eProgramParam::Count) * 4> data;
		glBindBuffer(GL_UNIFORM_BUFFER, material);
		glGetBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(data), data.data());
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		Check(std::equal(data.begin(), data.end(), probe.GetParameterData()),
			"GPU material contents match the complete CPU block, including unchanged slots");
	};
	checkData();
	{
		ProgramCalls calls;
		for(int i = 0; i < 128; ++i) Check(probe.Apply(), "repeat unchanged submission");
		std::cout << "128 unchanged submissions: " << calls.programs << " program binds, "
			<< calls.materials << " material binds, " << calls.uploads.size() << " uploads\n";
		Check(calls.programs == 0 && calls.materials == 0 && calls.uploads.empty(),
			"unchanged submissions avoid program/material rebinds and uploads");
	}
	auto upload = [&](unsigned first, unsigned count) {
		ProgramCalls calls;
		Check(probe.Apply(), "apply changed parameters");
		Check(calls.programs == 0 && calls.materials == 0 && calls.uploads.size() == 1 &&
			calls.uploads[0].first == first * 16 && calls.uploads[0].second == count * 16,
			"one upload covers only the changed parameter interval without rebinding");
	};
	probe.SetFloat(eProgramParam::fogStart, 0.25f);
	upload(static_cast<unsigned>(eProgramParam::fogStart), 1);
	std::cout << "Changed scalar upload: 16 bytes; complete material block: "
		<< static_cast<unsigned>(eProgramParam::Count) * 16 << " bytes\n";
	checkData();
	cMatrixf matrix;
	for(unsigned row = 0; row < 4; ++row)
		for(unsigned col = 0; col < 4; ++col) matrix.m[row][col] = float(row * 4 + col + 1);
	probe.SetMatrixf(eProgramParam::worldViewProj, matrix);
	upload(static_cast<unsigned>(eProgramParam::worldViewProj), 4);
	// Updates arrive in reverse slot order; gaps must keep their old values.
	probe.SetFloat(eProgramParam::gamma, 0.75f);
	probe.SetVec3f(eProgramParam::ambientColor, 0.1f, 0.2f, 0.3f);
	upload(0, static_cast<unsigned>(eProgramParam::gamma) + 1);
	probe.SetFloat(eProgramParam::t, 0.875f);
	upload(static_cast<unsigned>(eProgramParam::t), 1);
	checkData();
	{
		ProgramCalls calls;
		probe.SetFloat(eProgramParam::t, 0.875f);
		Check(probe.Apply() && calls.uploads.empty(), "identical values stay clean after partial upload");
	}
	probe.Bind();
	Check(other.Apply(), "switch to a different program and material");
	Check(cGLSLProgram::GetBoundProgram() == &probe && cGLSLProgram::GetAppliedProgram() == &other,
		"logical selection stays independent of the applied program");
	{
		ProgramCalls calls;
		Check(probe.Apply() && calls.programs == 1 && calls.materials == 1 && calls.uploads.empty(),
			"switching back restores bindings without reuploading clean data");
	}
	Check(GetGLInteger(GL_CURRENT_PROGRAM) == (GLint)probe.GetHandle(), "program binding restored");
	GLint restoredMaterial = 0;
	glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, 2, &restoredMaterial);
	Check(restoredMaterial == material, "material binding restored");
	checkData();
	// External clients either restore their bindings or invalidate the cache.
	glUseProgram(0);
	glBindBufferBase(GL_UNIFORM_BUFFER, 2, 0);
	cGLSLProgram::ClearAppliedProgram();
	Check(probe.Apply(), "recover from explicitly invalidated external bindings");
	glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, 2, &restoredMaterial);
	Check(GetGLInteger(GL_CURRENT_PROGRAM) == (GLint)probe.GetHandle() && restoredMaterial == material,
		"external invalidation restores both bindings");
	probe.SetFloat(eProgramParam::fogStart, 0.625f);
	std::ofstream(directory / "hpl1_SubmissionProbe.fragment") << fragment << "\ninvalid shader edit\n";
	Check(!probe.Reload() && probe.Apply(), "failed reload preserves pending material updates");
	checkData();
	std::ofstream(directory / "hpl1_SubmissionProbe.fragment") << fragment;
	probe.SetFloat(eProgramParam::fogEnd, 0.5f);
	Check(other.Apply() && probe.Reload(), "reload a previously applied but inactive program");
	probe.RebindCurrent();
	Check(GetGLInteger(GL_CURRENT_PROGRAM) == (GLint)probe.GetHandle(), "inactive reload updates restoration handle");
	Check(probe.Reload() && probe.Apply(), "active reload remains usable with cached bindings");
	checkData();
	{
		cGLSLProgram temporary("submission temporary", false);
		Check(temporary.CreateFromFile("SubmissionProbe") && temporary.Apply(), "temporary program applied");
	}
	Check(probe.Apply(), "recover after deleting the applied program and material");
	// A texture layout change switches the linked handle, but shares its UBO.
	cGLSLProgram textured("submission textured", false);
	Check(textured.CreateFromFile("Default", "TEXTURE"), "texture variant submission probe");
	float layers[8] = {};
	Check(textured.Apply(1, layers) && textured.Apply(0, layers), "warm both sampler layouts");
	{
		ProgramCalls calls;
		Check(textured.Apply(1, layers) && textured.Apply(0, layers), "switch warmed sampler layouts");
		Check(calls.programs == 2 && calls.materials == 0, "variant switches retain the shared material binding");
	}
	Check(textured.Apply(1, layers), "select array layout");
	layers[0] = 1;
	Check(textured.Apply(1, layers), "advance animation without switching layouts");
	const GLint layerLocation = glGetUniformLocation(GetGLInteger(GL_CURRENT_PROGRAM), "hpl_TextureLayer0");
	float layer = -1;
	glGetUniformfv(GetGLInteger(GL_CURRENT_PROGRAM), layerLocation, &layer);
	Check(layerLocation >= 0 && layer == 1, "cached bindings still update animation layers");
	cGLSLProgram::ExitPrograms();
	CheckNoGLErrors();
}

static void TestTextures(cLowLevelGraphicsSDL& gfx)
{
	cSDLPixelFormat format;
	cSDLBitmap2D red(SDL_CreateSurface(4,4,SDL_PIXELFORMAT_RGBA32), &format, "SDL");
	cSDLBitmap2D green(SDL_CreateSurface(4,4,SDL_PIXELFORMAT_RGBA32), &format, "SDL");
	red.FillRect(cRect2l(0,0,0,0), cColor(1,0,0,1));
	green.FillRect(cRect2l(0,0,0,0), cColor(0,1,0,1));
	cGLTexture atlas("atlas", &format, &gfx, eTextureType_Normal, true, eTextureTarget_2D);
	Check(atlas.CreateFromBitmap(&red), "initial atlas upload");
	int atlasHandle=atlas.GetHandle();
	Check(atlas.CreateFromBitmap(&green) && atlas.GetHandle()==atlasHandle, "atlas updates reuse immutable storage");
	CheckImmutableStorage(atlas, GL_TEXTURE_2D, 3);
	std::array<unsigned char,64> pixels{};
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	Check(pixels[0]==0 && pixels[1]==255 && pixels[3]==255, "atlas contents updated");
	gfx.SetTexture(0, &atlas);
	unsigned char blue[4]={0,0,255,255};
	atlas.SetPixels2D(0, cVector2l(0), cVector2l(1), eColorDataFormat_RGBA, blue);
	glBindTexture(GL_TEXTURE_2D, atlas.GetHandle());
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
	Check(pixels[2]==255 && pixels[5]==255, "atlas subimage update preserves neighbors");

	cGLTexture anim("animation", &format, &gfx, eTextureType_Normal, true, eTextureTarget_2D);
	tBitmap2DVec frames={&red,&green};
	Check(anim.CreateAnimFromBitmapVec(&frames) && anim.HasAnimation(), "animation array upload");
	// Replacing a lingering 2D binding with array storage must preserve a
	// different array that has since become the unit's selected resource.
	cGLTexture promoted("promoted", &format, &gfx, eTextureType_Normal, false, eTextureTarget_2D);
	Check(promoted.CreateFromBitmap(&red), "initial static texture before animation promotion");
	gfx.SetTexture(2, &promoted);
	gfx.SetTexture(2, &anim);
	Check(promoted.CreateAnimFromBitmapVec(&frames), "promote an unselected static texture to an array");
	const auto promotedBinding = gfx.mState.GetTextureUnitBinding(2);
	GLint selectedArray = 0;
	glActiveTexture(GL_TEXTURE2);
	glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &selectedArray);
	Check(promotedBinding.mlTexture2D == 0 && promotedBinding.mlTextureArray == (GLuint)anim.GetHandle() &&
		selectedArray == anim.GetHandle(), "storage target changes preserve a different selected array");
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, anim.GetHandle());
	CheckImmutableStorage(anim, GL_TEXTURE_2D_ARRAY, 3);
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

	cGLTexture volume("volume", &format, &gfx, eTextureType_Normal, true, eTextureTarget_3D);
	std::array<unsigned char,64> voxels{};
	Check(volume.CreateFromArray(voxels.data(),1,cVector3l(4)), "3D texture upload");
	CheckImmutableStorage(volume, GL_TEXTURE_3D, 1);
	cGLTexture cube("cube", &format, &gfx, eTextureType_Normal, true, eTextureTarget_CubeMap);
	tBitmap2DVec faces(6,&red);
	Check(cube.CreateCubeFromBitmapVec(&faces), "cube upload");
	CheckImmutableStorage(cube, GL_TEXTURE_CUBE_MAP, 3);
	cGLTexture fog("fog", &format, &gfx, eTextureType_Normal, false, eTextureTarget_1D);
	unsigned char ramp[8]={1,2,3,4,5,6,7,8};
	Check(fog.CreateFromArray(ramp, 2, cVector3l(4, 1, 1)), "luminance-alpha ramp");
	CheckImmutableStorage(fog, GL_TEXTURE_1D, 1);
	GLint swizzle=0; glGetTexParameteriv(GL_TEXTURE_1D,GL_TEXTURE_SWIZZLE_A,&swizzle);
	Check(swizzle==GL_GREEN, "luminance-alpha swizzle");
	cGLTexture target("target", &format, &gfx, eTextureType_RenderTarget, false, eTextureTarget_2D);
	Check(target.Create(4, 4, cColor(0)), "render target");
	CheckImmutableStorage(target, GL_TEXTURE_2D, 1);
	Check(target.Create(8, 8, cColor(0)) && target.GetWidth() == 8, "render-target resize");
	CheckImmutableStorage(target, GL_TEXTURE_2D, 1);
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
	cGLTexture quality("quality", &format, &gfx, eTextureType_Normal, true, eTextureTarget_2D);
	quality.SetSizeLevel(1);tBitmap2DVec qualityFrames={&large,&large};
	Check(quality.CreateAnimFromBitmapVec(&qualityFrames) && quality.GetWidth()==32,"array texture quality dimensions");
	CheckImmutableStorage(quality, GL_TEXTURE_2D_ARRAY, 6);
	std::vector<unsigned char> reduced(32*32*2*4);
	glGetTexImage(GL_TEXTURE_2D_ARRAY,0,GL_RGBA,GL_UNSIGNED_BYTE,reduced.data());
	Check(reduced[0]==128 && reduced[1]==128 && reduced[32*32*4]==128,"array texture quality averages each layer");
	gfx.SetTexture(0,nullptr);
	CheckNoGLErrors();

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
	CheckNoGLErrors();
	glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);glDeleteFramebuffers(1,&fbo);
	gfx.SetTexture(0,nullptr);
}
static GLint GetGLInteger(GLenum name)
{
	GLint value = 0;
	glGetIntegerv(name, &value);
	return value;
}
static void CheckBoundTarget(GLuint framebuffer, int width, int height)
{
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	Check(GetGLInteger(GL_DRAW_FRAMEBUFFER_BINDING) == (GLint)framebuffer &&
			GetGLInteger(GL_READ_FRAMEBUFFER_BINDING) == (GLint)framebuffer,
		"restore both framebuffer bindings");
	Check(viewport[0] == 0 && viewport[1] == 0 && viewport[2] == width && viewport[3] == height,
		"restore target viewport");
	Check(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "complete draw target");
}
static void TestShadowStorage()
{
	cGLGraphicsState state;
	cGLRenderTargets targets(state);
	cGLShadowTargets shadows(state, targets);
	Check(state.Init() && targets.Init(cVector2l(32, 24), 4), "initialize shadow cache owners");
	shadows.Init();
	targets.BeginScene();
	const GLuint scene = GetGLInteger(GL_DRAW_FRAMEBUFFER_BINDING);
	std::vector<uint64_t> signature = {11};
	const size_t mapBytes = 32 * 32 * 4 + sizeof(uint64_t);
	auto texture = [&](bool cube) {
		glActiveTexture(GL_TEXTURE0 + cGLSLProgram::kShadowMapUnit);
		GLuint handle = GetGLInteger(cube ? GL_TEXTURE_BINDING_CUBE_MAP : GL_TEXTURE_BINDING_2D);
		glActiveTexture(GL_TEXTURE0 + state.GetActiveTextureUnit());
		return handle;
	};
	auto fill = [&](bool cube, int face, int size, float value) {
		shadows.BeginShadowMap(cube, face, size);
		glClearDepth(value);
		glClear(GL_DEPTH_BUFFER_BIT);
		shadows.EndShadowMap();
		CheckBoundTarget(scene, 32, 24);
	};
	Check(shadows.SelectShadowMap(1, false, 32, signature, mapBytes * 2) == 0, "new cache entry is invalid");
	const GLuint first = texture(false);
	fill(false, 0, 32, 0.25f);
	Check(shadows.SelectShadowMap(2, false, 32, signature, mapBytes * 2) == 0, "second light has independent storage");
	const GLuint second = texture(false);
	fill(false, 0, 32, 0.75f);
	Check(first != second && shadows.SelectShadowMap(1, false, 32, signature, mapBytes * 2) == 1,
		"unchanged light restores its valid map after drawing another light");
	CheckBoundTarget(scene, 32, 24);
	std::array<float, 32 * 32> depth;
	glActiveTexture(GL_TEXTURE0 + cGLSLProgram::kShadowMapUnit);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, GL_FLOAT, depth.data());
	glActiveTexture(GL_TEXTURE0 + state.GetActiveTextureUnit());
	Check(std::fabs(depth[0] - 0.25f) < 1e-5f, "cached depth pixels survive another light");
	shadows.SelectShadowMap(3, false, 32, signature, mapBytes * 2);
	Check(glIsTexture(first) && (!glIsTexture(second) || second == texture(false)),
		"LRU eviction preserves the most recently reused light");
	Check(shadows.GetCacheEntries() == 2 && shadows.GetCacheBytes() <= mapBytes * 2, "shadow cache obeys its byte budget");
	// Shrinking the budget may evict the requested old entry as well.
	shadows.SelectShadowMap(1, false, 32, signature, mapBytes);
	Check(shadows.GetCacheEntries() == 1 && shadows.GetCacheBytes() <= mapBytes, "budget reduction also accounts for reinsertion");
	const size_t budget = 1024 * 1024;
	Check(shadows.SelectShadowMap(4, true, 16, signature, budget) == 0, "new cube has no valid faces");
	fill(true, 0, 16, 0.125f);
	Check(shadows.SelectShadowMap(4, true, 16, signature, budget) == 1, "only the rendered cube face is valid");
	fill(true, 5, 16, 0.875f);
	Check(shadows.SelectShadowMap(4, true, 16, signature, budget) == 33, "newly exposed face preserves existing faces");
	std::array<float, 16 * 16> cubeDepth;
	glActiveTexture(GL_TEXTURE0 + cGLSLProgram::kShadowMapUnit);
	glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_DEPTH_COMPONENT, GL_FLOAT, cubeDepth.data());
	glActiveTexture(GL_TEXTURE0 + state.GetActiveTextureUnit());
	Check(std::fabs(cubeDepth[0] - 0.125f) < 1e-5f, "rendering another cube face preserves earlier depth");
	signature[0]++;
	Check(shadows.SelectShadowMap(4, true, 16, signature, budget) == 0, "changed inputs invalidate every cube face");
	fill(true, 0, 16, 0.5f);
	const GLuint beforeMembershipChange = texture(true);
	signature.push_back(99);
	Check(shadows.SelectShadowMap(4, true, 16, signature, budget) == 0 && texture(true) == beforeMembershipChange,
		"caster-list size changes invalidate depth without reallocating its storage");
	signature.resize(1);
	Check(shadows.SelectShadowMap(4, true, 16, signature, budget) == 0 && texture(true) == beforeMembershipChange,
		"shrinking the signature releases CPU storage while retaining the depth texture");
	Check(shadows.SelectShadowMap(4, true, 32, signature, budget) == 0, "resizing replaces cached storage and validity");
	shadows.SelectShadowMap(4, true, 32, signature, 0);
	Check(shadows.GetCacheBytes() == 0 && shadows.GetCacheEntries() == 0, "zero budget releases retained maps");
	fill(true, 0, 32, 0.5f);
	Check(shadows.SelectShadowMap(4, true, 32, signature, 0) == 0, "scratch maps never claim reusable depth");
	for(uint64_t id = 100; id < 165; ++id) shadows.SelectShadowMap(id, false, 16, signature, budget);
	Check(shadows.GetCacheEntries() == 64, "small-map metadata also has a bounded entry count");
	const GLuint retained = texture(false);
	shadows.Reset();
	Check(!glIsTexture(retained) && shadows.GetCacheBytes() == 0, "cache textures released before context teardown");
	CheckNoGLErrors();
}

static void TestBackendComponents(cLowLevelGraphicsSDL &gfx)
{
	// Exercise all GL owners under the caller's EGL context, without an SDL window.
	cGLGraphicsState state;
	cGLRenderTargets targets(state);
	cGpuProfiler profiler;
	cGLStreamDrawing stream(state, profiler);
	stream.SetRenderTargets(&targets);
	cGLShadowTargets shadows(state, targets);
	cGLFinalPasses passes(state, targets, profiler);
	Check(state.Init(), "initialize draw state without SDL");
	stream.Init();
	Check(targets.Init(cVector2l(32, 24), 4), "initialize scene and output targets without SDL");
	Check(targets.GetSamples() >= 2, "exercise multisampled scene");
	shadows.Init();
	const GLuint output = targets.GetOutputFramebuffer();
	CheckBoundTarget(output, 32, 24);
	state.SetDrawMatrices(cDrawMatrices{});
	state.SetFrameUniforms(cMatrixf::Identity, cMatrixf::Identity, cVector3f(0), 0.1f, 100, 1, targets.GetSize());
	if(auto *bound = cGLSLProgram::GetBoundProgram()) bound->UnBind();

	targets.BeginScene();
	const GLuint scene = GetGLInteger(GL_DRAW_FRAMEBUFFER_BINDING);
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

	// Scene depth is cached independently of the colour resolve.
	unsigned int depthBlits = targets.GetSceneDepthResolveCount();
	Check(!targets.IsSceneDepthResolved(), "new scene has unresolved depth");
	targets.ResolveSceneDepth();
	Check(targets.IsSceneDepthResolved() && targets.GetSceneDepthResolveCount() == depthBlits + 1,
		"first depth read performs one blit");
	targets.ResolveSceneDepth();
	Check(targets.GetSceneDepthResolveCount() == depthBlits + 1, "repeated depth read is free");
	state.SetDepthWriteActive(false);
	Check(state.PrepareDraw(targets.GetDrawState(false)), "prepare with depth writes disabled");
	Check(targets.GetSceneDepthResolveCount() == depthBlits + 1,
		"preparing a draw is not a completed depth write");
	state.SetDepthWriteActive(true);
	state.SetColorWriteActive(false, false, false, false);
	stream.DrawTriangles({cVertex(cVector3f(0.7f, 0.7f, -0.5f), cVector3f(0), cColor(1, 0, 0, 1)),
							 cVertex(cVector3f(0.95f, 0.7f, -0.5f), cVector3f(0), cColor(1, 0, 0, 1)),
							 cVertex(cVector3f(0.7f, 0.95f, -0.5f), cVector3f(0), cColor(1, 0, 0, 1))},
		targets.GetDrawState(false));
	state.SetColorWriteActive(true, true, true, true);
	Check(!targets.IsSceneDepthResolved(), "a completed depth-writing draw invalidates the copy");
	targets.ResolveSceneDepth();
	Check(targets.GetSceneDepthResolveCount() == depthBlits + 2,
		"a genuine depth write becomes visible to the next reader");
	// A draw with depth testing off cannot write depth and keeps the copy.
	state.SetDepthTestActive(false);
	stream.DrawTriangles({cVertex(cVector3f(-0.95f, 0.7f, 0), cVector3f(0), cColor(0, 1, 0, 1)),
							 cVertex(cVector3f(-0.7f, 0.7f, 0), cVector3f(0), cColor(0, 1, 0, 1)),
							 cVertex(cVector3f(-0.95f, 0.95f, 0), cVector3f(0), cColor(0, 1, 0, 1))},
		targets.GetDrawState(false));
	Check(targets.IsSceneDepthResolved() && targets.GetSceneDepthResolveCount() == depthBlits + 2,
		"colour-only draws keep resolved depth");
	state.SetDepthTestActive(true);
	shadows.BeginShadowMap(false, 0, 64);
	Check(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "spot shadow target");
	Check(state.PrepareDraw(targets.GetDrawState(shadows.IsActive())) &&
			!glIsEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE),
		"shadow draws disable alpha to coverage");
	shadows.EndShadowMap();
	CheckBoundTarget(scene, 32, 24);
	Check(state.PrepareDraw(targets.GetDrawState(false)) && glIsEnabled(GL_SAMPLE_ALPHA_TO_COVERAGE),
		"shadow exit invalidates shader state");
	Check(targets.IsSceneDepthResolved() && targets.GetSceneDepthResolveCount() == depthBlits + 2,
		"shadow work does not invalidate scene depth");
	for(int face = 0; face < 6; ++face)
	{
		shadows.BeginShadowMap(true, face, 32);
		Check(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "cube shadow face");
		shadows.EndShadowMap();
		CheckBoundTarget(scene, 32, 24);
	}
	GLuint shadowTextures[2], shadowSamplers[2];
	for(int i = 0; i < 2; ++i)
	{
		glActiveTexture(GL_TEXTURE0 + cGLSLProgram::kShadowMapUnit + i);
		shadowTextures[i] = GetGLInteger(i ? GL_TEXTURE_BINDING_CUBE_MAP : GL_TEXTURE_BINDING_2D);
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
	CheckBoundTarget(scene, 32, 24);
	GLboolean mask[4];
	glGetBooleanv(GL_COLOR_WRITEMASK, mask);
	Check(glIsEnabled(GL_SCISSOR_TEST) && !GetGLInteger(GL_DEPTH_WRITEMASK) && !mask[0] && mask[1],
		"depth resolve restores scissor and write masks");
	targets.SetSceneColorOnly(true);
	targets.ResolveScene();
	CheckBoundTarget(targets.GetResolvedColorFramebuffer(), 32, 24);
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
	// Once colour is resolved, depth writes go to the resolve attachment.
	// Asking for depth again must not copy the older multisampled depth over.
	const unsigned int afterResolve = targets.GetSceneDepthResolveCount();
	state.SetDepthTestActive(true);
	state.SetDepthWriteActive(true);
	state.SetColorWriteActive(false, false, false, false);
	stream.DrawTriangles({cVertex(cVector3f(0.7f, 0.7f, -0.5f), cVector3f(0), cColor(1, 0, 0, 1)),
							 cVertex(cVector3f(0.95f, 0.7f, -0.5f), cVector3f(0), cColor(1, 0, 0, 1)),
							 cVertex(cVector3f(0.7f, 0.95f, -0.5f), cVector3f(0), cColor(1, 0, 0, 1))},
		targets.GetDrawState(false));
	state.SetColorWriteActive(true, true, true, true);
	targets.ResolveSceneDepth();
	Check(targets.GetSceneDepthResolveCount() == afterResolve,
		"post-resolve depth write is not overwritten by an older scene copy");
	glReadPixels(28, 20, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &depth);
	Check(std::fabs(depth - 0.25f) < 0.01f, "newer resolved depth survives the depth request");
	const GLuint resolved = GetGLInteger(GL_DRAW_FRAMEBUFFER_BINDING);
	cSDLPixelFormat copyFormat;
	cGLTexture copy("scene copy", &copyFormat, &gfx, eTextureType_RenderTarget, false, eTextureTarget_2D);
	Check(copy.Create(32, 24, cColor(0)), "scene copy texture");
	Check(passes.DrawSceneCopy(&copy, cVector2l(0), cVector2l(32, 24)), "copy resolved scene through its pass");
	CheckBoundTarget(resolved, 32, 24);
	targets.SetRenderTarget(&copy);
	glReadPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, color);
	Check(std::fabs(color[0] - 0.25f) < 0.005f && std::fabs(color[2] - 0.75f) < 0.005f, "scene copy retains color");
	targets.SetRenderTarget(nullptr);
	shadows.BeginShadowMap(false, 0, 32);
	shadows.EndShadowMap();
	CheckBoundTarget(resolved, 32, 24);
	// Offscreen resource-target work is not the scene's depth.
	const unsigned int beforeResource = targets.GetSceneDepthResolveCount();
	const bool bDepthWasResolved = targets.IsSceneDepthResolved();

	cSDLPixelFormat format;
	cGLTexture texture("component target", &format, &gfx, eTextureType_RenderTarget, false, eTextureTarget_2D);
	Check(texture.Create(12, 10, cColor(0)), "component resource target");
	targets.SetRenderTarget(&texture);
	const GLuint resource = GetGLInteger(GL_DRAW_FRAMEBUFFER_BINDING);
	shadows.BeginShadowMap(true, 3, 64);
	shadows.EndShadowMap();
	CheckBoundTarget(resource, 12, 10);
	Check(texture.Create(18, 14, cColor(0)), "resize component resource target");
	targets.SetRenderTarget(&texture);
	CheckBoundTarget(resource, 18, 14);
	cGLTexture second("second target", &format, &gfx, eTextureType_RenderTarget, false, eTextureTarget_2D);
	Check(second.Create(18, 14, cColor(0)), "second render target");
	iTexture *attachments[] = {&texture, &second};
	targets.SetRenderTargets(attachments, 2);
	CheckBoundTarget(resource, 18, 14);
	Check(GetGLInteger(GL_DRAW_BUFFER1) == GL_COLOR_ATTACHMENT1, "multiple draw buffers");
	targets.SetRenderTarget(&texture);
	CheckBoundTarget(resource, 18, 14);
	GLint attachment = 0;
	glGetFramebufferAttachmentParameteriv(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &attachment);
	Check(attachment == 0 && GetGLInteger(GL_DRAW_BUFFER1) == GL_NONE, "single target detaches extra attachments");
	targets.SetRenderTarget(nullptr);
	CheckBoundTarget(resolved, 32, 24);
	Check(targets.GetSceneDepthResolveCount() == beforeResource && targets.IsSceneDepthResolved() == bDepthWasResolved,
		"resource-target work does not touch scene depth");
	// A 2D-array texture on unit 0 must come back as an array binding, not
	// as the 2D target the fullscreen pass leaves there.
	cSDLPixelFormat arrayFormat;
	cSDLBitmap2D arrayRed(SDL_CreateSurface(2,2,SDL_PIXELFORMAT_RGBA32), &arrayFormat, "SDL");
	cSDLBitmap2D arrayGreen(SDL_CreateSurface(2,2,SDL_PIXELFORMAT_RGBA32), &arrayFormat, "SDL");
	arrayRed.FillRect(cRect2l(0,0,0,0), cColor(1,0,0,1));
	arrayGreen.FillRect(cRect2l(0,0,0,0), cColor(0,1,0,1));
	cGLTexture array("restore array", &arrayFormat, &gfx, eTextureType_Normal, false, eTextureTarget_2D);
	tBitmap2DVec arrayFrames={&arrayRed,&arrayGreen};
	Check(array.CreateAnimFromBitmapVec(&arrayFrames), "array texture for pass restoration");
	// Default-program selection follows the current texture target; a cube
	// default must not stay selected after the cube texture is replaced.
	cGLTexture cube("restore cube", &arrayFormat, &gfx, eTextureType_Normal, false, eTextureTarget_CubeMap);
	tBitmap2DVec cubeFaces(6, &arrayRed);
	Check(cube.CreateCubeFromBitmapVec(&cubeFaces), "cube texture for default selection");
	state.SetTexture(0, &cube);
	Check(state.PrepareDraw(targets.GetDrawState(false)) &&
			cGLSLProgram::GetAppliedProgram() &&
			cGLSLProgram::GetAppliedProgram()->GetProgramName() == "Default CUBE",
		"cube texture selects the cube default");
	state.SetTexture(0, &array);
	Check(state.PrepareDraw(targets.GetDrawState(false)) &&
			cGLSLProgram::GetAppliedProgram() &&
			cGLSLProgram::GetAppliedProgram()->GetProgramName() == "Default TEXTURE",
		"a later array texture leaves the cube default behind");
	// Replacement while unbound must also update the cached handle.
	cGLTexture resized("resized", &arrayFormat, &gfx, eTextureType_RenderTarget, false, eTextureTarget_2D);
	Check(resized.Create(2, 2, cColor(0)), "texture before unbound resize");
	state.SetTexture(0, &resized);
	state.SetTexture(0, nullptr);
	const GLuint beforeResize = resized.GetTextureHandle();
	Check(resized.Create(4, 4, cColor(0)), "texture resized while unbound");
	// The fixture texture uses the facade's owners; notify the local state
	// the way cLowLevelGraphicsSDL would.
	state.TextureStorageChanged(&resized, beforeResize);
	Check(state.GetTextureUnitBinding(0).mlTexture2D == resized.GetTextureHandle(),
		"storage replacement updates a cached handle without the resource pointer");

	// A deleted texture must not be restored by a fullscreen pass.
	cGLTexture *pDeleted = hplNew(cGLTexture, ("deleted", &arrayFormat, &gfx, eTextureType_RenderTarget, false, eTextureTarget_2D));
	Check(pDeleted->Create(2, 2, cColor(0)), "texture deleted before a pass");
	state.SetTexture(0, pDeleted);
	// Unbinding keeps the GL texture bound; deletion still has to clear the
	// cached handle that a later pass would otherwise restore.
	state.SetTexture(0, nullptr);
	state.ForgetTexture(pDeleted);
	delete pDeleted;
	state.SetTexture(0, &array);
	// The pass temporarily replaces a linked program, UBO, texture, sampler,
	// masks and capabilities. A following geometry draw must still use the caches.
	const GLuint sampler = state.GetSampler(false, GL_REPEAT, GL_REPEAT, GL_REPEAT);
	state.BindSampler(0, sampler);
	state.SetAlphaCutoff(0);
	Check(state.PrepareDraw(targets.GetDrawState(false)), "prepare draw before output");
	const GLint program = GetGLInteger(GL_CURRENT_PROGRAM);
	GLint material = 0;
	glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, 2, &material);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glEnable(GL_CULL_FACE);
	glEnable(GL_SCISSOR_TEST);
	glScissor(1, 2, 3, 4);
	glDepthMask(GL_FALSE);
	glColorMask(GL_FALSE, GL_TRUE, GL_FALSE, GL_TRUE);
	// These changes are deliberately raw, outside the owner. Declare that
	// boundary so the pass restores the state that is actually current.
	state.ResyncExternalState();
	cSmaaSettings smaa;
	smaa.mbActive = true;
	passes.SetSmaa(smaa);
	passes.EndScene();
	CheckBoundTarget(output, 32, 24);
	GLint restoredSampler = 0, restoredMaterial = 0;
	glGetIntegeri_v(GL_SAMPLER_BINDING, 0, &restoredSampler);
	glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, 2, &restoredMaterial);
	glGetBooleanv(GL_COLOR_WRITEMASK, mask);
	Check(GetGLInteger(GL_CURRENT_PROGRAM) == program && restoredMaterial == material && restoredSampler == (GLint)sampler,
		"fullscreen passes restore program, material and cached sampler");
	glActiveTexture(GL_TEXTURE0);
	GLint restoredArray = 0, restoredTexture2D = 0;
	glGetIntegerv(GL_TEXTURE_BINDING_2D_ARRAY, &restoredArray);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, &restoredTexture2D);
	Check(restoredArray == (GLint)array.GetHandle(),
		"fullscreen passes restore an animation-array binding on a shared unit");
	Check(restoredTexture2D == 0,
		"fullscreen passes do not restore a deleted texture handle");
	// RebindCurrent must not use a handle replaced by a reload.
	cGLSLProgram rebindProbe("rebind probe", false);
	Check(rebindProbe.CreateFromFile("Default"), "program for reload rebind");
	Check(rebindProbe.Apply(), "apply before reload");
	Check(rebindProbe.Reload(), "reload before rebind");
	rebindProbe.RebindCurrent();
	GLint currentHandle = 0;
	glGetIntegerv(GL_CURRENT_PROGRAM, &currentHandle);
	Check(currentHandle == (GLint)rebindProbe.GetHandle(),
		"rebind restores the variant handle produced by the reload");
	cGLSLProgram::ExitPrograms();
	Check(glIsEnabled(GL_SCISSOR_TEST) && glIsEnabled(GL_DEPTH_TEST) && glIsEnabled(GL_BLEND) &&
			glIsEnabled(GL_CULL_FACE) && !GetGLInteger(GL_DEPTH_WRITEMASK) && !mask[0] && mask[1],
		"fullscreen passes restore capabilities and write masks");
	state.SetScissorActive(false);
	glReadPixels(0, 0, 1, 1, GL_RGBA, GL_FLOAT, color);
	Check(color[0] > 0 && color[2] > color[0], "output pass draws the resolved scene");

	// EndScene must disable scissor through the state owner. A raw disable
	// leaves the cache on, so the first fullscreen pass restores it and the
	// SMAA clears stay scissored, leaving stale edge data outside the rect.
	GLint previousRead = 0, previousDraw = 0;
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousRead);
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDraw);
	glBindFramebuffer(GL_FRAMEBUFFER, passes.mvSmaaFramebuffers[1]);
	GLfloat vOldClear[4];
	glGetFloatv(GL_COLOR_CLEAR_VALUE, vOldClear);
	glClearColor(1.0f, 0.2f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glClearColor(vOldClear[0], vOldClear[1], vOldClear[2], vOldClear[3]);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, previousRead);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, previousDraw);
	state.SetScissorActive(true);
	glScissor(1, 2, 3, 4);
	passes.EndScene();
	glBindFramebuffer(GL_READ_FRAMEBUFFER, passes.mvSmaaFramebuffers[1]);
	glReadPixels(24, 4, 1, 1, GL_RGBA, GL_FLOAT, color);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, previousRead);
	Check(color[0] < 0.05f && color[1] < 0.05f,
		"SMAA clears are not scissored by restored fullscreen state");
	state.SetScissorActive(false);
	shadows.BeginShadowMap(false, 0, 32);
	shadows.EndShadowMap();
	CheckBoundTarget(output, 32, 24);

	// Repeated render-scale/FSAA changes rebuild scene/SMAA resources together.
	for(int samples : {0, 4, 0})
	{
		targets.SetMultisampling(samples);
		targets.SetRenderScale(samples ? 1.5f : 0.5f);
		targets.BeginScene();
		Check(!targets.IsSceneDepthResolved(), "every new frame starts with unresolved depth");
		const unsigned int beforeResolve = targets.GetSceneDepthResolveCount();
		targets.ResolveSceneDepth();
		Check(targets.GetSceneDepthResolveCount() == beforeResolve + (samples > 0 ? 1u : 0u),
			"depth resolve follows the current multisample setting");
		glClearColor(0.2f, 0.3f, 0.4f, 1);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		state.ResyncExternalState();
		glClear(GL_COLOR_BUFFER_BIT);
		passes.EndScene();
		CheckBoundTarget(output, 32, 24);
		CheckNoGLErrors();
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
	const GLuint streamVao = GetGLInteger(GL_VERTEX_ARRAY_BINDING), streamBuffer = GetGLInteger(GL_ARRAY_BUFFER_BINDING);
	GLint frameBuffer = 0, stateBuffer = 0;
	glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, 0, &frameBuffer);
	glGetIntegeri_v(GL_UNIFORM_BUFFER_BINDING, 1, &stateBuffer);
	glActiveTexture(GL_TEXTURE0 + cGLSLProgram::kShadowMapUnit);
	shadowTextures[0] = GetGLInteger(GL_TEXTURE_BINDING_2D);
	shadowTextures[1] = GetGLInteger(GL_TEXTURE_BINDING_CUBE_MAP);
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
	CheckNoGLErrors();
}

// Run the actual transparent executor with ordinary, custom and refractive
// draws. The small shaders make the expected blend/copy order measurable.
class TransparentResources : public cResources
{
public:
	TransparentResources(iLowLevelResources *low, iLowLevelGraphics *gfx) : cResources(low, gfx)
	{
		mpLowLevelSystem = nullptr;
		mpImageManager = nullptr; mpParticleManager = nullptr; mpSoundManager = nullptr;
		mpFontManager = nullptr; mpScriptManager = nullptr; mpTextureManager = nullptr;
		mpMaterialManager = nullptr; mpSoundEntityManager = nullptr; mpAnimationManager = nullptr;
		mpVideoManager = nullptr; mpMeshManager = nullptr; mpMeshLoaderHandler = nullptr;
		mpGpuProgramManager = hplNew(cGpuProgramManager, (mpFileSearcher, gfx, low, nullptr));
	}
};
class TransparentMaterial : public iMaterial
{
public:
	TransparentMaterial(cLowLevelGraphicsSDL &gfx, cRenderer3D &renderer)
		: iMaterial("transparent fixture", &gfx, nullptr, nullptr, nullptr, eMaterialPicture_Texture, &renderer)
	{
		mbIsTransparent = true;
	}
	~TransparentMaterial() override { for(auto &texture : mvTexture) texture = nullptr; }
	void Compile() override {}
	iGpuProgram *program = nullptr;
	iMaterialProgramSetup *setup = nullptr;
	iGpuProgram *refraction = nullptr;
	iGpuProgram *GetProgram(eMaterialRenderType, int, iLight3D*) override { return program; }
	iMaterialProgramSetup *GetProgramSetup(eMaterialRenderType, int, iLight3D*) override { return setup; }
	eMaterialBlendMode GetBlendMode(eMaterialRenderType, int, iLight3D*) override { return eMaterialBlendMode_Alpha; }
	iGpuProgram *GetRefractionProgram() override { return refraction; }
	bool GetRefractionSkipsStandardTrans() override { return true; }
};
class TransparentObject : public iRenderable
{
public:
	TransparentObject(TransparentMaterial &material, iVertexBuffer *vertices)
		: iRenderable("transparent fixture"), material(material), vertices(vertices) {}
	TransparentMaterial &material;
	iVertexBuffer *vertices;
	iGpuProgram *custom = nullptr;
	int queries = 0, setups = 0;
	iMaterial *GetMaterial() override { return &material; }
	iVertexBuffer *GetVertexBuffer() override { return vertices; }
	bool IsShadowCaster() override { return false; }
	cMatrixf *GetModelMatrix(cCamera3D*) override { return nullptr; }
	eRenderableType GetRenderType() override { return eRenderableType_Normal; }
	int GetMatrixUpdateCount() override { return 0; }
	tString GetEntityType() override { return "TransparentFixture"; }
	bool GetTransDraw(cCamera3D*, cTransDraw &draw) override
	{
		++queries;
		if(!custom) return false;
		draw.mpProgram = custom;
		draw.mbDepthTest = false;
		draw.mbCull = false;
		return true;
	}
	void SetupTransDraw(cCamera3D*, cRenderSettings *settings, cTransDraw&) override
	{
		++setups;
		Check(settings->mpProgram == custom && cGLSLProgram::GetBoundProgram() == custom,
			"custom setup runs after selecting its program");
		Check(!glIsEnabled(GL_DEPTH_TEST), "custom draw overrides depth testing at execution");
	}
};
class TransparentSetup : public iMaterialProgramSetup
{
public:
	int calls = 0, matrices = 0;
	void Setup(iGpuProgram*, cRenderSettings*) override { ++calls; }
	void SetupMatrix(cMatrixf*, cRenderSettings*) override { ++matrices; }
};
static void TestTransparentExecution(cLowLevelGraphicsSDL &gfx)
{
	Check(gfx.mState.Init(), "initialize transparent draw state");
	gfx.mStream.Init();
	gfx.mStream.SetRenderTargets(&gfx.mTargets);
	Check(gfx.mTargets.Init(cVector2l(32, 24), 0), "initialize transparent scene");
	const auto directory = std::filesystem::path(cGLSLProgram::GetShaderDirectory());
	const char *vertex = "out vec4 vColor; void main(){gl_Position=worldViewProj*hpl_Vertex;vColor=hpl_Color;}";
	std::ofstream(directory/"hpl1_TransparentProbe.vertex") << vertex;
	std::ofstream(directory/"hpl1_TransparentProbe.fragment") <<
		"in vec4 vColor; OUTPUT void main(){outColor=vColor;}";
	std::ofstream(directory/"hpl1_TransparentRead.vertex") << vertex;
	std::ofstream(directory/"hpl1_TransparentRead.fragment") <<
		"HPL_TEXTURE_2D(0) OUTPUT void main(){outColor=hpl_TexelFetch0(ivec2(gl_FragCoord.xy),0)*vec4(1,0.5,1,1);}";
	cGLSLProgram flat("transparent flat", false), custom("transparent custom", false), read("transparent read", false);
	Check(flat.CreateFromFile("TransparentProbe") && custom.CreateFromFile("TransparentProbe") &&
		read.CreateFromFile("TransparentRead"), "compile transparent order probes");
	cLowLevelResourcesSDL low(&gfx);
	TransparentResources resources(&low, &gfx);
	cGraphics graphics(&gfx, &low);
	cRenderList list(&graphics);
	cMeshCreator meshes(&gfx, &resources);
	cRenderer3D renderer(&gfx, &resources, &meshes, &list);
	cRendererPostEffects effects(&gfx, &resources, &list, &renderer);
	renderer.SetPostEffects(&effects);
	cCamera3D camera;
	list.SetCamera(&camera);
	renderer.GetRenderSettings()->mpCamera = &camera;
	gfx.SetFrameUniforms(camera.GetViewMatrix(), camera.GetProjectionMatrix(), cVector3f(0), 0.1f, 100, 0);

	auto geometry = [&](const cColor &color)
	{
		std::unique_ptr<iVertexBuffer> buffer(gfx.CreateVertexBuffer(eVertexFlag_Position | eVertexFlag_Color0,
			eVertexBufferDrawType_Tri, eVertexBufferUsageType_Static));
		for(const cVector3f &position : {cVector3f(-1.2f,-1.2f,-2), cVector3f(1.2f,-1.2f,-2), cVector3f(0,1.2f,-2)})
		{
			buffer->AddVertex(eVertexFlag_Position, position);
			buffer->AddColor(eVertexFlag_Color0, color);
		}
		for(unsigned i = 0; i < 3; ++i) buffer->AddIndex(i);
		Check(buffer->Compile(0), "compile transparent geometry");
		return buffer;
	};
	auto red = geometry(cColor(1,0,0,0.5f)), green = geometry(cColor(0,1,0,0.5f));
	auto white = geometry(cColor(1,1)), blue = geometry(cColor(0,0,1,0.5f));
	TransparentMaterial ordinary(gfx, renderer), refractive(gfx, renderer);
	ordinary.program = &flat;
	refractive.refraction = &read;
	cSDLPixelFormat format;
	cGLTexture distortion("distortion fixture", &format, &gfx, eTextureType_RenderTarget, false, eTextureTarget_2D);
	Check(distortion.Create(2,2,cColor(1)), "refraction marker texture");
	refractive.SetTexture(&distortion, eMaterialTexture_Refraction);
	TransparentObject first(ordinary, red.get()), own(ordinary, green.get());
	TransparentObject refract(refractive, white.get()), last(ordinary, blue.get());
	own.custom = &custom;
	first.SetPosition(cVector3f(0,0,-8)); own.SetPosition(cVector3f(0,0,-6));
	refract.SetPosition(cVector3f(0,0,-4)); last.SetPosition(cVector3f(0,0,-2));
	// A large bound makes the refraction copy cover the entire test image.
	refract.GetBoundingVolume()->SetLocalMinMax(cVector3f(-100), cVector3f(100));
	list.Clear();
	for(TransparentObject *object : {&last, &refract, &own, &first}) list.Add(object);
	list.Compile();
	Check(first.queries == 0 && own.queries == 0, "custom callbacks stay at execution time");
	gfx.BeginScene();
	gfx.SetDepthTestActive(true); gfx.SetDepthWriteActive(true);
	gfx.SetDepthTestFunc(eDepthTestFunc_LessOrEqual);
	gfx.SetClearColor(cColor(0,0)); gfx.SetClearDepth(1);
	gfx.SetColorWriteActive(true,true,true,true); gfx.SetScissorActive(false); gfx.ClearScreen();
	gfx.SetCullActive(true); gfx.SetCullMode(eCullMode_Clockwise);
	renderer.GetRenderSettings()->Clear();
	renderer.RenderTransparent(&camera);
	float pixel[4];
	glReadPixels(16,12,1,1,GL_RGBA,GL_FLOAT,pixel);
	Check(std::fabs(pixel[0]-0.125f)<0.005f && std::fabs(pixel[1]-0.125f)<0.005f &&
		std::fabs(pixel[2]-0.5f)<0.005f, "ordinary/custom/refraction/ordinary order preserves earlier scene colours");
	Check(first.queries == 1 && own.queries == 1 && refract.queries == 1 && last.queries == 1 && own.setups == 1,
		"each object callback runs once at its draw");
	Check(glIsEnabled(GL_DEPTH_TEST) && glIsEnabled(GL_CULL_FACE) && !GetGLInteger(GL_DEPTH_WRITEMASK),
		"following ordinary draw restores depth and culling after custom/refraction draws");

	// The same linked program can have different setup objects. Changing only
	// setup must rerun both callbacks, including for an identity model matrix.
	TransparentSetup setupA, setupB;
	cTransparentDraw draw;
	draw.mpProgram = &flat; draw.mpVtxBuffer = red.get(); draw.mBlendMode = eMaterialBlendMode_Alpha;
	draw.mpProgramSetup = &setupA;
	auto state = draw.GetState(); state.ApplyTransparent(renderer.GetRenderSettings());
	draw.mpProgramSetup = &setupB;
	state = draw.GetState(); state.ApplyTransparent(renderer.GetRenderSettings());
	Check(setupA.calls == 1 && setupB.calls == 1 && setupA.matrices == 1 && setupB.matrices == 1,
		"setup-only transitions reapply the program and identity matrix callbacks");
	renderer.GetRenderSettings()->Reset(&gfx);
	gfx.mTargets.FinishScene();
	CheckNoGLErrors();
}

class ShadowTextureManager : public cTextureManager
{
public:
	using cTextureManager::cTextureManager;
	void Add(iResourceBase *resource) { AddResource(resource); }
};
class ShadowResources : public TransparentResources
{
public:
	ShadowResources(cLowLevelResourcesSDL &low, cLowLevelGraphicsSDL &gfx, cGraphics &graphics, cSDLPixelFormat &format)
		: TransparentResources(&low, &gfx)
	{
		mpTextureManager = hplNew(ShadowTextureManager, (&graphics, this));
		auto *falloff = hplNew(cGLTexture, ("core_falloff_linear.BMP", &format, &gfx, eTextureType_Normal, false, eTextureTarget_1D));
		unsigned char pixels[] = {255, 255};
		Check(falloff->CreateFromArray(pixels, 1, cVector3l(2, 1, 1)), "fixture light falloff");
		falloff->IncUserCount();
		static_cast<ShadowTextureManager*>(mpTextureManager)->Add(falloff);
	}
};
class ShadowObject : public TransparentObject
{
public:
	using TransparentObject::TransparentObject;
	bool casts = true;
	bool IsShadowCaster() override { return casts; }
	cMatrixf *GetModelMatrix(cCamera3D*) override { return &GetWorldMatrix(); }
	int GetMatrixUpdateCount() override { return GetTransformUpdateCount(); }
};
class ShadowMaterial : public TransparentMaterial
{
public:
	ShadowMaterial(cLowLevelGraphicsSDL &gfx, cRenderer3D &renderer) : TransparentMaterial(gfx, renderer)
	{
		mbIsTransparent = false;
	}
};

static void TestShadowCaching(cLowLevelGraphicsSDL &gfx)
{
	Check(gfx.mState.Init(), "initialize cached shadow renderer state");
	gfx.mStream.Init();
	gfx.mStream.SetRenderTargets(&gfx.mTargets);
	Check(gfx.mTargets.Init(cVector2l(32, 24), 4), "initialize cached shadow scene");
	gfx.mShadows.Init();
	{
		cSDLPixelFormat format;
		cLowLevelResourcesSDL low(&gfx);
		cGraphics graphics(&gfx, &low);
		ShadowResources resources(low, gfx, graphics, format);
		cRenderList list(&graphics);
		cMeshCreator meshes(&gfx, &resources);
		cRenderer3D renderer(&gfx, &resources, &meshes, &list);
		renderer.GetRenderSettings()->Clear();
		cCamera3D camera;
		cFrustum frustum;
		for(auto &plane : frustum.mPlane) plane = cPlanef(0, 0, 1, 100);
		renderer.GetRenderSettings()->mpCamera = &camera;
		renderer.GetRenderSettings()->mpFrustum = &frustum;
		gfx.SetFrameUniforms(camera.GetViewMatrix(), camera.GetProjectionMatrix(), cVector3f(0), 0.1f, 100, 0);
		cShadowMapSettings settings;
		settings.mlSpotSize = settings.mlPointSize = 64;
		renderer.SetShadowMapSettings(settings);
		ShadowMaterial material(gfx, renderer);
		std::unique_ptr<iVertexBuffer> vertices(gfx.CreateVertexBuffer(
			eVertexFlag_Position | eVertexFlag_Color0 | eVertexFlag_TexCoord,
			eVertexBufferDrawType_Tri, eVertexBufferUsageType_Dynamic));
		for(const cVector3f &position : {cVector3f(-0.8f,-0.8f,-2), cVector3f(0.8f,-0.8f,-2),
			cVector3f(0.8f,0.8f,-2), cVector3f(-0.8f,0.8f,-2)})
		{
			vertices->AddVertex(eVertexFlag_Position, position);
			vertices->AddColor(eVertexFlag_Color0, cColor(1,1));
			vertices->AddVertex(eVertexFlag_TexCoord, cVector3f(position.x > 0 ? 1 : 0, position.y > 0 ? 1 : 0, 0));
		}
		for(unsigned index : {0u,1u,2u,2u,3u,0u}) vertices->AddIndex(index);
		Check(vertices->Compile(0), "shadow fixture geometry");
		ShadowObject object(material, vertices.get());
		object.GetBoundingVolume()->SetLocalMinMax(cVector3f(-1,-1,-2.1f), cVector3f(1,1,-1.9f));
		cSDLBitmap2D white(SDL_CreateSurface(2,2,SDL_PIXELFORMAT_RGBA32), &format, "SDL");
		cSDLBitmap2D holes(SDL_CreateSurface(2,2,SDL_PIXELFORMAT_RGBA32), &format, "SDL");
		white.FillRect(cRect2l(0,0,0,0), cColor(1,1));
		holes.FillRect(cRect2l(0,0,0,0), cColor(1,1,1,0));
		cGLTexture alpha("shadow alpha", &format, &gfx, eTextureType_Normal, false, eTextureTarget_2D);
		Check(alpha.CreateFromBitmap(&white), "initial shadow texture");
		material.SetTexture(&alpha, eMaterialTexture_Diffuse);
		gfx.SetTexture(0, &alpha);
		cLight3DSpot spot("cached spot", &resources), other("other spot", &resources);
		for(auto *light : {&spot, &other})
		{
			light->SetCastShadows(true);
			light->SetFarAttenuation(10);
			light->GetStaticCasters()->insert(&object);
		}
		GLuint query = 0;
		glGenQueries(1, &query);
		gfx.BeginScene();
		GLuint scene = GetGLInteger(GL_DRAW_FRAMEBUFFER_BINDING);
		unsigned submission = 0;
		auto render = [&](iLight3D &light) {
			++submission;
			renderer.mShadowMapStats = cShadowMapStats();
			gfx.SetDepthTestActive(true);
			gfx.SetDepthWriteActive(false);
			gfx.SetScissorActive(false);
			cShadowMapParams params;
			glBeginQuery(GL_PRIMITIVES_GENERATED, query);
			Check(renderer.RenderShadowMap(&light, &camera, params), "render shadow map through production renderer");
			glEndQuery(GL_PRIMITIVES_GENERATED);
			GLuint primitives = 0;
			glGetQueryObjectuiv(query, GL_QUERY_RESULT, &primitives);
			CheckBoundTarget(scene, gfx.mTargets.GetSize().x, gfx.mTargets.GetSize().y);
			const GLenum error = glGetError();
			if(error != GL_NO_ERROR) throw std::runtime_error("shadow submission " + std::to_string(submission) +
				" produced GL error " + std::to_string(error));
			return primitives;
		};
		auto depth = [&](bool cube = false, int face = 0) {
			std::vector<float> pixels(size_t(settings.mlSpotSize) * settings.mlSpotSize);
			glActiveTexture(GL_TEXTURE0 + cGLSLProgram::kShadowMapUnit);
			glGetTexImage(cube ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + face : GL_TEXTURE_2D,
				0, GL_DEPTH_COMPONENT, GL_FLOAT, pixels.data());
			glActiveTexture(GL_TEXTURE0 + gfx.mState.GetActiveTextureUnit());
			return pixels;
		};
		Check(render(spot) == 2 && renderer.GetShadowMapStats().mlMaps == 1, "first spotlight draws its casters");
		const auto original = depth();
		Check(*std::min_element(original.begin(), original.end()) < 0.99f, "shadow fixture contains real caster depth");
		Check(render(other) == 2 && render(spot) == 0, "unchanged light reuses depth after another light");
		Check(renderer.GetShadowMapStats().mlCachedMaps == 1 && depth() == original, "cache hit preserves exact depth pixels");
		std::cout << "Unchanged spotlight: 1 cached map, 0 generated primitives (initial draw: 2)\n";
		settings.mlCacheSizeMB = 0;
		renderer.SetShadowMapSettings(settings);
		Check(render(spot) == 2 && depth() == original, "uncached and cached spotlight depth match exactly");
		settings.mlCacheSizeMB = 128;
		renderer.SetShadowMapSettings(settings);
		Check(render(spot) == 2 && render(spot) == 0, "reenabling the cache fills once");
		object.SetPosition(cVector3f(0.3f,0,0));
		Check(render(spot) == 2 && depth() != original && render(spot) == 0, "moving a door invalidates depth then reuses it");
		vertices->GetArray(eVertexFlag_Position)[0] += 0.2f;
		vertices->UpdateData(eVertexFlag_Position, false);
		Check(render(spot) == 2 && render(spot) == 0, "uploaded deformation invalidates a stationary caster");
		vertices->SetElementNum(3);
		Check(render(spot) == 1, "element-count changes invalidate depth");
		vertices->SetElementNum(-1);
		Check(render(spot) == 2, "restoring all elements invalidates depth");
		object.SetVisible(false);
		Check(render(spot) == 0 && renderer.GetShadowMapStats().mlMaps == 1, "hidden caster clears its old shadow");
		const auto hidden = depth();
		Check(*std::min_element(hidden.begin(), hidden.end()) == 1, "hidden caster leaves empty depth");
		object.SetVisible(true);
		Check(render(spot) == 2, "shown caster invalidates depth");
		spot.GetStaticCasters()->clear();
		Check(render(spot) == 0 && renderer.GetShadowMapStats().mlMaps == 1, "removed caster clears its old shadow");
		spot.GetDynamicCasters()->insert(&object);
		Check(render(spot) == 2 && render(spot) == 0, "stationary dynamic caster can reuse depth");
		renderer.SetShowShadows(eRendererShowShadows_Static);
		Check(render(spot) == 0 && renderer.GetShadowMapStats().mlMaps == 1, "static-only mode drops dynamic shadows");
		renderer.SetShowShadows(eRendererShowShadows_All);
		Check(render(spot) == 2, "restoring dynamic shadows invalidates depth");
		spot.SetFOV(0.9f);
		Check(render(spot) == 2 && render(spot) == 0, "spot projection changes invalidate depth");
		spot.SetPosition(cVector3f(0.1f,0,0));
		Check(render(spot) == 2, "moving the light invalidates depth");
		settings.mfBias += 0.001f;
		settings.mfSoftness += 0.001f;
		renderer.SetShadowMapSettings(settings);
		Check(render(spot) == 0, "sampling-only changes retain depth");
		settings.mFaces = eShadowMapFaces_TowardsLight;
		renderer.SetShadowMapSettings(settings);
		Check(render(spot) == 2, "rasterized-side changes invalidate depth");
		settings.mFaces = eShadowMapFaces_Both;
		renderer.SetShadowMapSettings(settings);
		material.SetHasAlpha(true);
		Check(render(spot) == 2, "enabling alpha testing invalidates depth");
		Check(alpha.CreateFromBitmap(&holes), "edit alpha texture without replacing its resource");
		Check(render(spot) == 2, "alpha content edits invalidate depth");
		const auto empty = depth();
		Check(*std::min_element(empty.begin(), empty.end()) == 1, "transparent alpha removes caster depth");
		unsigned char whitePixels[16];
		std::fill(std::begin(whitePixels), std::end(whitePixels), 255);
		alpha.SetPixels2D(0, cVector2l(0), cVector2l(2), eColorDataFormat_RGBA, whitePixels);
		Check(render(spot) == 2 && depth() != empty, "alpha subimage updates refresh the shadow");
		tBitmap2DVec frames = {&white, &holes};
		cGLTexture animated("animated shadow alpha", &format, &gfx, eTextureType_Normal, false, eTextureTarget_2D);
		Check(animated.CreateAnimFromBitmapVec(&frames), "animated caster alpha");
		material.SetTexture(&animated, eMaterialTexture_Diffuse);
		const auto casterFragment = std::filesystem::path(cGLSLProgram::GetShaderDirectory()) / "hpl1_Diffuse_Color.fragment";
		std::ifstream casterFile(casterFragment);
		const std::string casterSource((std::istreambuf_iterator<char>(casterFile)), {});
		casterFile.close();
		std::ofstream(casterFragment, std::ios::app) << "\n#ifdef HPL_ARRAY_0\ninvalid new array variant\n#endif\n";
		Check(render(spot) == 0 && renderer.GetShadowMapStats().mlMaps == 1, "failed new caster shader skips geometry");
		std::ofstream(casterFragment) << casterSource;
		Check(render(spot) == 2 && render(spot) == 0, "unchanged alpha frame can reuse depth");
		animated.SetAnimationFramePosition(1);
		Check(render(spot) == 2 && depth() == empty, "alpha animation frame invalidates depth");
		animated.SetAnimationFramePosition(0);
		Check(render(spot) == 2 && depth() != empty, "reverse animation also invalidates depth");
		Check(renderer.mpDiffuseProgram->Reload(), "reload shadow caster shader");
		Check(render(spot) == 2, "shader revision invalidates cached depth");
		gfx.SetTextureFilter(eTextureFilter_Trilinear, 1);
		Check(render(spot) == 2, "sampler filtering changes invalidate depth");
		animated.SetWrapS(eTextureWrap_ClampToEdge);
		Check(render(spot) == 2, "alpha texture wrapping changes invalidate depth");
		cGLTexture live("live shadow alpha", &format, &gfx, eTextureType_RenderTarget, false, eTextureTarget_2D);
		Check(live.Create(2, 2, cColor(1)), "live alpha resource target");
		live.SetPixels2D(0, cVector2l(0), cVector2l(2), eColorDataFormat_RGBA, whitePixels);
		material.SetTexture(&live, eMaterialTexture_Diffuse);
		Check(render(spot) == 2 && render(spot) == 2 && renderer.GetShadowMapStats().mlCachedMaps == 0,
			"live render textures bypass reuse without discarding other retained maps");
		material.SetTexture(&animated, eMaterialTexture_Diffuse);
		Check(render(spot) == 0, "returning to unchanged ordinary texture restores its retained map");
		std::unique_ptr<iVertexBuffer> implicit(gfx.CreateVertexBuffer(eVertexFlag_Position | eVertexFlag_TexCoord,
			eVertexBufferDrawType_Tri, eVertexBufferUsageType_Static));
		for(unsigned i = 0; i < 4; ++i)
		{
			implicit->AddVertex(eVertexFlag_Position, vertices->GetVector3(eVertexFlag_Position, i));
			implicit->AddVertex(eVertexFlag_TexCoord, vertices->GetVector3(eVertexFlag_TexCoord, i));
		}
		for(unsigned i : {0u,1u,2u,2u,3u,0u}) implicit->AddIndex(i);
		Check(implicit->Compile(0), "caster with inherited vertex alpha");
		object.vertices = implicit.get();
		gfx.mState.SetCurrentAttrib(cGLSLProgram::eVertexAttrib_Color, 1, 1, 1, 0);
		Check(render(spot) == 2 && depth() == empty, "inherited zero vertex alpha removes caster depth");
		gfx.mState.SetCurrentAttrib(cGLSLProgram::eVertexAttrib_Color, 1, 1, 1, 1);
		Check(render(spot) == 2 && depth() != empty && renderer.GetShadowMapStats().mlCachedMaps == 0,
			"implicit vertex alpha changes bypass reuse");
		object.vertices = vertices.get();
		Check(render(spot) == 0, "explicit vertex attributes recover the retained map");
		object.casts = false;
		Check(render(spot) == 0 && renderer.GetShadowMapStats().mlMaps == 1, "disabled caster clears its old shadow");
		object.casts = true;
		Check(render(spot) == 2, "reenabled caster refreshes depth");
		gfx.mTargets.SetRenderScale(1.5f);
		gfx.BeginScene();
		scene = GetGLInteger(GL_DRAW_FRAMEBUFFER_BINDING);
		Check(render(spot) == 0, "scene target resizing preserves independent cached shadow depth");
		gfx.mTargets.SetRenderScale(1);
		gfx.BeginScene();
		scene = GetGLInteger(GL_DRAW_FRAMEBUFFER_BINDING);
		cLight3DPoint point("cached point", &resources);
		point.SetCastShadows(true);
		point.SetFarAttenuation(10);
		point.GetStaticCasters()->insert(&object);
		frustum.mPlane[0] = cPlanef(0,0,1,-2);
		render(point);
		Check(renderer.GetShadowMapStats().mlMaps == 5, "camera initially omits one cube face");
		frustum.mPlane[0] = cPlanef(0,0,1,100);
		render(point);
		Check(renderer.GetShadowMapStats().mlMaps == 1 && renderer.GetShadowMapStats().mlCachedMaps == 5,
			"camera exposes only the missing cube face");
		const auto cube = depth(true, 5);
		Check(render(point) == 0 && renderer.GetShadowMapStats().mlCachedMaps == 6 && depth(true, 5) == cube,
			"complete unchanged cube generates no geometry");
		settings.mlCacheSizeMB = 0;
		renderer.SetShadowMapSettings(settings);
		render(point);
		Check(depth(true, 5) == cube, "uncached and cached point-light depth match exactly");
		renderer.GetRenderSettings()->Reset(&gfx);
		glDeleteQueries(1, &query);
		gfx.mTargets.FinishScene();
	}
	gfx.mShadows.Reset();
	gfx.mStream.Reset();
	gfx.mTargets.DeleteFramebuffers();
	gfx.mState.Reset();
	CheckNoGLErrors();
}

static void TestHighResImages(cLowLevelGraphicsSDL &gfx, const std::filesystem::path &directory)
{
	// "name@4x.png" in the high-resolution directory replaces the game's
	// "name.*": drawn at the original size from its own mipmapped texture.
	const std::filesystem::path game = directory/"hires_game", hud = directory/"hires_hud";
	std::filesystem::create_directories(game);
	std::filesystem::create_directories(hud);
	auto save = [](const std::filesystem::path &file, int w, int h)
	{
		SDL_Surface *surface = SDL_CreateSurface(w, h, SDL_PIXELFORMAT_RGBA32);
		SDL_FillSurfaceRect(surface, nullptr, 0x80ff00ff);
		const bool saved = IMG_SavePNG(surface, file.string().c_str());
		SDL_DestroySurface(surface);
		Check(saved, "save high-resolution fixture");
	};
	save(game/"hud_icon.png", 8, 6); save(hud/"hud_icon@4x.png", 32, 24);
	save(game/"hud_plain.png", 8, 6);
	save(game/"hud_odd.png", 8, 6); save(hud/"hud_odd@4x.png", 30, 24);
	save(game/"hud_margin.png", 8, 6); save(hud/"hud_margin@4x.png", 48, 32);
	save(game/"hud_uneven.png", 8, 6); save(hud/"hud_uneven@4x.png", 36, 24);
	save(hud/"hud_new@4x.png", 40, 8);

	cLowLevelResourcesSDL low(&gfx);
	cFileSearcher files(&low);
	files.AddDirectory(game.string(), "*.*");
	cImageManager images(&files, &gfx, &low, nullptr);
	images.SetHighResDirectory(hud.string()+"/");

	cResourceImage *icon = images.CreateImage("hud_icon");
	Check(icon && icon->GetSize()==cVector2l(8,6) && icon->GetTexelSize()==cVector2l(32,24),
		"high-resolution image keeps the original size");
	const tVertexVec &vtx = icon->GetVertexVec();
	Check(vtx[2].pos.x==8 && vtx[2].pos.y==6 && vtx[2].tex.x>0.9f && vtx[2].tex.y>0.9f,
		"high-resolution image quad spans its own frame at the original size");
	tVertexVec copy = icon->GetVertexVecCopy(cVector2f(10,20), cVector2f(-1,-1));
	Check(copy[2].pos.x==18 && copy[2].pos.y==26, "default draw size is the original size");
	Check(icon->GetFrameBitmap()->IsLocked(), "high-resolution frame is not shared");
	Check(images.CreateImage("hud_icon.png")==icon, "replacement is found under the original name");

	cResourceImage *plain = images.CreateImage("hud_plain");
	cResourceImage *odd = images.CreateImage("hud_odd");
	Check(plain && plain->GetSize()==cVector2l(8,6) && plain->GetTexelSize()==cVector2l(8,6),
		"image without replacement is unchanged");
	Check(odd && odd->GetTexelSize()==cVector2l(8,6), "replacement of the wrong size falls back to the original");
	Check(plain->GetFrameTexture()==odd->GetFrameTexture() && plain->GetFrameTexture()!=icon->GetFrameTexture(),
		"ordinary images still share the atlas");

	// A margin of whole original pixels, equal on opposite sides, is drawn
	// around the original's rectangle and scales with the draw size.
	cResourceImage *margin = images.CreateImage("hud_margin");
	Check(margin && margin->GetSize()==cVector2l(8,6) && margin->GetMargin()==cVector2l(2,1),
		"replacement with a margin keeps the original size");
	const tVertexVec &marginVtx = margin->GetVertexVec();
	Check(marginVtx[0].pos.x==-2 && marginVtx[0].pos.y==-1 && marginVtx[2].pos.x==10 && marginVtx[2].pos.y==7,
		"margin lies around the original's rectangle");
	copy = margin->GetVertexVecCopy(cVector2f(10,20), cVector2f(16,12));
	Check(copy[0].pos.x==6 && copy[0].pos.y==18 && copy[2].pos.x==30 && copy[2].pos.y==34,
		"margin scales with the draw size");
	cRect2f fraction = margin->GetDrawRect(cVector2f(1,1));
	Check(fraction.x==-0.25f && fraction.w==1.5f && std::fabs(fraction.y + 1.0f/6.0f) < 1e-6f,
		"draw rectangle as a fraction of the draw size");
	cResourceImage *uneven = images.CreateImage("hud_uneven");
	Check(uneven && uneven->GetTexelSize()==cVector2l(8,6) && uneven->GetMargin()==cVector2l(0,0),
		"replacement with an uneven margin falls back to the original");

	// An image the game does not have is laid out at 1/N of its size
	cResourceImage *added = images.CreateImage("hud_new");
	Check(added && added->GetSize()==cVector2l(10,2) && added->GetTexelSize()==cVector2l(40,8),
		"new high-resolution image is laid out at 1/N of its size");
	Check(images.CreateImage("hud_new")==added, "new high-resolution image is loaded once");

	images.FlushAll();
	// The replacement's frame is 34x26 (one texel of border each side).
	CheckImmutableStorage(*icon->GetTexture(), GL_TEXTURE_2D, 6);
	CheckImmutableStorage(*plain->GetTexture(), GL_TEXTURE_2D, 1);
	CheckNoGLErrors();

	images.Destroy(icon); images.Destroy(icon); images.Destroy(plain); images.Destroy(odd);
	images.Destroy(margin); images.Destroy(uneven); images.Destroy(added); images.Destroy(added);
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
		std::filesystem::copy(OPENHPL_TEST_SHADER_DIR,directory,std::filesystem::copy_options::recursive);
		cGLSLProgram::SetShaderDirectory(directory.string()+"/");
		TestQuadInstances(gfx);
		TestTextures(gfx);
		TestHighResImages(gfx, directory);
		TestProgramSubmission();
		TestBackendComponents(gfx);
		TestShadowStorage();
		{
			// This fixture tears down its owners; keep their lifetime separate
			// from the shared texture and transparent-renderer fixture owners.
			cLowLevelGraphicsSDL shadowGraphics;
			cGLSLProgram::InitPrograms(&shadowGraphics);
			TestShadowCaching(shadowGraphics);
		}
		cGLSLProgram::InitPrograms(&gfx);
		TestTransparentExecution(gfx);
		std::filesystem::remove_all(directory);
		std::cout << "Renderer regressions passed on " << glGetString(GL_RENDERER) << " / GL " << glGetString(GL_VERSION) << '\n';
	} catch(const std::exception& e) { std::cerr << e.what() << '\n';result=1; }
	eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
	eglDestroyContext(display,context);eglTerminate(display);
	return result;
}
