/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of HPL1 Engine.
 *
 * HPL1 Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HPL1 Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HPL1 Engine.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <assert.h>
#include <stdlib.h>

#include "graphics/FontData.h"
#include "impl/LowLevelGraphicsSDL.h"
#include "math/Math.h"
#include "impl/SDLBitmap2D.h"
#include "impl/SDLFontData.h"
#include "impl/SDLTexture.h"

// The look-up textures of the SMAA reference implementation
#include "AreaTex.h"
#include "SearchTex.h"
#include "impl/GLSLProgram.h"
#include "system/LowLevelSystem.h"
#include "impl/VertexBufferVBO.h"

#include "impl/OcclusionQueryOGL.h"
#include "system/String.h"

#include <SDL3_image/SDL_image.h>
#include <vector>
#include <map>
#include <string>
#include <cstring>

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// GLOBAL FUNCTIONS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	GLenum ColorFormatToGL(eColorDataFormat aFormat)
	{
		switch(aFormat)
		{
		case eColorDataFormat_RGB:		return GL_RGB;
		case eColorDataFormat_RGBA:		return GL_RGBA;
		// One-channel textures are red, read as luminance (cSDLTexture)
		case eColorDataFormat_ALPHA:	return GL_RED;
		case eColorDataFormat_BGR:		return GL_BGR;
		case eColorDataFormat_BGRA:		return GL_BGRA;
		};

		return 0;
	}

	//-------------------------------------------------

	static void GLAPIENTRY GLDebugCallback(GLenum aSource, GLenum aType, GLuint alId, GLenum aSeverity,
											GLsizei alLength, const GLchar *apMessage, const void *apUser)
	{
		// NVIDIA: "vertex shader is being recompiled based on GL state", a
		// performance note the first time a program is drawn with some state.
		if(alId == 131218) return;

		// The same message tends to repeat every frame; log each one a few times
		// only. Keyed by text, as drivers give one id to many different errors.
		static std::map<std::string,int> mapCount;
		if(++mapCount[std::string(apMessage, alLength >= 0 ? alLength : strlen(apMessage))] > 3) return;
		Log("GL DEBUG [type 0x%x id %u sev 0x%x]: %s\n", aType, alId, aSeverity, apMessage);
	}

	//-------------------------------------------------

	// std140 layout of hpl_Frame in hpl1_frame.glsl; matrices column-major.
	struct cFrameUniformData
	{
		float mvView[16];
		float mvProjection[16];
		float mvViewProjection[16];
		float mvEyePos[4];
		float mvRenderSize[4];
		float mvDepthRange[4];
		float mvTime[4];
		float mvShading0[4];
		float mvShading1[4];
	};

	static void CopyColumnMajor(float *apDest, const cMatrixf& a_mtx)
	{
		cMatrixf mtxTranspose = a_mtx.GetTranspose();
		memcpy(apDest, mtxTranspose.v, sizeof(float)*16);
	}

	//-------------------------------------------------

	// The matrix glOrtho multiplies by.
	static cMatrixf OrthoMatrix(float afLeft, float afRight, float afBottom, float afTop,
								float afNear, float afFar)
	{
		return cMatrixf(2.0f/(afRight-afLeft), 0, 0, -(afRight+afLeft)/(afRight-afLeft),
						0, 2.0f/(afTop-afBottom), 0, -(afTop+afBottom)/(afTop-afBottom),
						0, 0, -2.0f/(afFar-afNear), -(afFar+afNear)/(afFar-afNear),
						0, 0, 0, 1);
	}

	//-------------------------------------------------

	GLenum TextureTargetToGL(eTextureTarget aTarget)
	{
		switch(aTarget)
		{
		case eTextureTarget_1D:		return GL_TEXTURE_1D;
		case eTextureTarget_2D:		return GL_TEXTURE_2D;
		case eTextureTarget_Rect:	return GL_TEXTURE_RECTANGLE;
		case eTextureTarget_CubeMap:	return GL_TEXTURE_CUBE_MAP;
		case eTextureTarget_3D:		return GL_TEXTURE_3D;
		}
		return 0;
	}


	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cLowLevelGraphicsSDL::cLowLevelGraphicsSDL()
	{
		mlBatchArraySize = 20000;
		mlVertexCount = 0;
		mlIndexCount =0;
		mlMultisampling =0;
		mlDisplay = 0;

		mvVirtualSize.x = 800;
		mvVirtualSize.y = 600;


		mpWindow = nullptr;
		mGLContext = nullptr;
		mfGammaCorrection = 1.0f;
		mfRenderScale = 1.0f;
		mGammaPass = cFullscreenPass("gamma_correction");
		mOutputPass = cFullscreenPass("output");
		mEncodePass = cFullscreenPass("output", "ENCODE_ONLY");
		mSmaaEdgesPass = cFullscreenPass("SMAA", "EDGES");
		mSmaaWeightsPass = cFullscreenPass("SMAA", "WEIGHTS");
		mSmaaWeightsNoDiagPass = cFullscreenPass("SMAA", "WEIGHTS SMAA_DISABLE_DIAG_DETECTION");
		mSmaaBlendPass = cFullscreenPass("SMAA", "BLEND");
		mlSmaaColor = mlSmaaEdges = mlSmaaWeights = mlSmaaStencil = 0;
		mvSmaaFramebuffers[0] = mvSmaaFramebuffers[1] = mvSmaaFramebuffers[2] = 0;
		mlSmaaAreaTexture = mlSmaaSearchTexture = 0;

		mbInScene = false;
		mbSceneResolved = false;
		mbInShadowMap = false;
		mlSceneFramebuffer = 0;
		mlSceneColorFramebuffer = 0;
		mbSceneColorOnly = false;
		mfFrameTime = 0;
		mlFrameUniformBuffer = 0;
		mlOutputFramebuffer = 0;
		mlOutputColor = 0;
		mlOutputDepthStencil = 0;
		mlSceneColor = 0;
		mlSceneDepthStencil = 0;
		mlResolveFramebuffer = 0;
		mlResolveColorFramebuffer = 0;
		mlResolveColor = 0;
		mlResolveDepthStencil = 0;
		mlTargetFramebuffer = 0;
		mlShadowFramebuffer = 0;
		mvShadowMaps[0] = mvShadowMaps[1] = 0;
		mvShadowMapSizes[0] = mvShadowMapSizes[1] = 0;
		mvShadowSamplers[0] = mvShadowSamplers[1] = 0;
		mlStateUniformBuffer = 0;

		mlBoundVertexArray = 0;
		mlStreamVertexArray = 0;
		mlEmptyVertexArray = 0;
		mlStreamVertexBuffer = 0;
		mlStreamIndexBuffer = 0;
		mlStreamVertexSize = 4 * 1024 * 1024;
		mlStreamIndexSize = 1024 * 1024;
		mlStreamVertexOffset = 0;
		mlStreamIndexOffset = 0;
		mlStreamArrays = 0;
		mlImmediateTexUnits = 0;

		// The GL defaults: white, normal along z, texture coordinates 0,0,0,1
		for(int i=0; i<cGLSLProgram::eVertexAttrib_LastEnum; ++i)
			SetCurrentAttrib(i, 0, 0, 0, 1);
		SetCurrentAttrib(cGLSLProgram::eVertexAttrib_Normal, 0, 0, 1, 1);
		SetCurrentAttrib(cGLSLProgram::eVertexAttrib_Color, 1, 1, 1, 1);
		mlCurrentAttribChanged = ~0u;

		mbMatricesChanged = true;
		mlActiveTextureUnit = 0;
		mbAlphaTest = false;
		mbAlphaToCoverage = false;
		mbLinearLighting = false;
		mbLinearSource = false;
		mbBlend = false;
		mBlendSrc = eBlendFunc_One;
		mBlendDest = eBlendFunc_Zero;
		mAlphaTestFunc = eAlphaTestFunc_Always;
		mfAlphaTestRef = 0;
		mbStateChanged = true;

		mpPixelFormat = hplNew(cSDLPixelFormat, () );

		mpRenderTarget=nullptr;

		for(int i=0;i<MAX_TEXTUREUNITS;i++)
		{
			mpCurrentTexture[i] = nullptr;
			mvUnitSampler[i] = 0;
		}
		mTextureFilter = eTextureFilter_Trilinear;
		mfTextureAnisotropy = 1.0f;

		mbClearColor = true;
		mbClearDepth = true;
		mbClearStencil = false;

		//Create the batch arrays:
		mlBatchStride = 13;
		//3 Pos floats, 4 color floats, 3 Tex coord floats .
		mpVertexArray = (float*)hplMalloc(sizeof(float) * mlBatchStride * mlBatchArraySize);
		mpIndexArray = (unsigned int*)hplMalloc(sizeof(unsigned int) * mlBatchArraySize); //Index is one int.

		for(int i=0;i<MAX_TEXTUREUNITS;i++)
		{
			mpTexCoordArray[i] = (float*)hplMalloc(sizeof(float) * 3 * mlBatchArraySize);
			mbTexCoordArrayActive[i] = false;
			mlTexCoordArrayCount[i]=0;
		}
	}

	//-----------------------------------------------------------------------

	cLowLevelGraphicsSDL::~cLowLevelGraphicsSDL()
	{
		hplFree(mpVertexArray);
		hplFree(mpIndexArray);
		for(int i=0;i<MAX_TEXTUREUNITS;i++)	hplFree(mpTexCoordArray[i]);

		hplDelete(mpPixelFormat);

		for(cFullscreenPass *pPass : {&mGammaPass, &mOutputPass, &mEncodePass, &mSmaaEdgesPass, &mSmaaWeightsPass,
									&mSmaaWeightsNoDiagPass, &mSmaaBlendPass})
			DeleteFullscreenPass(*pPass);
		DeleteFramebuffers();
		glDeleteTextures(1, &mlSmaaAreaTexture);
		glDeleteTextures(1, &mlSmaaSearchTexture);
		for(auto &sampler : m_mapSamplers) glDeleteSamplers(1, &sampler.second);
		if(mlFrameUniformBuffer) glDeleteBuffers(1, &mlFrameUniformBuffer);
		if(mlStateUniformBuffer) glDeleteBuffers(1, &mlStateUniformBuffer);
		mFixedFunction.Exit();
		if(mlStreamVertexArray) glDeleteVertexArrays(1, &mlStreamVertexArray);
		if(mlEmptyVertexArray) glDeleteVertexArrays(1, &mlEmptyVertexArray);
		if(mlStreamVertexBuffer) glDeleteBuffers(1, &mlStreamVertexBuffer);
		if(mlStreamIndexBuffer) glDeleteBuffers(1, &mlStreamIndexBuffer);
		mDebugOverlay.Exit();
		mGpuProfiler.Exit();

		cGLSLProgram::ExitPipeline();

		if(mGLContext) SDL_GL_DestroyContext(mGLContext);
		if(mpWindow) SDL_DestroyWindow(mpWindow);
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	bool cLowLevelGraphicsSDL::Init(int alWidth, int alHeight, int alBpp, int abFullscreen,
									int alMultisampling, const tString& asWindowCaption, int alDisplay)
	{
		mvScreenSize.x = alWidth;
		mvScreenSize.y = alHeight;
		mlBpp = alBpp;

		mlMultisampling = alMultisampling;

		//Set some GL Attributes
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);

		SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
		SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

		// Depth, stencil and multisampling belong to the scene framebuffer,
		// the window only shows the finished frame.
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
		SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);

		// Core profile: everything is drawn by shaders from vertex arrays.
		// The version is chosen when the context is created.
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

		const char *pGLDebug = SDL_getenv("OVERTURE_GL_DEBUG");
		bool bGLDebug = pGLDebug && pGLDebug[0] && pGLDebug[0]!='0';
		if(bGLDebug) SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);

		// A size <= 0 means "use the desktop resolution".
		SDL_DisplayID lDisplay = ChooseDisplay(alDisplay);
		const SDL_DisplayMode *pDesktopMode = SDL_GetDesktopDisplayMode(lDisplay);
		if((alWidth <= 0 || alHeight <= 0) && pDesktopMode)
		{
			alWidth = pDesktopMode->w;
			alHeight = pDesktopMode->h;
		}
		if(alWidth <= 0 || alHeight <= 0)
		{
			alWidth = 800;
			alHeight = 600;
		}

		SDL_WindowFlags lFlags = SDL_WINDOW_OPENGL;
		if(abFullscreen) lFlags |= SDL_WINDOW_FULLSCREEN;

		Log(" Setting video mode: %d x %d - %d bpp\n",alWidth, alHeight, alBpp);
		SDL_PropertiesID lProps = SDL_CreateProperties();
		SDL_SetStringProperty(lProps, SDL_PROP_WINDOW_CREATE_TITLE_STRING, asWindowCaption.c_str());
		SDL_SetNumberProperty(lProps, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED_DISPLAY(lDisplay));
		SDL_SetNumberProperty(lProps, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED_DISPLAY(lDisplay));
		SDL_SetNumberProperty(lProps, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, alWidth);
		SDL_SetNumberProperty(lProps, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, alHeight);
		SDL_SetNumberProperty(lProps, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, lFlags);
		mpWindow = SDL_CreateWindowWithProperties(lProps);
		SDL_DestroyProperties(lProps);
		if(mpWindow==nullptr)
		{
			FatalError("Unable to create window: %s\n", SDL_GetError());
			return false;
		}

		if(abFullscreen)
		{
			// Desktop-sized fullscreen is borderless; any other size switches
			// the display mode.
			const SDL_DisplayMode *pMode = nullptr;
			SDL_DisplayMode closestMode;
			if(pDesktopMode == nullptr || pDesktopMode->w != alWidth || pDesktopMode->h != alHeight)
			{
				if(SDL_GetClosestFullscreenDisplayMode(lDisplay, alWidth, alHeight, 0.0f, false, &closestMode))
					pMode = &closestMode;
			}
			SDL_SetWindowFullscreenMode(mpWindow, pMode);
			SDL_SyncWindow(mpWindow);
		}

		// The newest version there is: some drivers give exactly the one
		// asked for. 3.3 is what the renderer needs (with separate shader
		// objects as an extension before 4.1).
		static const int vVersions[][2] = {{4,6}, {4,5}, {4,3}, {4,1}, {3,3}};
		for(const int *pVersion : vVersions)
		{
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, pVersion[0]);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, pVersion[1]);
			mGLContext = SDL_GL_CreateContext(mpWindow);
			if(mGLContext) break;
		}
		if(mGLContext==nullptr)
		{
			FatalError("Unable to create OpenGL context: %s\n", SDL_GetError());
			return false;
		}

		int lPixelW=0, lPixelH=0;
		SDL_GetWindowSizeInPixels(mpWindow, &lPixelW, &lPixelH);
		mvScreenSize.x = lPixelW;
		mvScreenSize.y = lPixelH;
		Log(" Window size: %d x %d\n", mvScreenSize.x, mvScreenSize.y);

		Log(" Loading OpenGL functions...");
		int lGLVersion = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);
		if(lGLVersion == 0)
		{
			Log("ERROR!\n");
			FatalError("Couldn't load OpenGL functions!\n");
			return false;
		}
		Log("OK\n");
		Log("  OpenGL %d.%d core: %s / %s\n", GLAD_VERSION_MAJOR(lGLVersion), GLAD_VERSION_MINOR(lGLVersion),
			(const char*)glGetString(GL_VENDOR), (const char*)glGetString(GL_RENDERER));

		if(bGLDebug && GLAD_GL_KHR_debug)
		{
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageCallback(GLDebugCallback, nullptr);
			// Drop notifications, keep everything else
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
			Log("  OpenGL debug output enabled\n");
		}

		if(!cGLSLProgram::InitPipeline(this))
		{
			FatalError("OpenGL 4.1 or GL_ARB_separate_shader_objects is required!\n");
			return false;
		}

		mGpuProfiler.Init();

		glGenBuffers(1, &mlFrameUniformBuffer);
		glBindBuffer(GL_UNIFORM_BUFFER, mlFrameUniformBuffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(cFrameUniformData), nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		glBindBufferBase(GL_UNIFORM_BUFFER, cGLSLProgram::kFrameBlockBinding, mlFrameUniformBuffer);
		cGpuProfiler::Label(GL_BUFFER, mlFrameUniformBuffer, "hpl_Frame");

		glGenBuffers(1, &mlStateUniformBuffer);
		glBindBuffer(GL_UNIFORM_BUFFER, mlStateUniformBuffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(float)*kStateUniformFloats, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		glBindBufferBase(GL_UNIFORM_BUFFER, cGLSLProgram::kStateBlockBinding, mlStateUniformBuffer);
		cGpuProfiler::Label(GL_BUFFER, mlStateUniformBuffer, "hpl_State");

		if(!mFixedFunction.Init())
		{
			FatalError("Couldn't compile the fixed-function shaders!\n");
			return false;
		}

		glGenVertexArrays(1, &mlEmptyVertexArray);
		glGenVertexArrays(1, &mlStreamVertexArray);
		glGenBuffers(1, &mlStreamVertexBuffer);
		glGenBuffers(1, &mlStreamIndexBuffer);
		BindVertexArray(mlStreamVertexArray);
		glBindBuffer(GL_ARRAY_BUFFER, mlStreamVertexBuffer);
		glBufferData(GL_ARRAY_BUFFER, mlStreamVertexSize, nullptr, GL_STREAM_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mlStreamIndexBuffer);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mlStreamIndexSize, nullptr, GL_STREAM_DRAW);
		cGpuProfiler::Label(GL_VERTEX_ARRAY, mlStreamVertexArray, "Stream");
		cGpuProfiler::Label(GL_BUFFER, mlStreamVertexBuffer, "Stream vertices");
		cGpuProfiler::Label(GL_BUFFER, mlStreamIndexBuffer, "Stream indices");

		//Turn off cursor as default
		ShowCursor(false);

		mvRenderSize = mvScreenSize;
		mfRenderScale = 1.0f;
		if(!CreateOutputFramebuffer() || !CreateSceneFramebuffers())
		{
			FatalError("Couldn't create the framebuffers!\n");
			return false;
		}
		// Something of the right kind for the light shaders' shadow samplers
		CreateShadowMap(false, 16);
		CreateShadowMap(true, 16);

		//GL
		Log(" Setting up OpenGL\n");
		SetupGL();

		//Set the clear color
		SDL_GL_SwapWindow(mpWindow);

		return true;
	}

	//-----------------------------------------------------------------------

	// Debugging aid: OVERTURE_DUMP_FRAMES="60,300" writes those frames to
	// OVERTURE_DUMP_DIR (default: current dir) as frame_<n>.png.
	void cLowLevelGraphicsSDL::DebugDumpFrame()
	{
		static int lFrame = 0;
		static const char *pFrames = SDL_getenv("OVERTURE_DUMP_FRAMES");
		++lFrame;
		if(pFrames==nullptr || pFrames[0]==0) return;

		bool bDump = false;
		for(const char *p = pFrames; *p; )
		{
			if(atoi(p) == lFrame) { bDump = true; break; }
			while(*p && *p!=',') ++p;
			if(*p==',') ++p;
		}
		if(!bDump) return;

		const char *pDir = SDL_getenv("OVERTURE_DUMP_DIR");
		tString sFile = tString(pDir ? pDir : ".") + "/frame_" + cString::ToString(lFrame) + ".png";
		SaveFrameToPNG(sFile);
		Log(" Dumped frame %d to %s (GL error: 0x%x)\n", lFrame, sFile.c_str(), glGetError());
	}

	//-----------------------------------------------------------------------

	bool cLowLevelGraphicsSDL::SaveFrameToPNG(const tString& asFile)
	{
		SDL_Surface *pSurface = SDL_CreateSurface(mvScreenSize.x, mvScreenSize.y, SDL_PIXELFORMAT_RGB24);
		if(pSurface==nullptr) return false;

		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadBuffer(GL_BACK);
		std::vector<unsigned char> vPixels(mvScreenSize.x * mvScreenSize.y * 3);
		glReadPixels(0,0,mvScreenSize.x,mvScreenSize.y,GL_RGB,GL_UNSIGNED_BYTE,&vPixels[0]);
		for(int y=0; y<mvScreenSize.y; ++y)
		{
			memcpy((unsigned char*)pSurface->pixels + y*pSurface->pitch,
					&vPixels[(mvScreenSize.y-1-y) * mvScreenSize.x * 3], mvScreenSize.x * 3);
		}

		bool bRet = IMG_SavePNG(pSurface, asFile.c_str());
		SDL_DestroySurface(pSurface);
		if(bRet==false) Error("Couldn't save frame to '%s': %s\n", asFile.c_str(), SDL_GetError());
		return bRet;
	}

	//-----------------------------------------------------------------------

	SDL_DisplayID cLowLevelGraphicsSDL::ChooseDisplay(int alDisplay)
	{
		int lCount = 0;
		SDL_DisplayID *pDisplays = SDL_GetDisplays(&lCount);
		if(pDisplays==nullptr || lCount==0)
		{
			SDL_free(pDisplays);
			mlDisplay = 0;
			return SDL_GetPrimaryDisplay();
		}

		int lChosen = -1;

		// OVERTURE_DISPLAY=<index> goes before the setting.
		const char *pIndex = SDL_getenv("OVERTURE_DISPLAY");
		if(pIndex && pIndex[0])
		{
			int lIdx = atoi(pIndex);
			if(lIdx >= 0 && lIdx < lCount) lChosen = lIdx;
		}

		if(lChosen < 0 && alDisplay >= 0 && alDisplay < lCount) lChosen = alDisplay;

		// Otherwise use the largest display. Wayland has no primary display,
		// so SDL_GetPrimaryDisplay() just returns the first one it found.
		if(lChosen < 0)
		{
			long lBestArea = -1;
			for(int i=0; i<lCount; ++i)
			{
				const SDL_DisplayMode *pMode = SDL_GetDesktopDisplayMode(pDisplays[i]);
				if(pMode == nullptr) continue;
				long lArea = (long)pMode->w * pMode->h;
				if(lArea > lBestArea){ lBestArea = lArea; lChosen = i; }
			}
		}
		if(lChosen < 0) lChosen = 0;

		for(int i=0; i<lCount; ++i)
		{
			const SDL_DisplayMode *pMode = SDL_GetDesktopDisplayMode(pDisplays[i]);
			Log(" Display %d: %s %dx%d%s\n", i, SDL_GetDisplayName(pDisplays[i]),
				pMode ? pMode->w : 0, pMode ? pMode->h : 0, i==lChosen ? " (used)" : "");
		}

		mlDisplay = lChosen;
		SDL_DisplayID lID = pDisplays[lChosen];
		SDL_free(pDisplays);
		return lID;
	}

	//-----------------------------------------------------------------------

	SDL_DisplayID cLowLevelGraphicsSDL::GetDisplayID(int alDisplay)
	{
		int lCount = 0;
		SDL_DisplayID *pDisplays = SDL_GetDisplays(&lCount);
		SDL_DisplayID lID = 0;
		if(pDisplays && alDisplay >= 0 && alDisplay < lCount) lID = pDisplays[alDisplay];
		SDL_free(pDisplays);
		return lID;
	}

	int cLowLevelGraphicsSDL::GetDisplayNum()
	{
		int lCount = 0;
		SDL_free(SDL_GetDisplays(&lCount));
		return lCount;
	}

	tString cLowLevelGraphicsSDL::GetDisplayName(int alDisplay)
	{
		const char *pName = SDL_GetDisplayName(GetDisplayID(alDisplay));
		return pName ? pName : "";
	}

	cVector2l cLowLevelGraphicsSDL::GetDisplaySize(int alDisplay)
	{
		const SDL_DisplayMode *pMode = SDL_GetDesktopDisplayMode(GetDisplayID(alDisplay));
		if(pMode==nullptr) return cVector2l(0,0);
		return cVector2l(pMode->w, pMode->h);
	}

	void cLowLevelGraphicsSDL::GetDisplayModes(int alDisplay, std::vector<cVector2l> &avModes)
	{
		avModes.clear();

		int lCount = 0;
		SDL_DisplayMode **pModes = SDL_GetFullscreenDisplayModes(GetDisplayID(alDisplay), &lCount);
		if(pModes==nullptr) return;

		// One entry per size; the list has every refresh rate and pixel format.
		std::map<long, cVector2l> mapSizes;
		for(int i=0; i<lCount; ++i)
			mapSizes[(long)pModes[i]->w * 100000 + pModes[i]->h] = cVector2l(pModes[i]->w, pModes[i]->h);
		SDL_free(pModes);

		for(std::map<long, cVector2l>::iterator it = mapSizes.begin(); it != mapSizes.end(); ++it)
			avModes.push_back(it->second);
	}

	int cLowLevelGraphicsSDL::GetDisplay()
	{
		return mlDisplay;
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetupGL()
	{
		//Inits GL stuff
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

		// Each fragment's colour is clamped to 0..1 before blending, as an
		// 8-bit buffer did (by the fragment shaders, see cGLSLProgram); only
		// the sums of blending go past 1 in the float scene buffer.

		//Depth Test setup
		glClearDepth(1.0f);//VAlues buffer is cleared with
		glEnable(GL_DEPTH_TEST); //enable depth testing
		glDepthFunc(GL_LEQUAL); //function to do depth test with

		//int lStencilBits=-1;
		//glGetIntegerv(GL_STENCIL_BITS,&lStencilBits);
		//Log(" Stencil bits: %d\n",lStencilBits);

		//Stencil setup
		glClearStencil(0);

		//Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

		for(int i=0; i<eMatrix_LastEnum; ++i)
		{
			mvMatrixStack[i].assign(1, cMatrixf::Identity);
			LoadMatrix((eMatrix)i);
		}

		// Cube maps are filtered across the edges of their faces
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

		//Show some info
		Log("  Max texture image units: %d\n",GetCaps(eGraphicCaps_MaxTextureImageUnits));
		Log("  Anisotropic filtering: %d\n",GetCaps(eGraphicCaps_AnisotropicFiltering));
		if(GetCaps(eGraphicCaps_AnisotropicFiltering))
			Log("  Max Anisotropic degree: %d\n",GetCaps(eGraphicCaps_MaxAnisotropicFiltering));
	}
	//-----------------------------------------------------------------------

	// An extension until OpenGL 4.6, with the same values
	static bool HasAnisotropicFiltering()
	{
		return GLAD_GL_VERSION_4_6 || GLAD_GL_EXT_texture_filter_anisotropic;
	}

	int cLowLevelGraphicsSDL::GetCaps(eGraphicCaps aType)
	{
		switch(aType)
		{
		//Max Texture Image Units
		case eGraphicCaps_MaxTextureImageUnits:
			{
				int lUnits;
				glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS,(GLint *)&lUnits);
				return lUnits;
			}

		//Texture Anisotropy
		case eGraphicCaps_AnisotropicFiltering:
			{
				return HasAnisotropicFiltering() ? 1 : 0;
			}

		//Texture Anisotropy
		case eGraphicCaps_MaxAnisotropicFiltering:
			{
				if(!HasAnisotropicFiltering()) return 0;

				float fMax;
				glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,&fMax);
				return (int)fMax;
			}

		//The scene is drawn into multisampled textures
		case eGraphicCaps_MaxMultisampling:
			{
				GLint lSamples = 0, lColorSamples = 0, lDepthSamples = 0;
				glGetIntegerv(GL_MAX_SAMPLES, &lSamples);
				glGetIntegerv(GL_MAX_COLOR_TEXTURE_SAMPLES, &lColorSamples);
				glGetIntegerv(GL_MAX_DEPTH_TEXTURE_SAMPLES, &lDepthSamples);
				return cMath::Min(lSamples, cMath::Min(lColorSamples, lDepthSamples));
			}
		}

		return 0;
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::ShowCursor(bool abX)
	{
		if(abX)
			SDL_ShowCursor();
		else
			SDL_HideCursor();
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetVsyncActive(bool abX)
	{
		SDL_GL_SetSwapInterval(abX ? 1 : 0);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetMultisampling(int alSamples)
	{
		if(alSamples < 2) alSamples = 0;
		alSamples = cMath::Min(alSamples, GetCaps(eGraphicCaps_MaxMultisampling));
		if(alSamples == mlMultisampling) return;

		mlMultisampling = alSamples;
		DeleteSceneFramebuffers();
		if(!CreateSceneFramebuffers()) Error("Couldn't create the scene framebuffer with %d samples!\n", alSamples);
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// SAMPLERS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetSamplerFilter(GLuint alSampler, bool abMipMaps)
	{
		GLenum lMinFilter = GL_LINEAR;
		if(abMipMaps)
			lMinFilter = mTextureFilter == eTextureFilter_Bilinear ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR_MIPMAP_LINEAR;
		glSamplerParameteri(alSampler, GL_TEXTURE_MIN_FILTER, lMinFilter);
		glSamplerParameteri(alSampler, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		if(HasAnisotropicFiltering())
			glSamplerParameterf(alSampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, abMipMaps ? mfTextureAnisotropy : 1.0f);
	}

	GLuint cLowLevelGraphicsSDL::GetSampler(bool abMipMaps, GLenum aWrapS, GLenum aWrapT, GLenum aWrapR)
	{
		GLuint &lSampler = m_mapSamplers[std::make_tuple(abMipMaps, aWrapS, aWrapT, aWrapR)];
		if(lSampler == 0)
		{
			glGenSamplers(1, &lSampler);
			glSamplerParameteri(lSampler, GL_TEXTURE_WRAP_S, aWrapS);
			glSamplerParameteri(lSampler, GL_TEXTURE_WRAP_T, aWrapT);
			glSamplerParameteri(lSampler, GL_TEXTURE_WRAP_R, aWrapR);
			SetSamplerFilter(lSampler, abMipMaps);
		}
		return lSampler;
	}

	void cLowLevelGraphicsSDL::SetTextureFilter(eTextureFilter aFilter, float afAnisotropy)
	{
		mTextureFilter = aFilter;
		mfTextureAnisotropy = cMath::Clamp(afAnisotropy, 1.0f, (float)cMath::Max(GetCaps(eGraphicCaps_MaxAnisotropicFiltering), 1));

		for(auto &sampler : m_mapSamplers)
			if(std::get<0>(sampler.first)) SetSamplerFilter(sampler.second, true);
	}

	// Past the units SetTexture() binds to are the scene's depth and the
	// shadow maps, with samplers of their own.
	void cLowLevelGraphicsSDL::BindSampler(unsigned int alUnit, GLuint alSampler)
	{
		if(alUnit >= MAX_TEXTUREUNITS || mvUnitSampler[alUnit] == alSampler) return;
		mvUnitSampler[alUnit] = alSampler;
		glBindSampler(alUnit, alSampler);
	}

	void cLowLevelGraphicsSDL::TextureSamplerChanged(iTexture* apTex)
	{
		for(unsigned int lUnit=0; lUnit<MAX_TEXTUREUNITS; lUnit++)
			if(mpCurrentTexture[lUnit] == apTex) BindSampler(lUnit, static_cast<cSDLTexture*>(apTex)->GetSampler());
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetGammaCorrection(float afX)
	{
		// SDL3 has no hardware gamma ramps. The value is applied to the
		// finished frame in SwapBuffers(), see DrawGammaPass().
		mfGammaCorrection = afX;
	}

	float cLowLevelGraphicsSDL::GetGammaCorrection()
	{
		return mfGammaCorrection;
	}

	//-----------------------------------------------------------------------

	// Like glClipPlane: the plane is in object space and is kept in eye
	// space, by the model-view matrix at the time it is set.
	void cLowLevelGraphicsSDL::SetClipPlane(int alIdx, const cPlanef& aPlane)
	{
		if(alIdx < 0 || alIdx >= kMaxClipPlanes) return;
		mvClipPlanes[alIdx] = aPlane;

		cMatrixf mtxInv = cMath::MatrixInverse(mvMatrixStack[eMatrix_ModelView].back());
		float vPlane[4] = {aPlane.a, aPlane.b, aPlane.c, aPlane.d};
		float vEye[4];
		for(int j=0; j<4; ++j)
			vEye[j] = vPlane[0]*mtxInv.m[0][j] + vPlane[1]*mtxInv.m[1][j] + vPlane[2]*mtxInv.m[2][j] + vPlane[3]*mtxInv.m[3][j];
		mFixedFunction.SetClipPlane(alIdx, cPlanef(vEye[0], vEye[1], vEye[2], vEye[3]));
	}
	cPlanef cLowLevelGraphicsSDL::GetClipPlane(int alIdx, const cPlanef& aPlane)
	{
		return mvClipPlanes[alIdx];
	}
	// Only the fixed-function vertex shader writes the clip distances.
	void cLowLevelGraphicsSDL::SetClipPlaneActive(int alIdx, bool abX)
	{
		if(abX) glEnable(GL_CLIP_DISTANCE0 + alIdx);
		else	glDisable(GL_CLIP_DISTANCE0 + alIdx);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SaveScreenToBMP(const tString& asFile)
	{
		glFinish();

		// Called between frames: the output framebuffer still holds the
		// last one (without the gamma pass).
		glBindFramebuffer(GL_READ_FRAMEBUFFER, mlOutputFramebuffer);

		cSDLBitmap2D *pBmp = hplNew( cSDLBitmap2D, (mpPixelFormat) );
		pBmp->Create(cVector2l(mvScreenSize.x,mvScreenSize.y),32);

		unsigned char *pDestPixels = (unsigned char*)pBmp->GetSurface()->pixels;
		unsigned char *pSrcPixels = (unsigned char*)hplMalloc(mvScreenSize.x * mvScreenSize.y * 4);

		SDL_LockSurface(pBmp->GetSurface());
		glReadPixels(0,0,mvScreenSize.x,mvScreenSize.y,GL_RGBA,GL_UNSIGNED_BYTE,pSrcPixels);

		for(int y=0; y<mvScreenSize.y; ++y)
		{
			for(int x=0; x<mvScreenSize.x; ++x)
			{
				unsigned char* pDestPix = &pDestPixels[((mvScreenSize.x * y) + x) * 4];
				unsigned char* pSrcPix = &pSrcPixels[((mvScreenSize.x * ((mvScreenSize.y-1) - y))
														+ x) * 4];

				pDestPix[0] = pSrcPix[0];
				pDestPix[1] = pSrcPix[1];
				pDestPix[2] = pSrcPix[2];
				pDestPix[3] = 255;
			}
		}

		glBindFramebuffer(GL_READ_FRAMEBUFFER, mlOutputFramebuffer);

		SDL_UnlockSurface(pBmp->GetSurface());
		SDL_SaveBMP(pBmp->GetSurface(),asFile.c_str());

		hplFree(pSrcPixels);
		hplDelete(pBmp);

	}

	//-----------------------------------------------------------------------

	iBitmap2D* cLowLevelGraphicsSDL::CreateBitmap2D(const cVector2l &avSize, unsigned int alBpp)
	{
		cSDLBitmap2D *pBmp = hplNew( cSDLBitmap2D, (mpPixelFormat) );
		pBmp->Create(avSize,alBpp);

		return pBmp;
	}

	//-----------------------------------------------------------------------

	iFontData* cLowLevelGraphicsSDL::CreateFontData(const tString &asName)
	{
		return hplNew( cSDLFontData, (asName, this) );
	}

	//-----------------------------------------------------------------------

	iBitmap2D* cLowLevelGraphicsSDL::CreateBitmap2DFromSurface(SDL_Surface* apSurface,const tString& asType)
	{
		cSDLBitmap2D *pBmp = hplNew( cSDLBitmap2D, (apSurface,mpPixelFormat,asType) );

		pBmp->msType = asType;

		return pBmp;
	}


	//-----------------------------------------------------------------------

	iGpuProgram* cLowLevelGraphicsSDL::CreateGpuProgram(const tString& asName, eGpuProgramType aType)
	{
		return hplNew( cGLSLProgram, (asName, aType) );
	}

	//-----------------------------------------------------------------------

	iPixelFormat* cLowLevelGraphicsSDL::GetPixelFormat()
	{
		return mpPixelFormat;
	}

	//-----------------------------------------------------------------------

	iTexture* cLowLevelGraphicsSDL::CreateTexture(bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget)
	{
		return hplNew( cSDLTexture, ("",mpPixelFormat,this,aType, abUseMipMaps, aTarget) );
	}

	//-----------------------------------------------------------------------

	iTexture* cLowLevelGraphicsSDL::CreateTexture(const tString &asName,bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget)
	{
		return hplNew( cSDLTexture, (asName,mpPixelFormat,this,aType, abUseMipMaps, aTarget) );
	}

	//-----------------------------------------------------------------------

	iTexture* cLowLevelGraphicsSDL::CreateTexture(iBitmap2D* apBmp,bool abUseMipMaps, eTextureType aType,
												eTextureTarget aTarget)
	{
		cSDLTexture *pTex = hplNew( cSDLTexture, ("",mpPixelFormat,this,aType, abUseMipMaps, aTarget) );
		pTex->CreateFromBitmap(apBmp);

		return pTex;
	}

	//-----------------------------------------------------------------------

	iTexture* cLowLevelGraphicsSDL::CreateTexture(const cVector2l& avSize,int alBpp,cColor aFillCol,
											bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget)
	{
		cSDLTexture *pTex=nullptr;

		if(aType==eTextureType_RenderTarget)
		{
			pTex = hplNew( cSDLTexture, ("",mpPixelFormat,this,aType, abUseMipMaps, aTarget) );
			pTex->Create(avSize.x, avSize.y, aFillCol);
		}
		else
		{
			// 64 bits: 16-bit floating point channels
			bool bFloat = alBpp == 64;
			iBitmap2D* pBmp = CreateBitmap2D(avSize,bFloat ? 32 : alBpp);
			pBmp->FillRect(cRect2l(0,0,0,0),aFillCol);

			pTex = hplNew( cSDLTexture, ("",mpPixelFormat,this,aType, abUseMipMaps, aTarget) );
			pTex->SetFloat(bFloat);
			bool bRet = pTex->CreateFromBitmap(pBmp);

			hplDelete(pBmp);

			if(bRet==false){
				hplDelete(pTex);
				return nullptr;
			}
		}
		return pTex;
	}

	//-----------------------------------------------------------------------

	iTexture* cLowLevelGraphicsSDL::CreateFloatTexture(const cVector2l& avSize, int alChannels, eTextureTarget aTarget)
	{
		iBitmap2D* pBmp = CreateBitmap2D(avSize,32);
		pBmp->FillRect(cRect2l(0,0,0,0),cColor(0,0));

		cSDLTexture *pTex = hplNew( cSDLTexture, ("",mpPixelFormat,this,eTextureType_Normal, false, aTarget) );
		pTex->SetFloat(true);
		pTex->SetFloatChannels(alChannels);
		bool bRet = pTex->CreateFromBitmap(pBmp);
		hplDelete(pBmp);

		if(bRet==false){
			hplDelete(pTex);
			return nullptr;
		}
		return pTex;
	}


	//-----------------------------------------------------------------------


	void cLowLevelGraphicsSDL::PushMatrix(eMatrix aMtxType)
	{
		std::vector<cMatrixf>& vStack = mvMatrixStack[aMtxType];
		vStack.push_back(vStack.back());
	}

	//-----------------------------------------------------------------------


	void cLowLevelGraphicsSDL::PopMatrix(eMatrix aMtxType)
	{
		std::vector<cMatrixf>& vStack = mvMatrixStack[aMtxType];
		if(vStack.size() > 1) vStack.pop_back();
		LoadMatrix(aMtxType);
	}
	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetMatrix(eMatrix aMtxType, const cMatrixf& a_mtxA)
	{
		mvMatrixStack[aMtxType].back() = a_mtxA;
		LoadMatrix(aMtxType);
	}

	//-----------------------------------------------------------------------


	void cLowLevelGraphicsSDL::SetIdentityMatrix(eMatrix aMtxType)
	{
		mvMatrixStack[aMtxType].back() = cMatrixf::Identity;
		LoadMatrix(aMtxType);
	}

	//-----------------------------------------------------------------------


	void cLowLevelGraphicsSDL::TranslateMatrix(eMatrix aMtxType, const cVector3f &avPos)
	{
		cMatrixf& mtx = mvMatrixStack[aMtxType].back();
		mtx = cMath::MatrixMul(mtx, cMath::MatrixTranslate(avPos));
		LoadMatrix(aMtxType);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::ScaleMatrix(eMatrix aMtxType, const cVector3f &avScale)
	{
		cMatrixf& mtx = mvMatrixStack[aMtxType].back();
		mtx = cMath::MatrixMul(mtx, cMath::MatrixScale(avScale));
		LoadMatrix(aMtxType);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetOrthoProjection(const cVector2f& avSize, float afMin, float afMax)
	{
		SetMatrix(eMatrix_Projection, OrthoMatrix(0,avSize.x,avSize.y,0,afMin,afMax));
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetVirtualOrthoProjection(float afMin, float afMax)
	{
		cVector2f vOffset = GetVirtualOffset();

		SetMatrix(eMatrix_Projection, OrthoMatrix(-vOffset.x, mvVirtualSize.x + vOffset.x,
												mvVirtualSize.y + vOffset.y, -vOffset.y, afMin,afMax));
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetTexture(unsigned int alUnit,iTexture* apTex)
	{
		if(apTex == mpCurrentTexture[alUnit]) return;

		GLenum NewTarget=0;
		if(apTex)
			NewTarget = GetGLTextureTargetEnum(apTex->GetTarget());

		glActiveTexture(GL_TEXTURE0 + alUnit);

		// A unit without a texture keeps the last one bound; the
		// fixed-function shaders leave such a unit out.
		if(apTex)
		{
			cSDLTexture *pSDLTex = static_cast<cSDLTexture*> (apTex);

			glBindTexture(NewTarget, pSDLTex->GetTextureHandle());
			BindSampler(alUnit, pSDLTex->GetSampler());
		}

		mpCurrentTexture[alUnit] = apTex;
		mlActiveTextureUnit = alUnit;
		mFixedFunction.SetUnitTarget(alUnit, apTex ? (int)apTex->GetTarget() : -1);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::ForgetTexture(iTexture* apTex)
	{
		for(unsigned int lUnit=0; lUnit<MAX_TEXTUREUNITS; lUnit++)
		{
			if(mpCurrentTexture[lUnit] != apTex) continue;

			mpCurrentTexture[lUnit] = nullptr;
			mFixedFunction.SetUnitTarget(lUnit, -1);
		}
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::RestoreSetTexture()
	{
		GLint lActiveUnit = GL_TEXTURE0;
		glGetIntegerv(GL_ACTIVE_TEXTURE,&lActiveUnit);

		int lUnit = lActiveUnit - GL_TEXTURE0;
		if(lUnit < 0 || lUnit >= MAX_TEXTUREUNITS) return;

		iTexture *pTex = mpCurrentTexture[lUnit];
		if(pTex == nullptr) return;

		GLenum target = GetGLTextureTargetEnum(pTex->GetTarget());
		glBindTexture(target, static_cast<cSDLTexture*>(pTex)->GetTextureHandle());
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetActiveTextureUnit(unsigned int alUnit)
	{
		glActiveTexture(GL_TEXTURE0 + alUnit);
		mlActiveTextureUnit = alUnit;
	}

	//-----------------------------------------------------------------------

	// The texture environment is per unit, and these set the active one's,
	// as glTexEnv did.
	void cLowLevelGraphicsSDL::SetTextureEnv(eTextureParam aParam, int alVal)
	{
		mFixedFunction.SetEnv(mlActiveTextureUnit, aParam, alVal);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetTextureConstantColor(const cColor &aColor)
	{
		mFixedFunction.SetConstantColor(mlActiveTextureUnit, aColor);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetColor(const cColor &aColor)
	{
		SetCurrentAttrib(cGLSLProgram::eVertexAttrib_Color, aColor.r, aColor.g, aColor.b, aColor.a);
	}

	//-----------------------------------------------------------------------

	iVertexBuffer* cLowLevelGraphicsSDL::CreateVertexBuffer(tVertexFlag aFlags,
														eVertexBufferDrawType aDrawType,
														eVertexBufferUsageType aUsageType,
														int alReserveVtxSize,int alReserveIdxSize)
	{
		return hplNew( cVertexBufferVBO, (this, aFlags,aDrawType,aUsageType,alReserveVtxSize,alReserveIdxSize) );
	}


	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DrawRect(const cVector2f &avPos,const cVector2f &avSize,float afZ)
	{
		const cColor white(1,1,1,1);
		const cVector3f vTex[4] = {cVector3f(0,0,0), cVector3f(1,0,0), cVector3f(1,1,0), cVector3f(0,1,0)};

		ImmediateBegin(1);
		ImmediateVertex(cVector3f(avPos.x, avPos.y,afZ), white, &vTex[0]);
		ImmediateVertex(cVector3f(avPos.x+avSize.x, avPos.y,afZ), white, &vTex[1]);
		ImmediateVertex(cVector3f(avPos.x+avSize.x, avPos.y+avSize.y,afZ), white, &vTex[2]);
		ImmediateVertex(cVector3f(avPos.x, avPos.y+avSize.y,afZ), white, &vTex[3]);
		ImmediateDraw(GL_TRIANGLE_FAN);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::FlushRendering()
	{
		glFlush();
	}
	void cLowLevelGraphicsSDL::SwapBuffers()
	{
		glFlush();

		if(cGLSLProgram::ReloadChangedFiles())
		{
			// Created again when next drawn
			for(cFullscreenPass *pPass : {&mGammaPass, &mOutputPass, &mEncodePass, &mSmaaEdgesPass, &mSmaaWeightsPass,
										&mSmaaWeightsNoDiagPass, &mSmaaBlendPass})
			{
				DeleteFullscreenPass(*pPass);
				pPass->mbFailed = false;
			}
		}

		BeginGpuSection("Present");
		PresentFrame();
		EndGpuSection();

		// The frame dumps read the window's back buffer, as shown.
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		DebugDumpFrame();
		if(msSaveNextFrame != "")
		{
			mGpuProfiler.Mark(msSaveNextFrame);
			SaveFrameToPNG(msSaveNextFrame);
			msSaveNextFrame = "";
		}

		mGpuProfiler.EndFrame();
		SDL_GL_SwapWindow(mpWindow);
		mGpuProfiler.BeginFrame();

		BindFramebuffer(mlOutputFramebuffer, mvScreenSize);
	}

	//-----------------------------------------------------------------------

	// The scene is floating point, so that light passes add up without
	// being rounded to 8 bits after each one.
	static const GLenum kSceneColorFormat = GL_RGBA16F;

	static GLuint CreateColorTexture(int alW, int alH, GLenum aFormat, GLenum aFilter)
	{
		GLuint lTex = 0;
		glGenTextures(1, &lTex);
		glBindTexture(GL_TEXTURE_2D, lTex);
		glTexImage2D(GL_TEXTURE_2D, 0, aFormat, alW, alH, 0, GL_RGBA,
						aFormat == GL_RGBA8 ? GL_UNSIGNED_BYTE : GL_FLOAT, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, aFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, aFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		return lTex;
	}

	static bool CheckFramebuffer(const char *asName)
	{
		GLenum lStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if(lStatus == GL_FRAMEBUFFER_COMPLETE) return true;
		Error("%s framebuffer incomplete: 0x%x\n", asName, lStatus);
		return false;
	}

	static GLuint CreateDepthStencilTexture(int alW, int alH)
	{
		GLuint lTex = 0;
		glGenTextures(1, &lTex);
		glBindTexture(GL_TEXTURE_2D, lTex);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, alW, alH, 0,
						GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		return lTex;
	}

	// The scene framebuffer (render size, with a depth and stencil buffer),
	// and one with the colour alone. Multisampled, they have a pair with one
	// sample a pixel to resolve into. Textures rather than renderbuffers, so
	// later passes can read them. They are bound by hand, SetTexture() never
	// sees them. Leaves the output framebuffer bound.
	bool cLowLevelGraphicsSDL::CreateSceneFramebuffers()
	{
		GLint lLastTexture = 0;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &lLastTexture);

		const int lW = mvRenderSize.x, lH = mvRenderSize.y;
		bool bOK = true;
		// The output pass samples the single-sampled scene colour, and
		// filters it when it is scaled to the window.
		GLenum lFilter = mvRenderSize == mvScreenSize ? GL_NEAREST : GL_LINEAR;

		mlMultisampling = cMath::Min(mlMultisampling, GetCaps(eGraphicCaps_MaxMultisampling));
		if(mlMultisampling < 2) mlMultisampling = 0;

		GLenum lTarget = GL_TEXTURE_2D;
		if(mlMultisampling > 0)
		{
			lTarget = GL_TEXTURE_2D_MULTISAMPLE;
			glGenTextures(1, &mlSceneColor);
			glGenTextures(1, &mlSceneDepthStencil);
			glBindTexture(lTarget, mlSceneColor);
			glTexImage2DMultisample(lTarget, mlMultisampling, kSceneColorFormat, lW, lH, GL_TRUE);
			glBindTexture(lTarget, mlSceneDepthStencil);
			glTexImage2DMultisample(lTarget, mlMultisampling, GL_DEPTH24_STENCIL8, lW, lH, GL_TRUE);
			glBindTexture(lTarget, 0);
		}
		else
		{
			mlSceneColor = CreateColorTexture(lW, lH, kSceneColorFormat, lFilter);
			mlSceneDepthStencil = CreateDepthStencilTexture(lW, lH);
		}

		glGenFramebuffers(1, &mlSceneFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, mlSceneFramebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, lTarget, mlSceneColor, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, lTarget, mlSceneDepthStencil, 0);
		bOK = CheckFramebuffer("Scene") && bOK;
		cGpuProfiler::Label(GL_FRAMEBUFFER, mlSceneFramebuffer, "Scene");

		// The same colour alone, for the passes that read the depth of
		// the scene they draw over.
		glGenFramebuffers(1, &mlSceneColorFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, mlSceneColorFramebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, lTarget, mlSceneColor, 0);
		bOK = CheckFramebuffer("Scene colour") && bOK;
		cGpuProfiler::Label(GL_FRAMEBUFFER, mlSceneColorFramebuffer, "Scene colour only");
		cGpuProfiler::Label(GL_TEXTURE, mlSceneColor, "Scene colour");
		cGpuProfiler::Label(GL_TEXTURE, mlSceneDepthStencil, "Scene depth/stencil");

		if(mlMultisampling > 0)
		{
			mlResolveColor = CreateColorTexture(lW, lH, kSceneColorFormat, lFilter);
			mlResolveDepthStencil = CreateDepthStencilTexture(lW, lH);
			glGenFramebuffers(1, &mlResolveFramebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, mlResolveFramebuffer);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mlResolveColor, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, mlResolveDepthStencil, 0);
			bOK = CheckFramebuffer("Resolve") && bOK;
			glGenFramebuffers(1, &mlResolveColorFramebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, mlResolveColorFramebuffer);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mlResolveColor, 0);
			bOK = CheckFramebuffer("Resolve colour") && bOK;
			cGpuProfiler::Label(GL_FRAMEBUFFER, mlResolveFramebuffer, "Resolve");
			cGpuProfiler::Label(GL_FRAMEBUFFER, mlResolveColorFramebuffer, "Resolve colour only");
			cGpuProfiler::Label(GL_TEXTURE, mlResolveColor, "Resolve colour");
			cGpuProfiler::Label(GL_TEXTURE, mlResolveDepthStencil, "Resolve depth/stencil");
		}

		// The depth with one sample a pixel stays bound to a unit past
		// those SetTexture() binds to, for the passes that read it while
		// they draw into something else (hpl1_scene_depth.glsl).
		glActiveTexture(GL_TEXTURE0 + cGLSLProgram::kSceneDepthUnit);
		glBindTexture(GL_TEXTURE_2D, mlMultisampling > 0 ? mlResolveDepthStencil : mlSceneDepthStencil);
		glActiveTexture(GL_TEXTURE0 + mlActiveTextureUnit);

		glBindTexture(GL_TEXTURE_2D, lLastTexture);
		BindFramebuffer(mlOutputFramebuffer, mvScreenSize);

		Log(" Scene framebuffer: %d x %d, %d samples, 16-bit float\n", lW, lH, mlMultisampling);
		return bOK;
	}

	// The output framebuffer, at the window size. The 2D drawing does not
	// use depth, but GL state it sets (stencil, depth test) should behave as
	// on any framebuffer.
	bool cLowLevelGraphicsSDL::CreateOutputFramebuffer()
	{
		GLint lLastTexture = 0;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &lLastTexture);

		mlOutputColor = CreateColorTexture(mvScreenSize.x, mvScreenSize.y, GL_RGBA8, GL_NEAREST);
		glGenRenderbuffers(1, &mlOutputDepthStencil);
		glBindRenderbuffer(GL_RENDERBUFFER, mlOutputDepthStencil);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, mvScreenSize.x, mvScreenSize.y);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
		glGenFramebuffers(1, &mlOutputFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, mlOutputFramebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mlOutputColor, 0);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mlOutputDepthStencil);
		bool bOK = CheckFramebuffer("Output");
		cGpuProfiler::Label(GL_FRAMEBUFFER, mlOutputFramebuffer, "Output");
		cGpuProfiler::Label(GL_TEXTURE, mlOutputColor, "Output colour");
		cGpuProfiler::Label(GL_RENDERBUFFER, mlOutputDepthStencil, "Output depth/stencil");

		glBindTexture(GL_TEXTURE_2D, lLastTexture);
		BindFramebuffer(mlOutputFramebuffer, mvScreenSize);
		return bOK;
	}

	void cLowLevelGraphicsSDL::DeleteSceneFramebuffers()
	{
		GLuint vFramebuffers[] = {mlSceneFramebuffer, mlSceneColorFramebuffer, mlResolveFramebuffer,
									mlResolveColorFramebuffer};
		GLuint vTextures[] = {mlSceneColor, mlSceneDepthStencil, mlResolveColor, mlResolveDepthStencil};
		glDeleteFramebuffers(4, vFramebuffers);
		glDeleteTextures(4, vTextures);
		mlSceneFramebuffer = mlSceneColorFramebuffer = mlResolveFramebuffer = mlResolveColorFramebuffer = 0;
		mlSceneColor = mlSceneDepthStencil = mlResolveColor = mlResolveDepthStencil = 0;
		DeleteSmaaTextures();
	}

	void cLowLevelGraphicsSDL::DeleteFramebuffers()
	{
		DeleteSceneFramebuffers();
		glDeleteFramebuffers(1, &mlOutputFramebuffer);
		glDeleteFramebuffers(1, &mlTargetFramebuffer);
		glDeleteFramebuffers(1, &mlShadowFramebuffer);
		glDeleteTextures(2, mvShadowMaps);
		glDeleteSamplers(2, mvShadowSamplers);
		mlShadowFramebuffer = 0;
		mvShadowMaps[0] = mvShadowMaps[1] = 0;
		mvShadowMapSizes[0] = mvShadowMapSizes[1] = 0;
		mvShadowSamplers[0] = mvShadowSamplers[1] = 0;
		glDeleteTextures(1, &mlOutputColor);
		glDeleteRenderbuffers(1, &mlOutputDepthStencil);
		mlOutputFramebuffer = mlTargetFramebuffer = mlOutputColor = mlOutputDepthStencil = 0;
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetRenderScale(float afScale)
	{
		mfRenderScale = cMath::Clamp(afScale, 0.25f, 2.0f);

		cVector2l vSize((int)(mvScreenSize.x * mfRenderScale + 0.5f), (int)(mvScreenSize.y * mfRenderScale + 0.5f));
		vSize.x = cMath::Max(vSize.x, 1);
		vSize.y = cMath::Max(vSize.y, 1);
		if(vSize == mvRenderSize) return;

		mvRenderSize = vSize;
		DeleteSceneFramebuffers();
		if(!CreateSceneFramebuffers()) Error("Couldn't create the scene framebuffer at %d x %d!\n", vSize.x, vSize.y);
	}

	float cLowLevelGraphicsSDL::GetRenderScale()
	{
		return mfRenderScale;
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::BindFramebuffer(GLuint alFramebuffer, const cVector2l& avSize)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, alFramebuffer);
		glViewport(0, 0, avSize.x, avSize.y);
		mvTargetSize = avSize;
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetLinearLighting(bool abX)
	{
		mbLinearLighting = abX;
		mbStateChanged = true;
	}

	bool cLowLevelGraphicsSDL::GetLinearLighting()
	{
		return mbLinearLighting;
	}

	void cLowLevelGraphicsSDL::SetLinearSource(bool abX)
	{
		mbLinearSource = abX;
		mbStateChanged = true;
	}

	void cLowLevelGraphicsSDL::SetLinearLightingTuning(const cLinearLightingTuning& aTuning)
	{
		mLinearTuning = aTuning;
		mbStateChanged = true;
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::BeginScene()
	{
		SetRenderTarget(nullptr);
		mbInScene = true;
		mbSceneResolved = false;
		mbSceneColorOnly = false;
		mbStateChanged = true;
		BindFramebuffer(mlSceneFramebuffer, mvRenderSize);
	}

	//-----------------------------------------------------------------------

	// The framebuffer that is drawn into, bound again after another was.
	void cLowLevelGraphicsSDL::BindCurrentFramebuffer()
	{
		if(mpRenderTarget)	glBindFramebuffer(GL_FRAMEBUFFER, mlTargetFramebuffer);
		else if(mbInScene)	glBindFramebuffer(GL_FRAMEBUFFER, GetSceneFramebuffer());
		else				glBindFramebuffer(GL_FRAMEBUFFER, mlOutputFramebuffer);
	}

	// Copies a part of the multisampled scene to the textures with one
	// sample a pixel: the colour as the average of the samples, of the depth
	// one of them. Leaves the two framebuffers bound.
	void cLowLevelGraphicsSDL::BlitScene(GLbitfield aMask, int alX0, int alY0, int alX1, int alY1)
	{
		GLboolean bScissor = glIsEnabled(GL_SCISSOR_TEST);
		GLboolean bDepthMask = GL_TRUE;
		GLboolean vColorMask[4];
		glGetBooleanv(GL_DEPTH_WRITEMASK, &bDepthMask);
		glGetBooleanv(GL_COLOR_WRITEMASK, vColorMask);
		if(bScissor) glDisable(GL_SCISSOR_TEST);
		glDepthMask(GL_TRUE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		glBindFramebuffer(GL_READ_FRAMEBUFFER, mlSceneFramebuffer);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mlResolveFramebuffer);
		glBlitFramebuffer(alX0, alY0, alX1, alY1, alX0, alY0, alX1, alY1, aMask, GL_NEAREST);

		if(bScissor) glEnable(GL_SCISSOR_TEST);
		glDepthMask(bDepthMask);
		glColorMask(vColorMask[0], vColorMask[1], vColorMask[2], vColorMask[3]);
	}

	void cLowLevelGraphicsSDL::ResolveSceneDepth()
	{
		if(mlMultisampling <= 0 || mbInScene==false || mbSceneResolved) return;

		BlitScene(GL_DEPTH_BUFFER_BIT, 0, 0, mvRenderSize.x, mvRenderSize.y);
		BindCurrentFramebuffer();
	}

	void cLowLevelGraphicsSDL::ResolveScene()
	{
		if(mbInScene==false || mbSceneResolved) return;

		if(mlMultisampling > 0) BlitScene(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, 0, 0, mvRenderSize.x, mvRenderSize.y);
		mbSceneResolved = true;
		mbStateChanged = true;
		if(mlMultisampling > 0) BindCurrentFramebuffer();
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetPassUniform(cFullscreenPass &aPass, const char *asName,
												float afX, float afY, float afZ, float afW)
	{
		glProgramUniform4f(aPass.mlFragment, glGetUniformLocation(aPass.mlFragment, asName), afX, afY, afZ, afW);
	}

	// Draws the scene to the output framebuffer through the output pass
	// (scaled, converted to 8 bits with dithering), after SMAA if that is
	// on, and leaves the output framebuffer bound for the 2D drawing.
	void cLowLevelGraphicsSDL::EndScene()
	{
		SetRenderTarget(nullptr);
		ResolveScene();
		mbInScene = false;
		mbSceneColorOnly = false;
		mbStateChanged = true;

		GLboolean bScissor = glIsEnabled(GL_SCISSOR_TEST);
		if(bScissor) glDisable(GL_SCISSOR_TEST);

		const bool bMultisampled = mlMultisampling > 0;
		GLuint lColor = bMultisampled ? mlResolveColor : mlSceneColor;
		GLuint lColorFramebuffer = bMultisampled ? mlResolveColorFramebuffer : mlSceneColorFramebuffer;

		// SMAA leaves the scene as screen values
		bool bLinear = mbLinearLighting;
		if(mSmaa.mbActive)
		{
			mGpuProfiler.Begin("SMAA");
			if(DrawSmaa(lColor, lColorFramebuffer)) bLinear = false;
			mGpuProfiler.End();
		}

		BindFramebuffer(mlOutputFramebuffer, mvScreenSize);
		if(CreateFullscreenPass(mOutputPass))
		{
			SetPassUniform(mOutputPass, "sceneSize", (float)mvRenderSize.x, (float)mvRenderSize.y,
							1.0f / mvRenderSize.x, 1.0f / mvRenderSize.y);
			SetPassUniform(mOutputPass, "invOutputSize", 1.0f / mvScreenSize.x, 1.0f / mvScreenSize.y);
			// The shoulder is set as a screen value; the curve works on light
			SetPassUniform(mOutputPass, "tonemap", bLinear ? 1.0f : 0.0f, mLinearTuning.mfExposure,
							powf(cMath::Clamp(mLinearTuning.mfShoulder, 0.0f, 1.0f), 2.2f),
							cMath::Max(mLinearTuning.mfWhite, 1.0f));
			// The grain: its cells are as large on a picture of any size,
			// and all of it changes at its own rate
			const cFilmGrainSettings &grain = mFilmGrain;
			float fGrainSeed = grain.mfRate > 0 ?	floorf(mfFrameTime * grain.mfRate) :
													mfFrameTime * 1000.0f;
			SetPassUniform(mOutputPass, "grain", (float)grain.mType, cMath::Clamp(grain.mfIntensity, 0.0f, 1.0f),
							cMath::Max(grain.mfSize * (float)mvScreenSize.y / 1080.0f, 1.0f),
							fmodf(fGrainSeed, 65536.0f));
			SetPassUniform(mOutputPass, "grainTone", cMath::Clamp(grain.mfColor, 0.0f, 1.0f),
							cMath::Clamp(grain.mfShadows, 0.0f, 1.0f), cMath::Clamp(grain.mfHighlights, 0.0f, 1.0f),
							cMath::Clamp(grain.mfDispersion, 0.0f, 1.0f));
			DrawFullscreenPass(mOutputPass, lColor, mvScreenSize);
		}
		else
		{
			// Without dithering
			bool bScaled = mvRenderSize != mvScreenSize;
			glBindFramebuffer(GL_READ_FRAMEBUFFER, lColorFramebuffer);
			glBlitFramebuffer(0, 0, mvRenderSize.x, mvRenderSize.y, 0, 0, mvScreenSize.x, mvScreenSize.y,
								GL_COLOR_BUFFER_BIT, bScaled ? GL_LINEAR : GL_NEAREST);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, mlOutputFramebuffer);
		}

		if(bScissor) glEnable(GL_SCISSOR_TEST);
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// SMAA
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DeleteSmaaTextures()
	{
		GLuint vTextures[] = {mlSmaaColor, mlSmaaEdges, mlSmaaWeights};
		glDeleteFramebuffers(3, mvSmaaFramebuffers);
		glDeleteTextures(3, vTextures);
		glDeleteRenderbuffers(1, &mlSmaaStencil);
		mvSmaaFramebuffers[0] = mvSmaaFramebuffers[1] = mvSmaaFramebuffers[2] = 0;
		mlSmaaColor = mlSmaaEdges = mlSmaaWeights = mlSmaaStencil = 0;
	}

	// The textures of the passes, at the render size, and the two look-up
	// textures of the reference implementation, which are the same at any
	// size: the share of a pixel an edge of a given shape covers, and how
	// far an edge goes on from where the search for its end stopped.
	bool cLowLevelGraphicsSDL::CreateSmaaTextures()
	{
		if(mlSmaaColor) return mvSmaaFramebuffers[0] != 0;

		GLint lLastTexture = 0, lLastAlignment = 4;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &lLastTexture);
		glGetIntegerv(GL_UNPACK_ALIGNMENT, &lLastAlignment);

		if(mlSmaaAreaTexture == 0)
		{
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			mlSmaaAreaTexture = CreateColorTexture(AREATEX_WIDTH, AREATEX_HEIGHT, GL_RG8, GL_LINEAR);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, AREATEX_WIDTH, AREATEX_HEIGHT, GL_RG, GL_UNSIGNED_BYTE, areaTexBytes);
			mlSmaaSearchTexture = CreateColorTexture(SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, GL_R8, GL_NEAREST);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, SEARCHTEX_WIDTH, SEARCHTEX_HEIGHT, GL_RED, GL_UNSIGNED_BYTE, searchTexBytes);
			glPixelStorei(GL_UNPACK_ALIGNMENT, lLastAlignment);
			cGpuProfiler::Label(GL_TEXTURE, mlSmaaAreaTexture, "SMAA area");
			cGpuProfiler::Label(GL_TEXTURE, mlSmaaSearchTexture, "SMAA search");
		}

		const int lW = mvRenderSize.x, lH = mvRenderSize.y;
		// The picture keeps the scene's precision: it is dithered to 8 bits later
		mlSmaaColor = CreateColorTexture(lW, lH, kSceneColorFormat, GL_LINEAR);
		mlSmaaEdges = CreateColorTexture(lW, lH, GL_RG8, GL_LINEAR);
		mlSmaaWeights = CreateColorTexture(lW, lH, GL_RGBA8, GL_LINEAR);
		glGenRenderbuffers(1, &mlSmaaStencil);
		glBindRenderbuffer(GL_RENDERBUFFER, mlSmaaStencil);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, lW, lH);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		bool bOK = true;
		const GLuint vTextures[3] = {mlSmaaColor, mlSmaaEdges, mlSmaaWeights};
		const char *vNames[3] = {"SMAA picture", "SMAA edges", "SMAA weights"};
		glGenFramebuffers(3, mvSmaaFramebuffers);
		for(int i=0; i<3; ++i)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, mvSmaaFramebuffers[i]);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, vTextures[i], 0);
			if(i > 0) glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, mlSmaaStencil);
			bOK = CheckFramebuffer(vNames[i]) && bOK;
			cGpuProfiler::Label(GL_FRAMEBUFFER, mvSmaaFramebuffers[i], vNames[i]);
			cGpuProfiler::Label(GL_TEXTURE, vTextures[i], vNames[i]);
		}

		glBindTexture(GL_TEXTURE_2D, lLastTexture);
		if(bOK==false)
		{
			// Not tried again until the scene's size changes
			glDeleteFramebuffers(3, mvSmaaFramebuffers);
			mvSmaaFramebuffers[0] = mvSmaaFramebuffers[1] = mvSmaaFramebuffers[2] = 0;
		}
		return bOK;
	}

	// alColor holds the finished scene and is what alColorFramebuffer draws
	// into. The scene is made into the picture as it is shown (tonemapped,
	// as screen values: edges are judged as they look), its edges are found,
	// then how much each pixel on them is to be blended with its neighbour,
	// and the blended picture goes back to alColor. False if a pass is
	// missing, and the scene is as it was.
	bool cLowLevelGraphicsSDL::DrawSmaa(GLuint alColor, GLuint alColorFramebuffer)
	{
		const cSmaaSettings &settings = mSmaa;
		int lDiagonalSteps = cMath::Clamp(settings.mlDiagonalSteps, 0, 20);
		cFullscreenPass &weightsPass = lDiagonalSteps > 0 ? mSmaaWeightsPass : mSmaaWeightsNoDiagPass;

		if(	!CreateFullscreenPass(mEncodePass) || !CreateFullscreenPass(mSmaaEdgesPass) ||
			!CreateFullscreenPass(weightsPass) || !CreateFullscreenPass(mSmaaBlendPass)) return false;
		if(CreateSmaaTextures()==false) return false;

		const cVector2l &vSize = mvRenderSize;
		GLfloat vClearColor[4];
		GLint lClearStencil = 0;
		glGetFloatv(GL_COLOR_CLEAR_VALUE, vClearColor);
		glGetIntegerv(GL_STENCIL_CLEAR_VALUE, &lClearStencil);
		glClearColor(0, 0, 0, 0);
		glClearStencil(0);
		glStencilMask(0xff);

		///////////////////////////////////////////
		// The picture
		BindFramebuffer(mvSmaaFramebuffers[0], vSize);
		SetPassUniform(mEncodePass, "tonemap", mbLinearLighting ? 1.0f : 0.0f, mLinearTuning.mfExposure,
						powf(cMath::Clamp(mLinearTuning.mfShoulder, 0.0f, 1.0f), 2.2f),
						cMath::Max(mLinearTuning.mfWhite, 1.0f));
		DrawFullscreenPass(mEncodePass, alColor, vSize);

		///////////////////////////////////////////
		// Edges: the stencil buffer is set where there is one
		for(cFullscreenPass *pPass : {&mSmaaEdgesPass, &weightsPass, &mSmaaBlendPass})
		{
			SetPassUniform(*pPass, "smaaMetrics", 1.0f / vSize.x, 1.0f / vSize.y, (float)vSize.x, (float)vSize.y);
			SetPassUniform(*pPass, "smaaEdge", (float)settings.mEdges, cMath::Clamp(settings.mfThreshold, 0.01f, 0.5f),
							cMath::Max(settings.mfDepthThreshold, 0.0001f), (float)settings.mlShow);
			SetPassUniform(*pPass, "smaaSearch", (float)cMath::Clamp(settings.mlSearchSteps, 1, 112),
							(float)lDiagonalSteps, (float)cMath::Clamp(settings.mlCornerRounding, 0, 100));
			SetPassUniform(*pPass, "smaaDepth", settings.mbDepthHelp ? 1.0f : 0.0f,
							cMath::Max(settings.mfDepthScale, 0.0f), cMath::Clamp(settings.mfDepthStrength, 0.0f, 1.0f));
		}

		BindFramebuffer(mvSmaaFramebuffers[1], vSize);
		glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glStencilFunc(GL_ALWAYS, 1, 0xff);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
		DrawFullscreenPass(mSmaaEdgesPass, mlSmaaColor, vSize, 0, 0, true);

		///////////////////////////////////////////
		// Blend weights, only where there are edges
		BindFramebuffer(mvSmaaFramebuffers[2], vSize);
		glClear(GL_COLOR_BUFFER_BIT);
		glStencilFunc(GL_EQUAL, 1, 0xff);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
		DrawFullscreenPass(weightsPass, mlSmaaEdges, vSize, mlSmaaAreaTexture, mlSmaaSearchTexture, true);

		///////////////////////////////////////////
		// The blend, back into the scene
		BindFramebuffer(alColorFramebuffer, vSize);
		DrawFullscreenPass(mSmaaBlendPass, mlSmaaColor, vSize, mlSmaaWeights, mlSmaaEdges);

		// The renderer sets the stencil function and mask anew whenever it
		// uses the stencil buffer
		glClearColor(vClearColor[0], vClearColor[1], vClearColor[2], vClearColor[3]);
		glClearStencil(lClearStencil);
		return true;
	}

	//-----------------------------------------------------------------------

	// Copies the finished frame to the window's back buffer, through the
	// gamma pass when the gamma is not 1. Leaves the window framebuffer bound.
	void cLowLevelGraphicsSDL::PresentFrame()
	{
		SetRenderTarget(nullptr);

		GLboolean bScissor = glIsEnabled(GL_SCISSOR_TEST);
		if(bScissor) glDisable(GL_SCISSOR_TEST);

		BindFramebuffer(0, mvScreenSize);
		if(DrawGammaPass()==false)
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, mlOutputFramebuffer);
			glBlitFramebuffer(0, 0, mvScreenSize.x, mvScreenSize.y, 0, 0, mvScreenSize.x, mvScreenSize.y,
								GL_COLOR_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}

		mDebugOverlay.Draw(mpWindow, &mGpuProfiler, mvRenderSize, mfRenderScale);

		// The renderer uses destination alpha as scratch space, so the frame
		// ends with arbitrary alpha. Wayland compositors treat the window as
		// translucent, so make it opaque.
		GLboolean vColorMask[4];
		GLfloat vClearColor[4];
		glGetBooleanv(GL_COLOR_WRITEMASK, vColorMask);
		glGetFloatv(GL_COLOR_CLEAR_VALUE, vClearColor);
		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
		glClearColor(0, 0, 0, 1);
		glClear(GL_COLOR_BUFFER_BIT);
		glColorMask(vColorMask[0], vColorMask[1], vColorMask[2], vColorMask[3]);
		glClearColor(vClearColor[0], vClearColor[1], vClearColor[2], vClearColor[3]);

		if(bScissor) glEnable(GL_SCISSOR_TEST);
	}

	//-----------------------------------------------------------------------

	// Loads hpl1_<name>.vertex/.fragment once. Returns false if they are
	// missing or do not compile, then and later.
	bool cLowLevelGraphicsSDL::CreateFullscreenPass(cFullscreenPass &aPass)
	{
		if(aPass.mlPipeline) return true;
		if(aPass.mbFailed) return false;

		aPass.mlVertex = cGLSLProgram::CreateStage(GL_VERTEX_SHADER, aPass.msName);
		aPass.mlFragment = cGLSLProgram::CreateStage(GL_FRAGMENT_SHADER, aPass.msName, aPass.msDefines);
		if(aPass.mlVertex==0 || aPass.mlFragment==0)
		{
			Error("Couldn't create the '%s' pass (%s)!\n", aPass.msName.c_str(), aPass.msDefines.c_str());
			DeleteFullscreenPass(aPass);
			aPass.mbFailed = true;
			return false;
		}

		glGenProgramPipelines(1,&aPass.mlPipeline);
		glUseProgramStages(aPass.mlPipeline, GL_VERTEX_SHADER_BIT, aPass.mlVertex);
		glUseProgramStages(aPass.mlPipeline, GL_FRAGMENT_SHADER_BIT, aPass.mlFragment);
		cGpuProfiler::Label(GL_PROGRAM_PIPELINE, aPass.mlPipeline, aPass.msName + " " + aPass.msDefines);

		return true;
	}

	void cLowLevelGraphicsSDL::DeleteFullscreenPass(cFullscreenPass &aPass)
	{
		if(aPass.mlPipeline) glDeleteProgramPipelines(1,&aPass.mlPipeline);
		if(aPass.mlVertex) glDeleteProgram(aPass.mlVertex);
		if(aPass.mlFragment) glDeleteProgram(aPass.mlFragment);
		aPass.mlPipeline = aPass.mlVertex = aPass.mlFragment = 0;
	}

	//-----------------------------------------------------------------------

	// Draws one triangle covering a viewport of avSize with the pass's
	// shaders and alTexture (2D) on unit 0, and the other two, if any, on
	// units 1 and 2, into the bound framebuffer. They are read as their own
	// parameters say, not through the samplers of the units. With abStencil
	// the stencil test is on, as the caller has set it. Uniforms are set by
	// the caller. All state is put back; the renderer does not expect it to
	// change.
	void cLowLevelGraphicsSDL::DrawFullscreenPass(cFullscreenPass &aPass, GLuint alTexture, const cVector2l &avSize,
													GLuint alTexture1, GLuint alTexture2, bool abStencil)
	{
		const GLuint vTextures[3] = {alTexture, alTexture1, alTexture2};
		GLint lLastUnit = 0, lLastPipeline = 0;
		GLint vLastTextures[3] = {0, 0, 0};
		glGetIntegerv(GL_ACTIVE_TEXTURE, &lLastUnit);
		glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &lLastPipeline);
		for(int i=2; i>=0; --i)
		{
			if(vTextures[i] == 0) continue;
			glActiveTexture(GL_TEXTURE0 + i);
			glGetIntegerv(GL_TEXTURE_BINDING_2D, &vLastTextures[i]);
			glBindTexture(GL_TEXTURE_2D, vTextures[i]);
			glBindSampler(i, 0);
		}

		static const GLenum vCaps[] = {GL_DEPTH_TEST, GL_STENCIL_TEST, GL_BLEND, GL_SCISSOR_TEST, GL_CULL_FACE,
										GL_SAMPLE_ALPHA_TO_COVERAGE,
										GL_CLIP_DISTANCE0, GL_CLIP_DISTANCE1, GL_CLIP_DISTANCE2,
										GL_CLIP_DISTANCE3, GL_CLIP_DISTANCE4, GL_CLIP_DISTANCE5};
		const int lCaps = sizeof(vCaps) / sizeof(vCaps[0]);
		GLboolean vLastCaps[lCaps];
		GLboolean bLastDepthMask = GL_TRUE;
		GLboolean vLastColorMask[4];
		GLint vLastViewport[4];
		for(int i=0; i<lCaps; ++i) vLastCaps[i] = glIsEnabled(vCaps[i]);
		glGetBooleanv(GL_DEPTH_WRITEMASK, &bLastDepthMask);
		glGetBooleanv(GL_COLOR_WRITEMASK, vLastColorMask);
		glGetIntegerv(GL_VIEWPORT, vLastViewport);

		for(int i=0; i<lCaps; ++i) glDisable(vCaps[i]);
		if(abStencil) glEnable(GL_STENCIL_TEST);
		glDepthMask(GL_FALSE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glViewport(0, 0, avSize.x, avSize.y);

		glBindProgramPipeline(aPass.mlPipeline);

		// The vertex shader makes the triangle from the vertex numbers
		mGpuProfiler.CountDraw(GL_TRIANGLES, 3);
		BindVertexArray(mlEmptyVertexArray);
		glDrawArrays(GL_TRIANGLES, 0, 3);

		glBindProgramPipeline(lLastPipeline);
		for(int i=0; i<lCaps; ++i)
		{
			if(vLastCaps[i])	glEnable(vCaps[i]);
			else				glDisable(vCaps[i]);
		}
		glDepthMask(bLastDepthMask);
		glColorMask(vLastColorMask[0], vLastColorMask[1], vLastColorMask[2], vLastColorMask[3]);
		glViewport(vLastViewport[0], vLastViewport[1], vLastViewport[2], vLastViewport[3]);
		for(int i=0; i<3; ++i)
		{
			if(vTextures[i] == 0) continue;
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, vLastTextures[i]);
			glBindSampler(i, mvUnitSampler[i]);
		}
		glActiveTexture(lLastUnit);
	}

	//-----------------------------------------------------------------------

	// Replaces the hardware gamma ramp: the output colour is drawn to the
	// bound framebuffer through the same curve SDL_SetGamma() used,
	// out = in ^ (1 / gamma). Returns false when the gamma is 1 or the pass
	// is missing, and nothing was drawn.
	bool cLowLevelGraphicsSDL::DrawGammaPass()
	{
		if(mfGammaCorrection > 0.999f && mfGammaCorrection < 1.001f) return false;
		if(CreateFullscreenPass(mGammaPass)==false) return false;

		glProgramUniform1f(mGammaPass.mlFragment, glGetUniformLocation(mGammaPass.mlFragment, "gamma"),
							mfGammaCorrection);
		DrawFullscreenPass(mGammaPass, mlOutputColor, mvScreenSize);
		return true;
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DrawTri(const tVertexVec &avVtx)
	{
		assert(avVtx.size()==3);

		DrawTri(avVtx.data());
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DrawTri(const cVertex* avVtx)
	{
		ImmediateBegin(1);
		for(int i=0;i<3;i++) ImmediateVertex(avVtx[i].pos, avVtx[i].col, &avVtx[i].tex);
		ImmediateDraw(GL_TRIANGLES);
	}

	//-----------------------------------------------------------------------

	// Quads are drawn as fans: the triangles 0,1,2 and 0,2,3.
	void cLowLevelGraphicsSDL::DrawQuad(const tVertexVec &avVtx)
	{
		assert(avVtx.size()==4);

		ImmediateBegin(1);
		for(int i=0;i<4;i++) ImmediateVertex(avVtx[i].pos, avVtx[i].col, &avVtx[i].tex);
		ImmediateDraw(GL_TRIANGLE_FAN);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DrawQuadMultiTex(const tVertexVec &avVtx,const tVector3fVec &avExtraUvs)
	{
		int lExtraUnits = cMath::Min((int)avExtraUvs.size()/4, cFixedFunctionShaders::kTexCoordUnits - 1);

		ImmediateBegin(1 + lExtraUnits);
		for(int i=0;i<4;i++)
		{
			cVector3f vTex[cFixedFunctionShaders::kTexCoordUnits];
			vTex[0] = avVtx[i].tex;
			for(int unit=0; unit<lExtraUnits; ++unit) vTex[unit + 1] = avExtraUvs[unit*4 + i];

			ImmediateVertex(avVtx[i].pos, avVtx[i].col, vTex);
		}
		ImmediateDraw(GL_TRIANGLE_FAN);
	}


	//-----------------------------------------------------------------------

	iOcclusionQuery* cLowLevelGraphicsSDL::CreateOcclusionQuery()
	{
		return hplNew(cOcclusionQueryOGL, () );
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DestroyOcclusionQuery(iOcclusionQuery *apQuery)
	{
		if(apQuery)	hplDelete(apQuery);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::ClearScreen()
	{
		GLbitfield bitmask=0;

		if(mbClearColor)bitmask |= GL_COLOR_BUFFER_BIT;
		if(mbClearDepth)bitmask |= GL_DEPTH_BUFFER_BIT;
		if(mbClearStencil)bitmask |= GL_STENCIL_BUFFER_BIT;

		glClear(bitmask);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetClearColor(const cColor& aCol){
		glClearColor(aCol.r, aCol.g, aCol.b, aCol.a);
	}
	void cLowLevelGraphicsSDL::SetClearDepth(float afDepth){
		glClearDepth(afDepth);
	}
	void cLowLevelGraphicsSDL::SetClearStencil(int alVal){
		glClearStencil(alVal);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetClearColorActive(bool abX){
		mbClearColor=abX;
	}
	void cLowLevelGraphicsSDL::SetClearDepthActive(bool abX){
		mbClearDepth=abX;
	}
	void cLowLevelGraphicsSDL::SetClearStencilActive(bool abX){
		mbClearStencil=abX;
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetColorWriteActive(bool abR,bool abG,bool abB,bool abA)
	{
		glColorMask(abR,abG,abB,abA);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetDepthWriteActive(bool abX)
	{
		glDepthMask(abX);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetDepthTestActive(bool abX)
	{
		if(abX) glEnable(GL_DEPTH_TEST);
		else glDisable(GL_DEPTH_TEST);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetDepthTestFunc(eDepthTestFunc aFunc)
	{
		glDepthFunc(GetGLDepthTestFuncEnum(aFunc));
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetAlphaTestActive(bool abX)
	{
		if(mbAlphaTest == abX) return;
		mbAlphaTest = abX;
		mbStateChanged = true;
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetAlphaTestFunc(eAlphaTestFunc aFunc,float afRef)
	{
		// The reference is clamped, as glAlphaFunc did
		afRef = cMath::Clamp(afRef, 0.0f, 1.0f);
		if(mAlphaTestFunc == aFunc && mfAlphaTestRef == afRef) return;
		mAlphaTestFunc = aFunc;
		mfAlphaTestRef = afRef;
		mbStateChanged = true;
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetStencilActive(bool abX)
	{
		if(abX) glEnable(GL_STENCIL_TEST);
		else glDisable(GL_STENCIL_TEST);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetStencil(eStencilFunc aFunc,int alRef, unsigned int aMask,
					eStencilOp aFailOp,eStencilOp aZFailOp,eStencilOp aZPassOp)
	{
		glStencilFunc(GetGLStencilFuncEnum(aFunc), alRef, aMask);

		glStencilOp(GetGLStencilOpEnum(aFailOp), GetGLStencilOpEnum(aZFailOp),
					GetGLStencilOpEnum(aZPassOp));
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetStencilTwoSide(eStencilFunc aFrontFunc,eStencilFunc aBackFunc,
					int alRef, unsigned int aMask,
					eStencilOp aFrontFailOp,eStencilOp aFrontZFailOp,eStencilOp aFrontZPassOp,
					eStencilOp aBackFailOp,eStencilOp aBackZFailOp,eStencilOp aBackZPassOp)
	{
		glStencilFuncSeparate(GL_FRONT, GetGLStencilFuncEnum(aFrontFunc), alRef, aMask);
		glStencilOpSeparate(GL_FRONT, GetGLStencilOpEnum(aFrontFailOp), GetGLStencilOpEnum(aFrontZFailOp),
							GetGLStencilOpEnum(aFrontZPassOp));

		glStencilFuncSeparate(GL_BACK, GetGLStencilFuncEnum(aBackFunc), alRef, aMask);
		glStencilOpSeparate(GL_BACK, GetGLStencilOpEnum(aBackFailOp), GetGLStencilOpEnum(aBackZFailOp),
							GetGLStencilOpEnum(aBackZPassOp));
	}

	void cLowLevelGraphicsSDL::SetStencilTwoSide(bool abX)
	{
		// Front and back state are always separate; SetStencil() sets both.
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetCullActive(bool abX)
	{
		if(abX) glEnable(GL_CULL_FACE);
		else glDisable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}
	void cLowLevelGraphicsSDL::SetCullMode(eCullMode aMode)
	{
		glCullFace(GL_BACK);
		if(aMode == eCullMode_Clockwise) glFrontFace(GL_CCW);
		else							glFrontFace(GL_CW);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetScissorActive(bool abX)
	{
		if(abX) glEnable(GL_SCISSOR_TEST);
		else glDisable(GL_SCISSOR_TEST);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetScissorRect(const cRect2l &aRect)
	{
		glScissor(aRect.x, (mvTargetSize.y - aRect.y - 1)-aRect.h, aRect.w, aRect.h);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetBlendActive(bool abX)
	{
		if(abX)
			glEnable(GL_BLEND);
		else
			glDisable(GL_BLEND);

		if(mbBlend != abX) mbStateChanged = true;
		mbBlend = abX;
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetBlendFunc(eBlendFunc aSrcFactor, eBlendFunc aDestFactor)
	{
		glBlendFunc(GetGLBlendEnum(aSrcFactor),GetGLBlendEnum(aDestFactor));

		if(mBlendSrc != aSrcFactor || mBlendDest != aDestFactor) mbStateChanged = true;
		mBlendSrc = aSrcFactor;
		mBlendDest = aDestFactor;
	}

	//-----------------------------------------------------------------------


	void cLowLevelGraphicsSDL::SetBlendFuncSeparate(eBlendFunc aSrcFactorColor, eBlendFunc aDestFactorColor,
		eBlendFunc aSrcFactorAlpha, eBlendFunc aDestFactorAlpha)
	{
		glBlendFuncSeparate(GetGLBlendEnum(aSrcFactorColor),
							GetGLBlendEnum(aDestFactorColor),
							GetGLBlendEnum(aSrcFactorAlpha),
							GetGLBlendEnum(aDestFactorAlpha));
	}


	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DrawQuad(const tVertexVec &avVtx, const cColor aCol)
	{
		assert(avVtx.size()==4);

		ImmediateBegin(1);
		for(int i=0;i<4;i++) ImmediateVertex(avVtx[i].pos, aCol, &avVtx[i].tex);
		ImmediateDraw(GL_TRIANGLE_FAN);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DrawQuad(const tVertexVec &avVtx,const float afZ)
	{
		assert(avVtx.size()==4);

		ImmediateBegin(1);
		for(int i=0;i<4;i++)
		{
			cVector3f vTex(avVtx[i].tex.x, avVtx[i].tex.y, afZ);
			ImmediateVertex(avVtx[i].pos, avVtx[i].col, &vTex);
		}
		ImmediateDraw(GL_TRIANGLE_FAN);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DrawQuad(const tVertexVec &avVtx,const float afZ,const cColor &aCol)
	{
		assert(avVtx.size()==4);

		ImmediateBegin(1);
		for(int i=0;i<4;i++)
		{
			cVector3f vTex(avVtx[i].tex.x, avVtx[i].tex.y, afZ);
			ImmediateVertex(avVtx[i].pos, aCol, &vTex);
		}
		ImmediateDraw(GL_TRIANGLE_FAN);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::AddVertexToBatch(const cVertex *apVtx)
	{
		//Coord
		mpVertexArray[mlVertexCount + 0] =	apVtx->pos.x;
		mpVertexArray[mlVertexCount + 1] =	apVtx->pos.y;
		mpVertexArray[mlVertexCount + 2] =	apVtx->pos.z;
		//Color
		mpVertexArray[mlVertexCount + 3] =	apVtx->col.r;
		mpVertexArray[mlVertexCount + 4] =	apVtx->col.g;
		mpVertexArray[mlVertexCount + 5] =	apVtx->col.b;
		mpVertexArray[mlVertexCount + 6] =	apVtx->col.a;
		//Texture coord
		mpVertexArray[mlVertexCount + 7] =	apVtx->tex.x;
		mpVertexArray[mlVertexCount + 8] =	apVtx->tex.y;
		mpVertexArray[mlVertexCount + 9] =	apVtx->tex.z;
		//Normal coord
		mpVertexArray[mlVertexCount + 10] =	apVtx->norm.x;
		mpVertexArray[mlVertexCount + 11] =	apVtx->norm.y;
		mpVertexArray[mlVertexCount + 12] =	apVtx->norm.z;

		mlVertexCount = mlVertexCount + mlBatchStride;

		if(mlVertexCount/mlBatchStride >= mlBatchArraySize)
		{
			//Make the array larger.
		}
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::AddVertexToBatch(const cVertex *apVtx, const cVector3f* avTransform)
	{
		//Coord
		mpVertexArray[mlVertexCount + 0] =	apVtx->pos.x+avTransform->x;
		mpVertexArray[mlVertexCount + 1] =	apVtx->pos.y+avTransform->y;
		mpVertexArray[mlVertexCount + 2] =	apVtx->pos.z+avTransform->z;

		/*Log("Trans: %s\n",avTransform->ToString().c_str());
		Log("Adding: %f:%f:%f\n",mpVertexArray[mlVertexCount + 0],
									mpVertexArray[mlVertexCount + 1],
									mpVertexArray[mlVertexCount + 2]);*/
		//Color
		mpVertexArray[mlVertexCount + 3] =	apVtx->col.r;
		mpVertexArray[mlVertexCount + 4] =	apVtx->col.g;
		mpVertexArray[mlVertexCount + 5] =	apVtx->col.b;
		mpVertexArray[mlVertexCount + 6] =	apVtx->col.a;
		//Texture coord
		mpVertexArray[mlVertexCount + 7] =	apVtx->tex.x;
		mpVertexArray[mlVertexCount + 8] =	apVtx->tex.y;
		mpVertexArray[mlVertexCount + 9] =	apVtx->tex.z;

		/*Log("Tex: %f:%f:%f\n",mpVertexArray[mlVertexCount + 7],
			mpVertexArray[mlVertexCount + 8],
			mpVertexArray[mlVertexCount + 9]);*/

		//Normal coord
		mpVertexArray[mlVertexCount + 10] =	apVtx->norm.x;
		mpVertexArray[mlVertexCount + 11] =	apVtx->norm.y;
		mpVertexArray[mlVertexCount + 12] =	apVtx->norm.z;

		mlVertexCount = mlVertexCount + mlBatchStride;

		if(mlVertexCount/mlBatchStride >= mlBatchArraySize)
		{
			//Make the array larger.
		}
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::AddVertexToBatch(const cVertex *apVtx, const cMatrixf* aMtx)
	{

	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::AddVertexToBatch_Size2D(const cVertex *apVtx, const cVector3f* avTransform,
		const cColor* apCol,const float& mfW, const float& mfH)
	{
		//Coord
		mpVertexArray[mlVertexCount + 0] =	avTransform->x + mfW;
		mpVertexArray[mlVertexCount + 1] =	avTransform->y + mfH;
		mpVertexArray[mlVertexCount + 2] =	avTransform->z;

		//Color
		mpVertexArray[mlVertexCount + 3] =	apCol->r;
		mpVertexArray[mlVertexCount + 4] =	apCol->g;
		mpVertexArray[mlVertexCount + 5] =	apCol->b;
		mpVertexArray[mlVertexCount + 6] =	apCol->a;

		//Texture coord
		mpVertexArray[mlVertexCount + 7] =	apVtx->tex.x;
		mpVertexArray[mlVertexCount + 8] =	apVtx->tex.y;
		mpVertexArray[mlVertexCount + 9] =	apVtx->tex.z;


		mlVertexCount = mlVertexCount + mlBatchStride;

		if(mlVertexCount/mlBatchStride >= mlBatchArraySize)
		{
			//Make the array larger.
		}
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::AddVertexToBatch_Raw(	const cVector3f& avPos, const cColor &aColor,
														const cVector3f& avTex)
	{
		//Coord
		mpVertexArray[mlVertexCount + 0] =	avPos.x;
		mpVertexArray[mlVertexCount + 1] =	avPos.y;
		mpVertexArray[mlVertexCount + 2] =	avPos.z;

		//Color
		mpVertexArray[mlVertexCount + 3] =	aColor.r;
		mpVertexArray[mlVertexCount + 4] =	aColor.g;
		mpVertexArray[mlVertexCount + 5] =	aColor.b;
		mpVertexArray[mlVertexCount + 6] =	aColor.a;

		//Texture coord
		mpVertexArray[mlVertexCount + 7] =	avTex.x;
		mpVertexArray[mlVertexCount + 8] =	avTex.y;
		mpVertexArray[mlVertexCount + 9] =	avTex.z;


		mlVertexCount = mlVertexCount + mlBatchStride;
	}


	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::AddIndexToBatch(int alIndex)
	{
		mpIndexArray[mlIndexCount] = alIndex;
		mlIndexCount++;

		if(mlIndexCount>=mlBatchArraySize)
		{
			//Make the array larger.
		}
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::AddTexCoordToBatch(unsigned int alUnit,const cVector3f *apCoord)
	{
		unsigned int lCount = mlTexCoordArrayCount[alUnit];

		mpTexCoordArray[alUnit][lCount+0] = apCoord->x;
		mpTexCoordArray[alUnit][lCount+1] = apCoord->y;
		mpTexCoordArray[alUnit][lCount+2] = apCoord->z;

		mlTexCoordArrayCount[alUnit]+=3;
	}

	//-----------------------------------------------------------------------

	// Flushing has always pointed every unit at the vertices' own texture
	// coordinates, so the extra arrays are not drawn with.
	void cLowLevelGraphicsSDL::SetBatchTextureUnitActive(unsigned int alUnit,bool abActive)
	{
		mbTexCoordArrayActive[alUnit] = abActive;
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::FlushTriBatch(tVtxBatchFlag aTypeFlags, bool abAutoClear)
	{
		DrawBatch(aTypeFlags, false);

		if(abAutoClear){
			mlIndexCount = 0;
			mlVertexCount = 0;
			for(int i=0;i<MAX_TEXTUREUNITS;i++)
				mlTexCoordArrayCount[i]=0;
		}
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::FlushQuadBatch(tVtxBatchFlag aTypeFlags, bool abAutoClear)
	{
		DrawBatch(aTypeFlags, true);

		if(abAutoClear){
			mlIndexCount = 0;
			mlVertexCount = 0;
			for(int i=0;i<MAX_TEXTUREUNITS;i++)
				mlTexCoordArrayCount[i]=0;
		}
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::ClearBatch()
	{
		mlIndexCount = 0;
		mlVertexCount = 0;
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DrawLine(const cVector3f& avBegin, const cVector3f& avEnd, cColor aCol)
	{
		SetTexture(0,nullptr);
		//SetBlendActive(false);
		ImmediateBegin(0);
		ImmediateVertex(avBegin, aCol);
		ImmediateVertex(avEnd, aCol);
		ImmediateDraw(GL_LINES);
	}

	void cLowLevelGraphicsSDL::DrawBoxMaxMin(const cVector3f& avMax, const cVector3f& avMin, cColor aCol)
	{
		SetTexture(0,nullptr);
		SetBlendActive(false);

		// Corner i: bit 0 picks x, bit 1 y, bit 2 z from the minimum
		static const int vEdges[12][2] = {
			{0,1}, {0,2}, {1,3}, {3,2}, //Pos Z Quad
			{4,5}, {4,6}, {5,7}, {7,6}, //Neg Z Quad
			{0,4}, {1,5}, {3,7}, {2,6}  //Lines between
		};

		ImmediateBegin(0);
		for(int i=0; i<12; ++i)
		{
			for(int j=0; j<2; ++j)
			{
				int lCorner = vEdges[i][j];
				ImmediateVertex(cVector3f(	lCorner & 1 ? avMin.x : avMax.x,
											lCorner & 2 ? avMin.y : avMax.y,
											lCorner & 4 ? avMin.z : avMax.z), aCol);
			}
		}
		ImmediateDraw(GL_LINES);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DrawSphere(const cVector3f& avPos, float afRadius, cColor aCol)
	{
		int alSegments = 32;
		float afAngleStep = k2Pif /(float)alSegments;

		SetTexture(0,nullptr);
		SetBlendActive(false);

		ImmediateBegin(0);

		//X Circle:
		for(float a=0; a< k2Pif; a+= afAngleStep)
		{
			ImmediateVertex(cVector3f(avPos.x, avPos.y + sin(a)*afRadius,
									avPos.z + cos(a)*afRadius), aCol);

			ImmediateVertex(cVector3f(avPos.x, avPos.y + sin(a+afAngleStep)*afRadius,
									avPos.z + cos(a+afAngleStep)*afRadius), aCol);
		}

		//Y Circle:
		for(float a=0; a< k2Pif; a+= afAngleStep)
		{
			ImmediateVertex(cVector3f(avPos.x + cos(a)*afRadius, avPos.y,
									avPos.z + sin(a)*afRadius), aCol);

			ImmediateVertex(cVector3f(avPos.x + cos(a+afAngleStep)*afRadius, avPos.y ,
									avPos.z+ sin(a+afAngleStep)*afRadius), aCol);
		}

		//Z Circle:
		for(float a=0; a< k2Pif; a+= afAngleStep)
		{
			ImmediateVertex(cVector3f(avPos.x + cos(a)*afRadius, avPos.y + sin(a)*afRadius, avPos.z), aCol);

			ImmediateVertex(cVector3f(avPos.x + cos(a+afAngleStep)*afRadius,
									avPos.y + sin(a+afAngleStep)*afRadius,
									avPos.z), aCol);
		}

		ImmediateDraw(GL_LINES);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DrawLine2D(const cVector2f& avBegin, const cVector2f& avEnd, float afZ, cColor aCol)
	{
		SetTexture(0,nullptr);
		SetBlendActive(false);
		ImmediateBegin(0);
		ImmediateVertex(cVector3f(avBegin.x,avBegin.y,afZ), aCol);
		ImmediateVertex(cVector3f(avEnd.x,avEnd.y,afZ), aCol);
		ImmediateDraw(GL_LINES);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DrawLineRect2D(const cRect2f& aRect, float afZ, cColor aCol)
	{
		SetTexture(0, nullptr);
		SetBlendActive(false);
		ImmediateBegin(0);
		ImmediateVertex(cVector3f(aRect.x,aRect.y,afZ), aCol);
		ImmediateVertex(cVector3f(aRect.x+aRect.w,aRect.y,afZ), aCol);
		ImmediateVertex(cVector3f(aRect.x+aRect.w,aRect.y+aRect.h,afZ), aCol);
		ImmediateVertex(cVector3f(aRect.x,aRect.y+aRect.h,afZ), aCol);
		ImmediateVertex(cVector3f(aRect.x,aRect.y,afZ), aCol);
		ImmediateDraw(GL_LINE_STRIP);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DrawFilledRect2D(const cRect2f& aRect, float afZ, cColor aCol)
	{
		SetTexture(0, nullptr);
		ImmediateBegin(0);
		ImmediateVertex(cVector3f(aRect.x,aRect.y,afZ), aCol);
		ImmediateVertex(cVector3f(aRect.x+aRect.w,aRect.y,afZ), aCol);
		ImmediateVertex(cVector3f(aRect.x+aRect.w,aRect.y+aRect.h,afZ), aCol);
		ImmediateVertex(cVector3f(aRect.x,aRect.y+aRect.h,afZ), aCol);
		ImmediateDraw(GL_TRIANGLE_FAN);
	}

	//-----------------------------------------------------------------------

	/**
	 * \todo Implement this.
	 * \param avCenter
	 * \param afRadius
	 * \param afZ
	 * \param aCol
	 */
	void cLowLevelGraphicsSDL::DrawLineCircle2D(const cVector2f& avCenter, float afRadius, float afZ, cColor aCol)
	{
		//Implement later
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::CopyContextToTexure(iTexture* apTex, const cVector2l &avPos,
							const cVector2l &avSize, const cVector2l &avTexOffset)
	{
		if(apTex==nullptr)return;

		int lScreenY = (mvTargetSize.y - avSize.y) - avPos.y;
		int lTexY = (apTex->GetHeight() - avSize.y) - avTexOffset.y;

		//Log("TExoffset: %d %d\n",avTexOffset.x,lTexY);
		//Log("ScreenOffset: %d %d (h: %d s: %d p: %d)\n",avPos.x,lScreenY,mvScreenSize.y,
		//												avSize.y,avPos.y);

		SetTexture(0, apTex);

		// A multisampled framebuffer can't be copied from; resolve the area
		// first and copy from the resolved one.
		bool bResolve = mlMultisampling > 0 && mbInScene && mbSceneResolved==false && mpRenderTarget == nullptr;
		if(bResolve)
		{
			BlitScene(GL_COLOR_BUFFER_BIT, avPos.x, lScreenY, avPos.x + avSize.x, lScreenY + avSize.y);
			glBindFramebuffer(GL_FRAMEBUFFER, mlResolveFramebuffer);
		}

		glCopyTexSubImage2D(GetGLTextureTargetEnum(apTex->GetTarget()),0,
							avTexOffset.x, lTexY,
							avPos.x, lScreenY, avSize.x, avSize.y);

		if(bResolve) BindCurrentFramebuffer();
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetRenderTarget(iTexture* pTex)
	{
		if(pTex == mpRenderTarget) return;
		mpRenderTarget = pTex;
		mbStateChanged = true;

		if(pTex == nullptr)
		{
			if(mbInScene)	BindFramebuffer(GetSceneFramebuffer(), mvRenderSize);
			else			BindFramebuffer(mlOutputFramebuffer, mvScreenSize);
			return;
		}

		if(mlTargetFramebuffer == 0) glGenFramebuffers(1, &mlTargetFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, mlTargetFramebuffer);
		cGpuProfiler::Label(GL_FRAMEBUFFER, mlTargetFramebuffer, "Render target");
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GetGLTextureTargetEnum(pTex->GetTarget()),
								static_cast<cSDLTexture*>(pTex)->GetTextureHandle(), 0);
		BindFramebuffer(mlTargetFramebuffer, cVector2l(pTex->GetWidth(), pTex->GetHeight()));
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetSceneColorOnly(bool abX)
	{
		if(mbInScene==false || abX == mbSceneColorOnly) return;
		mbSceneColorOnly = abX;

		if(mpRenderTarget==nullptr) BindFramebuffer(GetSceneFramebuffer(), mvRenderSize);
	}

	//-----------------------------------------------------------------------

	// The depth texture of a kind of shadow map, at a new size. It is left
	// bound to the two shadow map units, where SetTexture() never binds.
	void cLowLevelGraphicsSDL::CreateShadowMap(bool abCube, int alSize)
	{
		const int lIdx = abCube ? 1 : 0;
		const GLenum lTarget = abCube ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;

		if(mvShadowSamplers[0] == 0)
		{
			glGenSamplers(2, mvShadowSamplers);
			for(int i=0; i<2; ++i)
			{
				GLuint lSampler = mvShadowSamplers[i];
				GLenum lFilter = i == 0 ? GL_LINEAR : GL_NEAREST;
				glSamplerParameteri(lSampler, GL_TEXTURE_MIN_FILTER, lFilter);
				glSamplerParameteri(lSampler, GL_TEXTURE_MAG_FILTER, lFilter);
				// Past the edge of a spotlight's map there is nothing in the
				// way: the edge's texels repeated would stretch what they hold
				// outwards. (A cube map has no edge, its faces are seamless.)
				const GLfloat vBorder[4] = {1, 1, 1, 1};
				glSamplerParameteri(lSampler, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
				glSamplerParameteri(lSampler, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
				glSamplerParameteri(lSampler, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
				glSamplerParameterfv(lSampler, GL_TEXTURE_BORDER_COLOR, vBorder);
				if(i == 0)
				{
					glSamplerParameteri(lSampler, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
					glSamplerParameteri(lSampler, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
				}
				glBindSampler(cGLSLProgram::kShadowMapUnit + i, lSampler);
			}
		}

		if(mvShadowMaps[lIdx]) glDeleteTextures(1, &mvShadowMaps[lIdx]);
		glGenTextures(1, &mvShadowMaps[lIdx]);
		mvShadowMapSizes[lIdx] = alSize;

		for(int i=0; i<2; ++i)
		{
			glActiveTexture(GL_TEXTURE0 + cGLSLProgram::kShadowMapUnit + i);
			glBindTexture(lTarget, mvShadowMaps[lIdx]);
		}
		if(abCube)
		{
			for(int i=0; i<6; ++i)
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT24, alSize, alSize, 0,
								GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, alSize, alSize, 0,
							GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);
		}
		glTexParameteri(lTarget, GL_TEXTURE_MAX_LEVEL, 0);
		glActiveTexture(GL_TEXTURE0 + mlActiveTextureUnit);

		cGpuProfiler::Label(GL_TEXTURE, mvShadowMaps[lIdx], abCube ? "Shadow cube map" : "Shadow map");
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::BeginShadowMap(bool abCube, int alFace, int alSize)
	{
		const int lIdx = abCube ? 1 : 0;
		alSize = cMath::Clamp(alSize, 16, 8192);
		if(mvShadowMapSizes[lIdx] != alSize) CreateShadowMap(abCube, alSize);

		if(mlShadowFramebuffer == 0)
		{
			glGenFramebuffers(1, &mlShadowFramebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, mlShadowFramebuffer);
			cGpuProfiler::Label(GL_FRAMEBUFFER, mlShadowFramebuffer, "Shadow map");
			glDrawBuffer(GL_NONE);
			glReadBuffer(GL_NONE);
		}
		glBindFramebuffer(GL_FRAMEBUFFER, mlShadowFramebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
								abCube ? GL_TEXTURE_CUBE_MAP_POSITIVE_X + alFace : GL_TEXTURE_2D,
								mvShadowMaps[lIdx], 0);
		BindFramebuffer(mlShadowFramebuffer, cVector2l(alSize, alSize));
		mbInShadowMap = true;
		mbStateChanged = true;

		glEnable(GL_DEPTH_CLAMP);
		glDepthMask(GL_TRUE);
		glClearDepth(1);
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	void cLowLevelGraphicsSDL::EndShadowMap()
	{
		glDisable(GL_DEPTH_CLAMP);
		mbInShadowMap = false;
		mbStateChanged = true;

		if(mpRenderTarget)
		{
			iTexture *pTarget = mpRenderTarget;
			mpRenderTarget = nullptr;
			SetRenderTarget(pTarget);
		}
		else if(mbInScene)	BindFramebuffer(GetSceneFramebuffer(), mvRenderSize);
		else				BindFramebuffer(mlOutputFramebuffer, mvScreenSize);
	}

	void cLowLevelGraphicsSDL::SetLightLook(const cLightLook& aLook)
	{
		mLightLook = aLook;
		mbStateChanged = true;
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetShadowMapParams(const cShadowMapParams& aParams)
	{
		mShadowParams = aParams;
		mbStateChanged = true;
	}

	//-----------------------------------------------------------------------


	cVector2f cLowLevelGraphicsSDL::GetScreenSize()
	{
		return cVector2f((float)mvScreenSize.x, (float)mvScreenSize.y);
	}

	cVector2f cLowLevelGraphicsSDL::GetRenderSize()
	{
		return cVector2f((float)mvRenderSize.x, (float)mvRenderSize.y);
	}

	//-----------------------------------------------------------------------

	cVector2f cLowLevelGraphicsSDL::GetVirtualSize()
	{
		return mvVirtualSize;
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetVirtualSize(cVector2f avSize)
	{
		mvVirtualSize = avSize;
	}

	//-----------------------------------------------------------------------

	cVector2f cLowLevelGraphicsSDL::GetVirtualOffset()
	{
		if(mvScreenSize.x <= 0 || mvScreenSize.y <= 0) return cVector2f(0,0);

		// One virtual unit is the same number of pixels on both axes, so the
		// axis with pixels to spare gets a border on each side.
		float fScaleX = (float)mvScreenSize.x / mvVirtualSize.x;
		float fScaleY = (float)mvScreenSize.y / mvVirtualSize.y;
		float fScale = fScaleX < fScaleY ? fScaleX : fScaleY;

		return cVector2f(	((float)mvScreenSize.x / fScale - mvVirtualSize.x) * 0.5f,
							((float)mvScreenSize.y / fScale - mvVirtualSize.y) * 0.5f);
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::BindVertexArray(GLuint alVertexArray)
	{
		if(alVertexArray == mlBoundVertexArray) return;
		glBindVertexArray(alVertexArray);
		mlBoundVertexArray = alVertexArray;
	}

	//-----------------------------------------------------------------------

	// Copies data to the stream buffer bound to aTarget and returns its
	// offset. A full buffer is replaced by a new one, so that what was
	// drawn from it stays as it was.
	static size_t StreamToBuffer(GLenum aTarget, size_t &alSize, size_t &alOffset,
								const void *apData, size_t alBytes)
	{
		if(alOffset + alBytes > alSize)
		{
			while(alBytes > alSize) alSize *= 2;
			glBufferData(aTarget, alSize, nullptr, GL_STREAM_DRAW);
			alOffset = 0;
		}

		glBufferSubData(aTarget, alOffset, alBytes, apData);
		size_t lStart = alOffset;
		alOffset += alBytes;
		return lStart;
	}

	//-----------------------------------------------------------------------

	const void* cLowLevelGraphicsSDL::StreamIndices(const unsigned int *apIndices, int alCount)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mlStreamIndexBuffer);
		return (const void*)StreamToBuffer(GL_ELEMENT_ARRAY_BUFFER, mlStreamIndexSize, mlStreamIndexOffset,
											apIndices, alCount * sizeof(unsigned int));
	}

	//-----------------------------------------------------------------------

	// Points an attribute of the stream vertex array at floats in the
	// stream buffer (bound to GL_ARRAY_BUFFER) and adds it to alArrays.
	void cLowLevelGraphicsSDL::StreamArray(unsigned int &alArrays, int alAttrib, int alElements,
											size_t alStride, size_t alOffset)
	{
		glVertexAttribPointer(alAttrib, alElements, GL_FLOAT, GL_FALSE, (GLsizei)alStride, (const void*)alOffset);
		alArrays |= 1u << alAttrib;
	}

	// Enables the arrays in alArrays and disables the others.
	void cLowLevelGraphicsSDL::SetStreamArrays(unsigned int alArrays)
	{
		unsigned int lChanged = alArrays ^ mlStreamArrays;
		for(int i=0; lChanged && i<cGLSLProgram::eVertexAttrib_LastEnum; ++i)
		{
			if((lChanged & (1u << i))==0) continue;
			if(alArrays & (1u << i))	glEnableVertexAttribArray(i);
			else						glDisableVertexAttribArray(i);
		}
		mlStreamArrays = alArrays;
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetCurrentAttrib(int alAttrib, float afX, float afY, float afZ, float afW)
	{
		float *pVal = mvCurrentAttrib[alAttrib];
		pVal[0] = afX; pVal[1] = afY; pVal[2] = afZ; pVal[3] = afW;
		mlCurrentAttribChanged |= 1u << alAttrib;
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::ImmediateBegin(int alTexUnits)
	{
		mvImmediate.clear();
		mlImmediateTexUnits = alTexUnits;
	}

	// apTex has a coordinate for each of the texture units given to
	// ImmediateBegin().
	void cLowLevelGraphicsSDL::ImmediateVertex(const cVector3f &avPos, const cColor &aCol, const cVector3f *apTex)
	{
		mvImmediate.insert(mvImmediate.end(), {avPos.x, avPos.y, avPos.z, aCol.r, aCol.g, aCol.b, aCol.a});
		for(int i=0; i<mlImmediateTexUnits; ++i)
			mvImmediate.insert(mvImmediate.end(), {apTex[i].x, apTex[i].y, apTex[i].z});
	}

	void cLowLevelGraphicsSDL::ImmediateDraw(GLenum aMode)
	{
		const size_t lFloats = 7 + 3 * mlImmediateTexUnits;
		const size_t lStride = lFloats * sizeof(float);
		const int lVertices = (int)(mvImmediate.size() / lFloats);
		if(lVertices == 0) return;

		PrepareDraw();
		mGpuProfiler.CountDraw(aMode, lVertices);

		BindVertexArray(mlStreamVertexArray);
		glBindBuffer(GL_ARRAY_BUFFER, mlStreamVertexBuffer);
		size_t lOffset = StreamToBuffer(GL_ARRAY_BUFFER, mlStreamVertexSize, mlStreamVertexOffset,
										mvImmediate.data(), mvImmediate.size() * sizeof(float));

		unsigned int lArrays = 0;
		StreamArray(lArrays, cGLSLProgram::eVertexAttrib_Position, 3, lStride, lOffset);
		StreamArray(lArrays, cGLSLProgram::eVertexAttrib_Color, 4, lStride, lOffset + 3*sizeof(float));
		for(int i=0; i<mlImmediateTexUnits; ++i)
			StreamArray(lArrays, cGLSLProgram::eVertexAttrib_TexCoord0 + i, 3, lStride,
						lOffset + (7 + 3*i)*sizeof(float));
		SetStreamArrays(lArrays);

		glDrawArrays(aMode, 0, lVertices);

		// The last vertex's values stay as the current ones, as with glColor
		// and glTexCoord: draws without those arrays get them.
		const float *pLast = &mvImmediate[mvImmediate.size() - lFloats];
		SetCurrentAttrib(cGLSLProgram::eVertexAttrib_Color, pLast[3], pLast[4], pLast[5], pLast[6]);
		for(int i=0; i<mlImmediateTexUnits; ++i)
			SetCurrentAttrib(cGLSLProgram::eVertexAttrib_TexCoord0 + i, pLast[7 + 3*i], pLast[8 + 3*i],
							pLast[9 + 3*i], 1);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DrawBatch(tVtxBatchFlag aTypeFlags, bool abQuads)
	{
		const unsigned int *pIndices = mpIndexArray;
		int lIndexCount = (int)mlIndexCount;
		if(abQuads)
		{
			// Two triangles per quad: 0,1,2 and 0,2,3
			mvQuadIndices.clear();
			for(unsigned int i=0; i+3 < mlIndexCount; i+=4)
			{
				const unsigned int *pQuad = &mpIndexArray[i];
				mvQuadIndices.insert(mvQuadIndices.end(),
									{pQuad[0], pQuad[1], pQuad[2], pQuad[0], pQuad[2], pQuad[3]});
			}
			pIndices = mvQuadIndices.data();
			lIndexCount = (int)mvQuadIndices.size();
		}
		if(lIndexCount == 0 || mlVertexCount == 0) return;

		PrepareDraw();
		mGpuProfiler.CountDraw(GL_TRIANGLES, lIndexCount);

		BindVertexArray(mlStreamVertexArray);
		glBindBuffer(GL_ARRAY_BUFFER, mlStreamVertexBuffer);
		size_t lOffset = StreamToBuffer(GL_ARRAY_BUFFER, mlStreamVertexSize, mlStreamVertexOffset,
										mpVertexArray, mlVertexCount * sizeof(float));

		// See AddVertexToBatch(): position, colour, texture coordinate, normal
		const size_t lStride = sizeof(float) * mlBatchStride;
		const size_t lTexOffset = lOffset + 7*sizeof(float);
		unsigned int lArrays = 0;
		if(aTypeFlags & eVtxBatchFlag_Position)
			StreamArray(lArrays, cGLSLProgram::eVertexAttrib_Position, 3, lStride, lOffset);
		if(aTypeFlags & eVtxBatchFlag_Color0)
			StreamArray(lArrays, cGLSLProgram::eVertexAttrib_Color, 4, lStride, lOffset + 3*sizeof(float));
		if(aTypeFlags & eVtxBatchFlag_Normal)
			StreamArray(lArrays, cGLSLProgram::eVertexAttrib_Normal, 3, lStride, lOffset + 10*sizeof(float));
		if(aTypeFlags & eVtxBatchFlag_Texture0)
			StreamArray(lArrays, cGLSLProgram::eVertexAttrib_TexCoord0, 3, lStride, lTexOffset);
		if(aTypeFlags & eVtxBatchFlag_Texture1)
			StreamArray(lArrays, cGLSLProgram::eVertexAttrib_TexCoord0 + 1, 3, lStride, lTexOffset);
		if(aTypeFlags & eVtxBatchFlag_Texture2)
			StreamArray(lArrays, cGLSLProgram::eVertexAttrib_TexCoord0 + 2, 3, lStride, lTexOffset);
		SetStreamArrays(lArrays);

		const void *pIndexOffset = StreamIndices(pIndices, lIndexCount);
		glDrawElements(GL_TRIANGLES, lIndexCount, GL_UNSIGNED_INT, pIndexOffset);
	}

	//-----------------------------------------------------------------------

	GLenum cLowLevelGraphicsSDL::GetGLBlendEnum(eBlendFunc aType)
	{
		switch(aType)
		{
		case eBlendFunc_Zero:					return GL_ZERO;
		case eBlendFunc_One:					return GL_ONE;
		case eBlendFunc_SrcColor:				return GL_SRC_COLOR;
		case eBlendFunc_OneMinusSrcColor:		return GL_ONE_MINUS_SRC_COLOR;
		case eBlendFunc_DestColor:				return GL_DST_COLOR;
		case eBlendFunc_OneMinusDestColor:		return GL_ONE_MINUS_DST_COLOR;
		case eBlendFunc_SrcAlpha:				return GL_SRC_ALPHA;
		case eBlendFunc_OneMinusSrcAlpha:		return GL_ONE_MINUS_SRC_ALPHA;
		case eBlendFunc_DestAlpha:				return GL_DST_ALPHA;
		case eBlendFunc_OneMinusDestAlpha:		return GL_ONE_MINUS_DST_ALPHA;
		case eBlendFunc_SrcAlphaSaturate:		return GL_SRC_ALPHA_SATURATE;
		}
		return 0;
	}

	//-----------------------------------------------------------------------

	GLenum cLowLevelGraphicsSDL::GetGLTextureTargetEnum(eTextureTarget aType)
	{
		switch(aType)
		{
		case eTextureTarget_1D:		return GL_TEXTURE_1D;
		case eTextureTarget_2D:		return GL_TEXTURE_2D;
		case eTextureTarget_Rect:
			{
				return GL_TEXTURE_RECTANGLE;
			}
		case eTextureTarget_CubeMap:	return GL_TEXTURE_CUBE_MAP;
		case eTextureTarget_3D:		return GL_TEXTURE_3D;
		}
		return 0;
	}

	//-----------------------------------------------------------------------

	GLenum cLowLevelGraphicsSDL::GetGLDepthTestFuncEnum(eDepthTestFunc aType)
	{
		switch(aType)
		{
		case eDepthTestFunc_Never:			return GL_NEVER;
		case eDepthTestFunc_Less:				return GL_LESS;
		case eDepthTestFunc_LessOrEqual:		return GL_LEQUAL;
		case eDepthTestFunc_Greater:			return GL_GREATER;
		case eDepthTestFunc_GreaterOrEqual:	return GL_GEQUAL;
		case eDepthTestFunc_Equal:			return GL_EQUAL;
		case eDepthTestFunc_NotEqual:			return GL_NOTEQUAL;
		case eDepthTestFunc_Always:			return GL_ALWAYS;
		}
		return 0;
	}

	//-----------------------------------------------------------------------

	GLenum cLowLevelGraphicsSDL::GetGLStencilFuncEnum(eStencilFunc aType)
	{
		switch(aType)
		{
		case eStencilFunc_Never:			return GL_NEVER;
		case eStencilFunc_Less:				return GL_LESS;
		case eStencilFunc_LessOrEqual:		return GL_LEQUAL;
		case eStencilFunc_Greater:			return GL_GREATER;
		case eStencilFunc_GreaterOrEqual:	return GL_GEQUAL;
		case eStencilFunc_Equal:			return GL_EQUAL;
		case eStencilFunc_NotEqual:			return GL_NOTEQUAL;
		case eStencilFunc_Always:			return GL_ALWAYS;
		}
		return 0;
	}

	//-----------------------------------------------------------------------

	GLenum cLowLevelGraphicsSDL::GetGLStencilOpEnum(eStencilOp aType)
	{
		switch(aType)
		{
		case eStencilOp_Keep:			return GL_KEEP;
		case eStencilOp_Zero:			return GL_ZERO;
		case eStencilOp_Replace:		return GL_REPLACE;
		case eStencilOp_Increment:		return GL_INCR;
		case eStencilOp_Decrement:		return GL_DECR;
		case eStencilOp_Invert:			return GL_INVERT;
		case eStencilOp_IncrementWrap:	return GL_INCR_WRAP;
		case eStencilOp_DecrementWrap:	return GL_DECR_WRAP;
		}
		return 0;
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetFrameUniforms(const cMatrixf& a_mtxView, const cMatrixf& a_mtxProjection,
												const cVector3f& avEyePos, float afNear, float afFar,
												float afTime)
	{
		cVector2f vSize = GetRenderSize();

		cFrameUniformData data;
		CopyColumnMajor(data.mvView, a_mtxView);
		CopyColumnMajor(data.mvProjection, a_mtxProjection);
		CopyColumnMajor(data.mvViewProjection, cMath::MatrixMul(a_mtxProjection, a_mtxView));
		data.mvEyePos[0] = avEyePos.x; data.mvEyePos[1] = avEyePos.y; data.mvEyePos[2] = avEyePos.z;
		data.mvEyePos[3] = 1;
		data.mvRenderSize[0] = vSize.x; data.mvRenderSize[1] = vSize.y;
		data.mvRenderSize[2] = 1.0f / vSize.x; data.mvRenderSize[3] = 1.0f / vSize.y;
		data.mvDepthRange[0] = afNear; data.mvDepthRange[1] = afFar;
		data.mvDepthRange[2] = data.mvDepthRange[3] = 0;
		data.mvTime[0] = afTime;
		mfFrameTime = afTime;
		data.mvTime[1] = data.mvTime[2] = data.mvTime[3] = 0;
		data.mvShading0[0] = mShading.mbImproved ? 1.0f : 0.0f;
		data.mvShading0[1] = mShading.mfSpecular;
		data.mvShading0[2] = mShading.mfReflectance;
		data.mvShading0[3] = mShading.mfAmbientSheen;
		data.mvShading1[0] = mShading.mfRoughDull;
		data.mvShading1[1] = mShading.mfRoughShiny;
		data.mvShading1[2] = mShading.mfAmbientUp;
		data.mvShading1[3] = mShading.mfAmbientFacing;

		glBindBuffer(GL_UNIFORM_BUFFER, mlFrameUniformBuffer);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(data), &data);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}

	//-----------------------------------------------------------------------

	// The texture matrix is only read by engine programs that ask for it
	// (none do); the fixed-function shaders do not use one.
	void cLowLevelGraphicsSDL::LoadMatrix(eMatrix aMtxType)
	{
		if(aMtxType != eMatrix_Texture) mbMatricesChanged = true;
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::PrepareDraw()
	{
		GLuint lVertex = cGLSLProgram::GetVertexStage();
		GLuint lFragment = cGLSLProgram::GetFragmentStage();

		if(lVertex==0 && lFragment!=0)
		{
			static bool bWarned = false;
			if(!bWarned) Warning("A fragment program is drawn without a vertex program\n");
			bWarned = true;
		}

		if(lVertex==0)
		{
			lVertex = mFixedFunction.GetVertexStage(mvMatrixStack[eMatrix_ModelView].back(),
													mvMatrixStack[eMatrix_Projection].back(), mbMatricesChanged);
			mbMatricesChanged = false;
		}
		if(lFragment==0) lFragment = mFixedFunction.GetFragmentStage();

		cGLSLProgram::SetPipelineStages(lVertex, lFragment);

		if(mlCurrentAttribChanged)
		{
			for(int i=0; i<cGLSLProgram::eVertexAttrib_LastEnum; ++i)
				if(mlCurrentAttribChanged & (1u << i)) glVertexAttrib4fv(i, mvCurrentAttrib[i]);
			mlCurrentAttribChanged = 0;
		}

		if(mbStateChanged)
		{
			// Colours are converted for the scene when it holds linear
			// values, unless what is drawn is linear already. "Modulate x2"
			// (destination x source x 2) doubles on encoded values: on
			// linear ones that is 2^2.2, of which the blend supplies 2. What
			// is added to the scene gets the glow value (the light passes
			// are added too, but do their own conversion).
			bool bConvert = mbLinearLighting && mbInScene && !mbLinearSource;
			bool bMulX2 = mbBlend && mBlendSrc == eBlendFunc_DestColor && mBlendDest == eBlendFunc_SrcColor;
			bool bAdd = mbBlend && mBlendDest == eBlendFunc_One &&
						(mBlendSrc == eBlendFunc_One || mBlendSrc == eBlendFunc_SrcAlpha);
			float fScale = bMulX2 ? 2.2974f : 1.0f;
			float fGlow = bConvert && bAdd ? mLinearTuning.mfGlow : 1.0f;
			bool bAlpha = mbBlend && mBlendSrc == eBlendFunc_SrcAlpha && mBlendDest == eBlendFunc_OneMinusSrcAlpha;
			float fAlphaMatch = bConvert && bAlpha ? cMath::Clamp(mLinearTuning.mfBlendMatch, 0.0f, 1.0f) : 0.0f;
			// In the multisampled scene a test for "at least" decides for
			// each sample: the fragment's alpha becomes the share of the
			// pixel's samples it covers (kAlphaToCoverage in the shaders)
			bool bToCoverage =	mbAlphaTest && mlMultisampling > 0 && mbInScene && mbSceneResolved==false &&
								mpRenderTarget==nullptr && mbInShadowMap==false &&
								(mAlphaTestFunc == eAlphaTestFunc_GreaterOrEqual || mAlphaTestFunc == eAlphaTestFunc_Greater);
			if(bToCoverage != mbAlphaToCoverage)
			{
				if(bToCoverage)	glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
				else			glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);
				mbAlphaToCoverage = bToCoverage;
			}
			float fAlphaFunc = bToCoverage ? (float)cGLSLProgram::kAlphaToCoverage : (float)mAlphaTestFunc + 1;
			const cShadowMapParams &shadow = mShadowParams;
			// The colour is scaled as a screen value: brightness is an
			// amount of light. Exactly 1 for a light as the game made it.
			const cLightLook &look = mLightLook;
			float fLookScale = look.mfBrightness == 1.0f ? 1.0f : powf(cMath::Max(look.mfBrightness, 0.0f), 1.0f / 2.2f);
			float vState[kStateUniformFloats] = {
				mbAlphaTest ? fAlphaFunc : 0.0f, mfAlphaTestRef, fAlphaMatch, fGlow,
				bConvert ? 1.0f : 0.0f, fScale, mLinearTuning.mfLightExp,
				mbLinearLighting && mbInScene && mbLinearSource ? 1.0f : 0.0f,
				(float)shadow.mlFilter, shadow.mfNear, shadow.mfFar, shadow.mfTexelSize,
				shadow.mfSoftness, shadow.mfLightSize, shadow.mfMaxBlur, shadow.mbShowOnly ? 1.0f : 0.0f,
				shadow.mfBias, shadow.mfSlopeBias, (float)shadow.mlSamples, (float)shadow.mlBlockerSamples,
				shadow.mvUvScale.x, shadow.mvUvScale.y, shadow.mbNoise ? 1.0f : 0.0f, 0.0f,
				look.mbDistanceFalloff ? 1.0f : 0.0f, look.mfEvenAt, look.mfNearBoost, look.HasTube() ? 1.0f : 0.0f,
				look.mbBeam ? 1.0f : 0.0f, look.mfBeamHotspot, look.mfBeamField, look.mfBeamWidth,
				look.mTint.r * fLookScale, look.mTint.g * fLookScale, look.mTint.b * fLookScale, 0.0f};
			glBindBuffer(GL_UNIFORM_BUFFER, mlStateUniformBuffer);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(vState), vState);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
			mbStateChanged = false;
		}
	}

	//-----------------------------------------------------------------------

}
