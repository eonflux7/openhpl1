// This file is part of Overture Remade, licensed under the GNU GPL v3 or
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
#include "impl/LowLevelGraphicsSDL.h"
#include "impl/SDLTexture.h"
#include "impl/GLSLProgram.h"
#include "math/Math.h"
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
int hplMain(const hpl::tString&)
{
	EGLDisplay display=eglGetPlatformDisplay(EGL_PLATFORM_SURFACELESS_MESA,EGL_DEFAULT_DISPLAY,nullptr);
	if(display==EGL_NO_DISPLAY || !eglInitialize(display,nullptr,nullptr)) return 77;
	eglBindAPI(EGL_OPENGL_API);
	const EGLint attributes[]={EGL_CONTEXT_MAJOR_VERSION,4,EGL_CONTEXT_MINOR_VERSION,3,
		EGL_CONTEXT_OPENGL_PROFILE_MASK,EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT,EGL_NONE};
	EGLContext context=eglCreateContext(display,nullptr,EGL_NO_CONTEXT,attributes);
	if(context==EGL_NO_CONTEXT || !eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,context)) return 77;
	int result=0;
	try {
		Check(gladLoadGL((GLADloadfunc)eglGetProcAddress)!=0,"load OpenGL");
		cLowLevelGraphicsSDL gfx;
		cGLSLProgram::InitPrograms(&gfx);
		auto directory=std::filesystem::temp_directory_path() / ("overture-storage-" +
			std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
		std::filesystem::copy(OVERTURE_TEST_SHADER_DIR,directory,std::filesystem::copy_options::recursive);
		cGLSLProgram::SetShaderDirectory(directory.string()+"/");
		TestTextures(gfx);
		std::filesystem::remove_all(directory);
		std::cout << "Renderer storage regressions passed on " << glGetString(GL_RENDERER) << '\n';
	} catch(const std::exception& e) { std::cerr << e.what() << '\n';result=1; }
	eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);
	eglDestroyContext(display,context);eglTerminate(display);
	return result;
}
