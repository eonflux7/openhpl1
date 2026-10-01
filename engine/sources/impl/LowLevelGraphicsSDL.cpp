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
		case eColorDataFormat_ALPHA:	return GL_ALPHA;
		case eColorDataFormat_BGR:		return GL_BGR_EXT;
		case eColorDataFormat_BGRA:		return GL_BGRA_EXT;
		};

		return 0;
	}

	//-------------------------------------------------

	static void GLAPIENTRY GLDebugCallback(GLenum aSource, GLenum aType, GLuint alId, GLenum aSeverity,
											GLsizei alLength, const GLchar *apMessage, const void *apUser)
	{
		// NVIDIA: "vertex shader is being recompiled based on GL state", a
		// note about compatibility-profile state that goes with fixed-function.
		if(alId == 131218) return;

		// The same message tends to repeat every frame; log each one a few times
		// only. Keyed by text, as drivers give one id to many different errors.
		static std::map<std::string,int> mapCount;
		if(++mapCount[std::string(apMessage, alLength >= 0 ? alLength : strlen(apMessage))] > 3) return;
		Log("GL DEBUG [type 0x%x id %u sev 0x%x]: %s\n", aType, alId, aSeverity, apMessage);
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
		case eTextureTarget_Rect:	return GL_TEXTURE_RECTANGLE_NV;
		case eTextureTarget_CubeMap:	return GL_TEXTURE_CUBE_MAP_ARB;
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
		mGammaPass.msName = "gamma_correction";
		mOutputPass.msName = "output";

		mbInScene = false;
		mlSceneFramebuffer = 0;
		mlOutputFramebuffer = 0;
		mlOutputColor = 0;
		mlOutputDepthStencil = 0;
		mlSceneColor = 0;
		mlSceneDepthStencil = 0;
		mlResolveFramebuffer = 0;
		mlResolveColor = 0;
		mlTargetFramebuffer = 0;

		mpPixelFormat = hplNew(cSDLPixelFormat, () );

		mpRenderTarget=nullptr;

		for(int i=0;i<MAX_TEXTUREUNITS;i++)
			mpCurrentTexture[i] = nullptr;

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

		DeleteFullscreenPass(mGammaPass);
		DeleteFullscreenPass(mOutputPass);
		DeleteFramebuffers();

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

		// The renderer still uses the fixed-function pipeline next to GLSL.
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);

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

		mGLContext = SDL_GL_CreateContext(mpWindow);
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
		Log("  OpenGL %d.%d: %s / %s\n", GLAD_VERSION_MAJOR(lGLVersion), GLAD_VERSION_MINOR(lGLVersion),
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

		//Check Multisample properties
		CheckMultisampleCaps();

		//Turn off cursor as default
		ShowCursor(false);

		mvRenderSize = mvScreenSize;
		mfRenderScale = 1.0f;
		if(!CreateOutputFramebuffer() || !CreateSceneFramebuffers())
		{
			FatalError("Couldn't create the framebuffers!\n");
			return false;
		}

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

	void cLowLevelGraphicsSDL::CheckMultisampleCaps()
	{

	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetupGL()
	{
		//Inits GL stuff
		//Set Shade model and clear color.
		glShadeModel(GL_SMOOTH);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

		// Each fragment's colour is clamped to 0..1 before blending, as an
		// 8-bit buffer did; only the sums of blending go past 1 in the float
		// scene buffer.
		glClampColor(GL_CLAMP_FRAGMENT_COLOR, GL_TRUE);

		//Depth Test setup
		glClearDepth(1.0f);//VAlues buffer is cleared with
		glEnable(GL_DEPTH_TEST); //enable depth testing
		glDepthFunc(GL_LEQUAL); //function to do depth test with
		glDisable(GL_ALPHA_TEST);

		//Set best perspective correction
		glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

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
		glMatrixMode(GL_PROJECTION);

		//glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);

		/////  BEGIN BATCH ARRAY STUFF ///////////////

		//Enable all the vertex arrays that are used:
		glEnableClientState(GL_VERTEX_ARRAY ); //The positions
		glEnableClientState(GL_COLOR_ARRAY ); //The color
		glEnableClientState(GL_TEXTURE_COORD_ARRAY); //Tex coords
		glDisableClientState(GL_NORMAL_ARRAY);
		//Disable the once not used.
		glDisableClientState(GL_INDEX_ARRAY); //color index
		glDisableClientState(GL_EDGE_FLAG_ARRAY);

		///// END BATCH ARRAY STUFF ///////////////

		//Show some info
		Log("  Max texture image units: %d\n",GetCaps(eGraphicCaps_MaxTextureImageUnits));
		Log("  Anisotropic filtering: %d\n",GetCaps(eGraphicCaps_AnisotropicFiltering));
		if(GetCaps(eGraphicCaps_AnisotropicFiltering))
			Log("  Max Anisotropic degree: %d\n",GetCaps(eGraphicCaps_MaxAnisotropicFiltering));
	}
	//-----------------------------------------------------------------------

	int cLowLevelGraphicsSDL::GetCaps(eGraphicCaps aType)
	{
		switch(aType)
		{
		//Max Texture Image Units
		case eGraphicCaps_MaxTextureImageUnits:
			{
				int lUnits;
				glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS_ARB,(GLint *)&lUnits);
				return lUnits;
			}

		//Texture Anisotropy
		case eGraphicCaps_AnisotropicFiltering:
			{
				if(GLAD_GL_EXT_texture_filter_anisotropic) return 1;
				else return 0;
			}

		//Texture Anisotropy
		case eGraphicCaps_MaxAnisotropicFiltering:
			{
				if(!GLAD_GL_EXT_texture_filter_anisotropic) return 0;

				float fMax;
				glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT,&fMax);
				return (int)fMax;
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

	void cLowLevelGraphicsSDL::SetMultisamplingActive(bool abX)
	{
		if(mlMultisampling<=0) return;

		if(abX)
			glEnable(GL_MULTISAMPLE_ARB);
		else
			glDisable(GL_MULTISAMPLE_ARB);
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

	void cLowLevelGraphicsSDL::SetClipPlane(int alIdx, const cPlanef& aPlane)
	{
		mvClipPlanes[alIdx] = aPlane;

		double vPlane[4];
		vPlane[0] = aPlane.a;
		vPlane[1] = aPlane.b;
		vPlane[2] = aPlane.c;
		vPlane[3] = aPlane.d;
		glClipPlane(GL_CLIP_PLANE0 + alIdx,vPlane);
	}
	cPlanef cLowLevelGraphicsSDL::GetClipPlane(int alIdx, const cPlanef& aPlane)
	{
		return mvClipPlanes[alIdx];
	}
	void cLowLevelGraphicsSDL::SetClipPlaneActive(int alIdx, bool abX)
	{
		if(abX) glEnable(GL_CLIP_PLANE0 + alIdx);
		else	glDisable(GL_CLIP_PLANE0 + alIdx);
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
			iBitmap2D* pBmp = CreateBitmap2D(avSize,alBpp);
			pBmp->FillRect(cRect2l(0,0,0,0),aFillCol);

			pTex = hplNew( cSDLTexture, ("",mpPixelFormat,this,aType, abUseMipMaps, aTarget) );
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


	void cLowLevelGraphicsSDL::PushMatrix(eMatrix aMtxType)
	{
		std::vector<cMatrixf>& vStack = mvMatrixStack[aMtxType];
		vStack.push_back(vStack.back());
		SetMatrixMode(aMtxType);
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
		GLenum LastTarget=0;
		if(mpCurrentTexture[alUnit])
			LastTarget = GetGLTextureTargetEnum(mpCurrentTexture[alUnit]->GetTarget());

		glActiveTextureARB(GL_TEXTURE0_ARB + alUnit);

		//Disable this unit if NULL
		if(apTex == nullptr)
		{
			glDisable(LastTarget);
			//glBindTexture(LastTarget,0);
		}
		//Enable the unit, set the texture handle and bind the pbuffer
		else
		{
			if(LastTarget != 0 && NewTarget != LastTarget) glDisable(LastTarget);

			cSDLTexture *pSDLTex = static_cast<cSDLTexture*> (apTex);

			glBindTexture(NewTarget, pSDLTex->GetTextureHandle());
			glEnable(NewTarget);
		}

		mpCurrentTexture[alUnit] = apTex;
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::ForgetTexture(iTexture* apTex)
	{
		for(unsigned int lUnit=0; lUnit<MAX_TEXTUREUNITS; lUnit++)
		{
			if(mpCurrentTexture[lUnit] != apTex) continue;

			GLint lActiveUnit = GL_TEXTURE0;
			glGetIntegerv(GL_ACTIVE_TEXTURE,&lActiveUnit);

			glActiveTexture(GL_TEXTURE0 + lUnit);
			glDisable(GetGLTextureTargetEnum(apTex->GetTarget()));
			glActiveTexture((GLenum)lActiveUnit);

			mpCurrentTexture[lUnit] = nullptr;
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
		glEnable(target);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetActiveTextureUnit(unsigned int alUnit)
	{
		glActiveTextureARB(GL_TEXTURE0_ARB + alUnit);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetTextureEnv(eTextureParam aParam, int alVal)
	{
		GLenum lParam = GetGLTextureParamEnum(aParam);

		glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_COMBINE_ARB);

		if(aParam==eTextureParam_ColorFunc || aParam==eTextureParam_AlphaFunc){
			glTexEnvi(GL_TEXTURE_ENV,lParam,GetGLTextureFuncEnum((eTextureFunc)alVal));
		}
		else if(aParam>=eTextureParam_ColorSource0 && aParam<=eTextureParam_AlphaSource2){
			glTexEnvi(GL_TEXTURE_ENV,lParam,GetGLTextureSourceEnum((eTextureSource)alVal));
		}
		else if(aParam>=eTextureParam_ColorOp0 && aParam<=eTextureParam_AlphaOp2){
			glTexEnvi(GL_TEXTURE_ENV,lParam,GetGLTextureOpEnum((eTextureOp)alVal));
		}
		else {
			glTexEnvi(GL_TEXTURE_ENV,lParam,alVal);
		}
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetTextureConstantColor(const cColor &aColor)
	{
		float vColor[4] = {	aColor.r, aColor.g, aColor.b, aColor.a	};

		glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, &vColor[0]);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetColor(const cColor &aColor)
	{
		glColor4f(aColor.r, aColor.g, aColor.b, aColor.a);
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
		glColor4f(1,1,1,1);
		glBegin(GL_QUADS);
		{
			glTexCoord2f(0.0, 0.0); glVertex3f(avPos.x, avPos.y,afZ);
			glTexCoord2f(1.0, 0.0); glVertex3f(avPos.x+avSize.x, avPos.y,afZ);
			glTexCoord2f(1.0, 1.0); glVertex3f(avPos.x+avSize.x, avPos.y+avSize.y,afZ);
			glTexCoord2f(0.0, 1.0); glVertex3f(avPos.x, avPos.y+avSize.y,afZ);
		}
		glEnd();
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::FlushRendering()
	{
		glFlush();
	}
	void cLowLevelGraphicsSDL::SwapBuffers()
	{
		glFlush();

		PresentFrame();

		// The frame dumps read the window's back buffer, as shown.
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		DebugDumpFrame();
		if(msSaveNextFrame != "")
		{
			SaveFrameToPNG(msSaveNextFrame);
			msSaveNextFrame = "";
		}
		SDL_GL_SwapWindow(mpWindow);

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

	// The scene framebuffer (render size, possibly multisampled, with a
	// resolve framebuffer then). Textures rather than renderbuffers, so later
	// passes can read them. They are bound by hand, SetTexture() never sees
	// them. Leaves the output framebuffer bound.
	bool cLowLevelGraphicsSDL::CreateSceneFramebuffers()
	{
		GLint lLastTexture = 0;
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &lLastTexture);

		const int lW = mvRenderSize.x, lH = mvRenderSize.y;
		bool bOK = true;
		// The output pass samples the single-sampled scene colour; scaling
		// to the window is done by the filter.
		GLenum lFilter = mvRenderSize == mvScreenSize ? GL_NEAREST : GL_LINEAR;

		GLenum lTarget = GL_TEXTURE_2D;
		if(mlMultisampling > 0)
		{
			GLint lMaxSamples = 0;
			glGetIntegerv(GL_MAX_SAMPLES, &lMaxSamples);
			if(mlMultisampling > lMaxSamples)
			{
				Warning("%d samples asked for, the most supported is %d\n", mlMultisampling, lMaxSamples);
				mlMultisampling = lMaxSamples;
			}
		}
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
			glGenTextures(1, &mlSceneDepthStencil);
			glBindTexture(GL_TEXTURE_2D, mlSceneDepthStencil);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, lW, lH, 0,
							GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		}

		glGenFramebuffers(1, &mlSceneFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, mlSceneFramebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, lTarget, mlSceneColor, 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, lTarget, mlSceneDepthStencil, 0);
		bOK = CheckFramebuffer("Scene") && bOK;

		if(mlMultisampling > 0)
		{
			mlResolveColor = CreateColorTexture(lW, lH, kSceneColorFormat, lFilter);
			glGenFramebuffers(1, &mlResolveFramebuffer);
			glBindFramebuffer(GL_FRAMEBUFFER, mlResolveFramebuffer);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mlResolveColor, 0);
			bOK = CheckFramebuffer("Resolve") && bOK;
		}

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

		glBindTexture(GL_TEXTURE_2D, lLastTexture);
		BindFramebuffer(mlOutputFramebuffer, mvScreenSize);
		return bOK;
	}

	void cLowLevelGraphicsSDL::DeleteSceneFramebuffers()
	{
		GLuint vFramebuffers[] = {mlSceneFramebuffer, mlResolveFramebuffer};
		GLuint vTextures[] = {mlSceneColor, mlSceneDepthStencil, mlResolveColor};
		glDeleteFramebuffers(2, vFramebuffers);
		glDeleteTextures(3, vTextures);
		mlSceneFramebuffer = mlResolveFramebuffer = 0;
		mlSceneColor = mlSceneDepthStencil = mlResolveColor = 0;
	}

	void cLowLevelGraphicsSDL::DeleteFramebuffers()
	{
		DeleteSceneFramebuffers();
		glDeleteFramebuffers(1, &mlOutputFramebuffer);
		glDeleteFramebuffers(1, &mlTargetFramebuffer);
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

	void cLowLevelGraphicsSDL::BeginScene()
	{
		SetRenderTarget(nullptr);
		mbInScene = true;
		BindFramebuffer(mlSceneFramebuffer, mvRenderSize);
	}

	//-----------------------------------------------------------------------

	// Draws the scene to the output framebuffer through the output pass
	// (resolved, scaled, converted to 8 bits with dithering) and leaves the
	// output framebuffer bound for the 2D drawing.
	void cLowLevelGraphicsSDL::EndScene()
	{
		SetRenderTarget(nullptr);
		mbInScene = false;

		GLboolean bScissor = glIsEnabled(GL_SCISSOR_TEST);
		if(bScissor) glDisable(GL_SCISSOR_TEST);

		GLuint lSource = mlSceneFramebuffer;
		if(mlMultisampling > 0)
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, mlSceneFramebuffer);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mlResolveFramebuffer);
			glBlitFramebuffer(0, 0, mvRenderSize.x, mvRenderSize.y, 0, 0, mvRenderSize.x, mvRenderSize.y,
								GL_COLOR_BUFFER_BIT, GL_NEAREST);
			lSource = mlResolveFramebuffer;
		}

		BindFramebuffer(mlOutputFramebuffer, mvScreenSize);
		if(CreateFullscreenPass(mOutputPass))
		{
			glProgramUniform2f(mOutputPass.mlFragment, glGetUniformLocation(mOutputPass.mlFragment, "invOutputSize"),
								1.0f / mvScreenSize.x, 1.0f / mvScreenSize.y);
			DrawFullscreenPass(mOutputPass, mlMultisampling > 0 ? mlResolveColor : mlSceneColor, mvScreenSize);
		}
		else
		{
			// Without dithering
			bool bScaled = mvRenderSize != mvScreenSize;
			glBindFramebuffer(GL_READ_FRAMEBUFFER, lSource);
			glBlitFramebuffer(0, 0, mvRenderSize.x, mvRenderSize.y, 0, 0, mvScreenSize.x, mvScreenSize.y,
								GL_COLOR_BUFFER_BIT, bScaled ? GL_LINEAR : GL_NEAREST);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, mlOutputFramebuffer);
		}

		if(bScissor) glEnable(GL_SCISSOR_TEST);
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
		aPass.mlFragment = cGLSLProgram::CreateStage(GL_FRAGMENT_SHADER, aPass.msName);
		if(aPass.mlVertex==0 || aPass.mlFragment==0)
		{
			Error("Couldn't create the '%s' pass!\n", aPass.msName.c_str());
			DeleteFullscreenPass(aPass);
			aPass.mbFailed = true;
			return false;
		}

		glGenProgramPipelines(1,&aPass.mlPipeline);
		glUseProgramStages(aPass.mlPipeline, GL_VERTEX_SHADER_BIT, aPass.mlVertex);
		glUseProgramStages(aPass.mlPipeline, GL_FRAGMENT_SHADER_BIT, aPass.mlFragment);

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
	// shaders and alTexture (2D) on unit 0, into the bound framebuffer.
	// Uniforms are set by the caller. All state is put back; the renderer
	// does not expect it to change.
	void cLowLevelGraphicsSDL::DrawFullscreenPass(cFullscreenPass &aPass, GLuint alTexture, const cVector2l &avSize)
	{
		GLint lLastUnit = 0, lLastTexture = 0, lLastPipeline = 0;
		glGetIntegerv(GL_ACTIVE_TEXTURE, &lLastUnit);
		glGetIntegerv(GL_PROGRAM_PIPELINE_BINDING, &lLastPipeline);
		glActiveTexture(GL_TEXTURE0);
		glGetIntegerv(GL_TEXTURE_BINDING_2D, &lLastTexture);

		glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT |
						GL_POLYGON_BIT | GL_VIEWPORT_BIT | GL_SCISSOR_BIT | GL_TRANSFORM_BIT);

		glBindTexture(GL_TEXTURE_2D, alTexture);

		glDisable(GL_DEPTH_TEST);
		glDisable(GL_STENCIL_TEST);
		glDisable(GL_ALPHA_TEST);
		glDisable(GL_BLEND);
		glDisable(GL_SCISSOR_TEST);
		glDisable(GL_CULL_FACE);
		for(int i=0; i<kMaxClipPlanes; ++i) glDisable(GL_CLIP_PLANE0 + i);
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		glDepthMask(GL_FALSE);
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glViewport(0, 0, avSize.x, avSize.y);

		glBindProgramPipeline(aPass.mlPipeline);

		glBegin(GL_TRIANGLES);
		glVertex2f(-1.0f, -1.0f);
		glVertex2f( 3.0f, -1.0f);
		glVertex2f(-1.0f,  3.0f);
		glEnd();

		glBindProgramPipeline(lLastPipeline);
		glPopAttrib();
		glBindTexture(GL_TEXTURE_2D, lLastTexture);
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

		glBegin(GL_TRIANGLES);
		{
			for(int i=0;i<3;i++){
				glTexCoord3f(avVtx[i].tex.x,avVtx[i].tex.y,avVtx[i].tex.z);
				glColor4f(avVtx[i].col.r,avVtx[i].col.g,avVtx[i].col.b,avVtx[i].col.a);
				glVertex3f(avVtx[i].pos.x,avVtx[i].pos.y,avVtx[i].pos.z);
			}
		}
		glEnd();
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DrawTri(const cVertex* avVtx)
	{
		glBegin(GL_TRIANGLES);
		{
			for(int i=0;i<3;i++){
				glTexCoord3f(avVtx[i].tex.x,avVtx[i].tex.y,avVtx[i].tex.z);
				glColor4f(avVtx[i].col.r,avVtx[i].col.g,avVtx[i].col.b,avVtx[i].col.a);
				glVertex3f(avVtx[i].pos.x,avVtx[i].pos.y,avVtx[i].pos.z);
			}
		}
		glEnd();
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DrawQuad(const tVertexVec &avVtx)
	{
		assert(avVtx.size()==4);

		glBegin(GL_QUADS);
		{
			for(int i=0;i<4;i++){
				glTexCoord3f(avVtx[i].tex.x,avVtx[i].tex.y,avVtx[i].tex.z);
				glColor4f(avVtx[i].col.r,avVtx[i].col.g,avVtx[i].col.b,avVtx[i].col.a);
				glVertex3f(avVtx[i].pos.x,avVtx[i].pos.y,avVtx[i].pos.z);
			}
		}
		glEnd();
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DrawQuadMultiTex(const tVertexVec &avVtx,const tVector3fVec &avExtraUvs)
	{
		int lExtraUnits = (int)avExtraUvs.size()/4;
		glBegin(GL_QUADS);
		{
			for(int i=0;i<4;i++)
			{
				glMultiTexCoord3fARB(GL_TEXTURE0_ARB,avVtx[i].tex.x,avVtx[i].tex.y,avVtx[i].tex.z);

				for(int unit=0; unit<lExtraUnits; ++unit)
				{
					glMultiTexCoord3fARB(GL_TEXTURE0_ARB + unit + 1,
										avExtraUvs[unit*4 + i].x, avExtraUvs[unit*4 + i].y, avExtraUvs[unit*4 + i].z);
				}

				glColor4f(avVtx[i].col.r,avVtx[i].col.g,avVtx[i].col.b,avVtx[i].col.a);
				glVertex3f(avVtx[i].pos.x,avVtx[i].pos.y,avVtx[i].pos.z);
			}
		}
		glEnd();

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
		if(abX) glEnable(GL_ALPHA_TEST);
		else glDisable(GL_ALPHA_TEST);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetAlphaTestFunc(eAlphaTestFunc aFunc,float afRef)
	{
		glAlphaFunc(GetGLAlphaTestFuncEnum(aFunc),afRef);
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
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetBlendFunc(eBlendFunc aSrcFactor, eBlendFunc aDestFactor)
	{
		glBlendFunc(GetGLBlendEnum(aSrcFactor),GetGLBlendEnum(aDestFactor));
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

		glBegin(GL_QUADS);
		{
			//Make all this inline??
			for(int i=0;i<4;i++){
				glTexCoord3f(avVtx[i].tex.x,avVtx[i].tex.y,avVtx[i].tex.z);
				glColor4f(aCol.r,aCol.g,aCol.b,aCol.a);
				glVertex3f(avVtx[i].pos.x,avVtx[i].pos.y,avVtx[i].pos.z);
			}
		}
		glEnd();
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DrawQuad(const tVertexVec &avVtx,const float afZ)
	{
		assert(avVtx.size()==4);

		glBegin(GL_QUADS);
		{
			//Make all this inline??
			for(int i=0;i<4;i++){
				glTexCoord3f(avVtx[i].tex.x,avVtx[i].tex.y,afZ);
				glColor4f(avVtx[i].col.r,avVtx[i].col.g,avVtx[i].col.b,avVtx[i].col.a);
				glVertex3f(avVtx[i].pos.x,avVtx[i].pos.y,avVtx[i].pos.z);
			}
		}
		glEnd();
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DrawQuad(const tVertexVec &avVtx,const float afZ,const cColor &aCol)
	{
		assert(avVtx.size()==4);

		glBegin(GL_QUADS);
		{
			//Make all this inline??
			for(int i=0;i<4;i++){
				glTexCoord3f(avVtx[i].tex.x,avVtx[i].tex.y,afZ);
				glColor4f(aCol.r,aCol.g,aCol.b,aCol.a);
				glVertex3f(avVtx[i].pos.x,avVtx[i].pos.y,avVtx[i].pos.z);
			}
		}
		glEnd();
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

	void cLowLevelGraphicsSDL::SetBatchTextureUnitActive(unsigned int alUnit,bool abActive)
	{
		glClientActiveTextureARB(GL_TEXTURE0_ARB+alUnit);

		if(abActive==false){
			glTexCoordPointer(3,GL_FLOAT,sizeof(float)*mlBatchStride, &mpVertexArray[7]);
		}
		else {
			glTexCoordPointer(3,GL_FLOAT,0, &mpTexCoordArray[alUnit][0]);
		}
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::FlushTriBatch(tVtxBatchFlag aTypeFlags, bool abAutoClear)
	{
		SetVtxBatchStates(aTypeFlags);
		SetUpBatchArrays();

		glDrawElements(GL_TRIANGLES,mlIndexCount,GL_UNSIGNED_INT, mpIndexArray);

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
		SetVtxBatchStates(aTypeFlags);
		SetUpBatchArrays();

		glDrawElements(GL_QUADS,mlIndexCount,GL_UNSIGNED_INT, mpIndexArray);

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
		glColor4f(aCol.r,aCol.g,aCol.b,aCol.a);
		glBegin(GL_LINES);
		{
			glVertex3f(avBegin.x,avBegin.y,avBegin.z);
			glVertex3f(avEnd.x,avEnd.y,avEnd.z);
		}
		glEnd();
	}

	void cLowLevelGraphicsSDL::DrawBoxMaxMin(const cVector3f& avMax, const cVector3f& avMin, cColor aCol)
	{
		SetTexture(0,nullptr);
		SetBlendActive(false);
		glColor4f(aCol.r,aCol.g,aCol.b,aCol.a);

		glBegin(GL_LINES);
		{
			//Pos Z Quad
			glVertex3f(avMax.x,avMax.y,avMax.z);
			glVertex3f(avMin.x,avMax.y,avMax.z);

			glVertex3f(avMax.x,avMax.y,avMax.z);
			glVertex3f(avMax.x,avMin.y,avMax.z);

			glVertex3f(avMin.x,avMax.y,avMax.z);
			glVertex3f(avMin.x,avMin.y,avMax.z);

			glVertex3f(avMin.x,avMin.y,avMax.z);
			glVertex3f(avMax.x,avMin.y,avMax.z);

			//Neg Z Quad
			glVertex3f(avMax.x,avMax.y,avMin.z);
			glVertex3f(avMin.x,avMax.y,avMin.z);

			glVertex3f(avMax.x,avMax.y,avMin.z);
			glVertex3f(avMax.x,avMin.y,avMin.z);

			glVertex3f(avMin.x,avMax.y,avMin.z);
			glVertex3f(avMin.x,avMin.y,avMin.z);

			glVertex3f(avMin.x,avMin.y,avMin.z);
			glVertex3f(avMax.x,avMin.y,avMin.z);

			//Lines between
			glVertex3f(avMax.x,avMax.y,avMax.z);
			glVertex3f(avMax.x,avMax.y,avMin.z);

			glVertex3f(avMin.x,avMax.y,avMax.z);
			glVertex3f(avMin.x,avMax.y,avMin.z);

			glVertex3f(avMin.x,avMin.y,avMax.z);
			glVertex3f(avMin.x,avMin.y,avMin.z);

			glVertex3f(avMax.x,avMin.y,avMax.z);
			glVertex3f(avMax.x,avMin.y,avMin.z);
		}
		glEnd();

	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DrawSphere(const cVector3f& avPos, float afRadius, cColor aCol)
	{
		int alSegments = 32;
		float afAngleStep = k2Pif /(float)alSegments;

		SetTexture(0,nullptr);
		SetBlendActive(false);
		glColor4f(aCol.r,aCol.g,aCol.b,aCol.a);
		glBegin(GL_LINES);
		{
			//X Circle:
			for(float a=0; a< k2Pif; a+= afAngleStep)
			{
				glVertex3f(avPos.x, avPos.y + sin(a)*afRadius,
									avPos.z + cos(a)*afRadius);

				glVertex3f(avPos.x, avPos.y + sin(a+afAngleStep)*afRadius,
									avPos.z + cos(a+afAngleStep)*afRadius);
			}

			//Y Circle:
			for(float a=0; a< k2Pif; a+= afAngleStep)
			{
				glVertex3f(avPos.x + cos(a)*afRadius, avPos.y,
									avPos.z + sin(a)*afRadius);

				glVertex3f(avPos.x + cos(a+afAngleStep)*afRadius, avPos.y ,
							avPos.z+ sin(a+afAngleStep)*afRadius);
			}

			//Z Circle:
			for(float a=0; a< k2Pif; a+= afAngleStep)
			{
				glVertex3f(avPos.x + cos(a)*afRadius, avPos.y + sin(a)*afRadius, avPos.z);

				glVertex3f(avPos.x + cos(a+afAngleStep)*afRadius,
							avPos.y + sin(a+afAngleStep)*afRadius,
							avPos.z);
			}

		}
		glEnd();
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DrawLine2D(const cVector2f& avBegin, const cVector2f& avEnd, float afZ, cColor aCol)
	{
		SetTexture(0,nullptr);
		SetBlendActive(false);
		glColor4f(aCol.r,aCol.g,aCol.b,aCol.a);
		glBegin(GL_LINES);
		{
			glVertex3f(avBegin.x,avBegin.y,afZ);
			glVertex3f(avEnd.x,avEnd.y,afZ);
		}
		glEnd();
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DrawLineRect2D(const cRect2f& aRect, float afZ, cColor aCol)
	{
		SetTexture(0, nullptr);
		SetBlendActive(false);
		glColor4f(aCol.r,aCol.g,aCol.b,aCol.a);
		glBegin(GL_LINE_STRIP);
		{
			glVertex3f(aRect.x,aRect.y,afZ);
			glVertex3f(aRect.x+aRect.w,aRect.y,afZ);
			glVertex3f(aRect.x+aRect.w,aRect.y+aRect.h,afZ);
			glVertex3f(aRect.x,aRect.y+aRect.h,afZ);
			glVertex3f(aRect.x,aRect.y,afZ);
		}
		glEnd();
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::DrawFilledRect2D(const cRect2f& aRect, float afZ, cColor aCol)
	{
		SetTexture(0, nullptr);
		glColor4f(aCol.r,aCol.g,aCol.b,aCol.a);
		glBegin(GL_QUADS);
		{
			glVertex3f(aRect.x,aRect.y,afZ);
			glVertex3f(aRect.x+aRect.w,aRect.y,afZ);
			glVertex3f(aRect.x+aRect.w,aRect.y+aRect.h,afZ);
			glVertex3f(aRect.x,aRect.y+aRect.h,afZ);
		}
		glEnd();
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
		bool bResolve = mlMultisampling > 0 && mbInScene && mpRenderTarget == nullptr;
		if(bResolve)
		{
			GLboolean bScissor = glIsEnabled(GL_SCISSOR_TEST);
			if(bScissor) glDisable(GL_SCISSOR_TEST);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, mlResolveFramebuffer);
			glBlitFramebuffer(avPos.x, lScreenY, avPos.x + avSize.x, lScreenY + avSize.y,
								avPos.x, lScreenY, avPos.x + avSize.x, lScreenY + avSize.y,
								GL_COLOR_BUFFER_BIT, GL_NEAREST);
			glBindFramebuffer(GL_FRAMEBUFFER, mlResolveFramebuffer);
			if(bScissor) glEnable(GL_SCISSOR_TEST);
		}

		glCopyTexSubImage2D(GetGLTextureTargetEnum(apTex->GetTarget()),0,
							avTexOffset.x, lTexY,
							avPos.x, lScreenY, avSize.x, avSize.y);

		if(bResolve) glBindFramebuffer(GL_FRAMEBUFFER, mlSceneFramebuffer);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetRenderTarget(iTexture* pTex)
	{
		if(pTex == mpRenderTarget) return;
		mpRenderTarget = pTex;

		if(pTex == nullptr)
		{
			if(mbInScene)	BindFramebuffer(mlSceneFramebuffer, mvRenderSize);
			else			BindFramebuffer(mlOutputFramebuffer, mvScreenSize);
			return;
		}

		if(mlTargetFramebuffer == 0) glGenFramebuffers(1, &mlTargetFramebuffer);
		glBindFramebuffer(GL_FRAMEBUFFER, mlTargetFramebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GetGLTextureTargetEnum(pTex->GetTarget()),
								static_cast<cSDLTexture*>(pTex)->GetTextureHandle(), 0);
		BindFramebuffer(mlTargetFramebuffer, cVector2l(pTex->GetWidth(), pTex->GetHeight()));
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

	void cLowLevelGraphicsSDL::SetUpBatchArrays()
	{
		//Set the arrays
		glVertexPointer(3,GL_FLOAT, sizeof(float)*mlBatchStride, mpVertexArray);
		glColorPointer(4,GL_FLOAT,sizeof(float)*mlBatchStride, &mpVertexArray[3]);
		glNormalPointer(GL_FLOAT,sizeof(float)*mlBatchStride, &mpVertexArray[10]);

		glClientActiveTextureARB(GL_TEXTURE0_ARB);
		glTexCoordPointer(3,GL_FLOAT,sizeof(float)*mlBatchStride, &mpVertexArray[7]);
		glClientActiveTextureARB(GL_TEXTURE1_ARB);
		glTexCoordPointer(3,GL_FLOAT,sizeof(float)*mlBatchStride, &mpVertexArray[7]);
		glClientActiveTextureARB(GL_TEXTURE2_ARB);
		glTexCoordPointer(3,GL_FLOAT,sizeof(float)*mlBatchStride, &mpVertexArray[7]);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetVtxBatchStates(tVtxBatchFlag aFlags)
	{
		if(aFlags & eVtxBatchFlag_Position)	glEnableClientState(GL_VERTEX_ARRAY );
		else glDisableClientState(GL_VERTEX_ARRAY );

		if(aFlags & eVtxBatchFlag_Color0) glEnableClientState(GL_COLOR_ARRAY );
		else glDisableClientState(GL_COLOR_ARRAY );

		if(aFlags & eVtxBatchFlag_Normal) glEnableClientState(GL_NORMAL_ARRAY );
		else glDisableClientState(GL_NORMAL_ARRAY );


		if(aFlags & eVtxBatchFlag_Texture0){
			glClientActiveTextureARB(GL_TEXTURE0_ARB);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY );
		}
		else {
			glClientActiveTextureARB(GL_TEXTURE0_ARB);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY );
		}

		if(aFlags & eVtxBatchFlag_Texture1){
			glClientActiveTextureARB(GL_TEXTURE1_ARB);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY );
		}
		else {
			glClientActiveTextureARB(GL_TEXTURE1_ARB);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY );
		}

		if(aFlags & eVtxBatchFlag_Texture2){
			glClientActiveTextureARB(GL_TEXTURE2_ARB);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY );
		}
		else {
			glClientActiveTextureARB(GL_TEXTURE2_ARB);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY );
		}


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

	GLenum cLowLevelGraphicsSDL::GetGLTextureParamEnum(eTextureParam aType)
	{
		switch(aType)
		{
		case eTextureParam_ColorFunc:		return GL_COMBINE_RGB_ARB;
		case eTextureParam_AlphaFunc:		return GL_COMBINE_ALPHA_ARB;
		case eTextureParam_ColorSource0:	return GL_SOURCE0_RGB_ARB;
		case eTextureParam_ColorSource1:	return GL_SOURCE1_RGB_ARB;
		case eTextureParam_ColorSource2:	return GL_SOURCE2_RGB_ARB;
		case eTextureParam_AlphaSource0:	return GL_SOURCE0_ALPHA_ARB;
		case eTextureParam_AlphaSource1:	return GL_SOURCE1_ALPHA_ARB;
		case eTextureParam_AlphaSource2:	return GL_SOURCE2_ALPHA_ARB;
		case eTextureParam_ColorOp0:		return GL_OPERAND0_RGB_ARB;
		case eTextureParam_ColorOp1:		return GL_OPERAND1_RGB_ARB;
		case eTextureParam_ColorOp2:		return GL_OPERAND2_RGB_ARB;
		case eTextureParam_AlphaOp0:		return GL_OPERAND0_ALPHA_ARB;
		case eTextureParam_AlphaOp1:		return GL_OPERAND1_ALPHA_ARB;
		case eTextureParam_AlphaOp2:		return GL_OPERAND2_ALPHA_ARB;
		case eTextureParam_ColorScale:		return GL_RGB_SCALE_ARB;
		case eTextureParam_AlphaScale:		return GL_ALPHA_SCALE;
		}
		return 0;
	}

	//-----------------------------------------------------------------------

	GLenum cLowLevelGraphicsSDL::GetGLTextureOpEnum(eTextureOp aType)
	{
		switch(aType)
		{
		case eTextureOp_Color:			return GL_SRC_COLOR;
		case eTextureOp_OneMinusColor:	return GL_ONE_MINUS_SRC_COLOR;
		case eTextureOp_Alpha:			return GL_SRC_ALPHA;
		case eTextureOp_OneMinusAlpha:	return GL_ONE_MINUS_SRC_ALPHA;
	}
		return 0;
	}

	//-----------------------------------------------------------------------

	GLenum cLowLevelGraphicsSDL::GetGLTextureSourceEnum(eTextureSource aType)
	{
		switch(aType)
		{
		case eTextureSource_Texture:	return GL_TEXTURE;
		case eTextureSource_Constant:	return GL_CONSTANT_ARB;
		case eTextureSource_Primary:	return GL_PRIMARY_COLOR_ARB;
		case eTextureSource_Previous:	return GL_PREVIOUS_ARB;
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
				return GL_TEXTURE_RECTANGLE_NV;
			}
		case eTextureTarget_CubeMap:	return GL_TEXTURE_CUBE_MAP_ARB;
		case eTextureTarget_3D:		return GL_TEXTURE_3D;
		}
		return 0;
	}

	//-----------------------------------------------------------------------

	GLenum cLowLevelGraphicsSDL::GetGLTextureFuncEnum(eTextureFunc aType)
	{
		switch(aType)
		{
		case eTextureFunc_Modulate:		return GL_MODULATE;
		case eTextureFunc_Replace:		return GL_REPLACE;
		case eTextureFunc_Add:			return GL_ADD;
		case eTextureFunc_Substract:	return GL_SUBTRACT_ARB;
		case eTextureFunc_AddSigned:	return GL_ADD_SIGNED_ARB;
		case eTextureFunc_Interpolate:	return GL_INTERPOLATE_ARB;
		case eTextureFunc_Dot3RGB:		return GL_DOT3_RGB_ARB;
		case eTextureFunc_Dot3RGBA:		return GL_DOT3_RGBA_ARB;
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

	GLenum cLowLevelGraphicsSDL::GetGLAlphaTestFuncEnum(eAlphaTestFunc aType)
	{
		switch(aType)
		{
		case eAlphaTestFunc_Never:			return GL_NEVER;
		case eAlphaTestFunc_Less:				return GL_LESS;
		case eAlphaTestFunc_LessOrEqual:		return GL_LEQUAL;
		case eAlphaTestFunc_Greater:			return GL_GREATER;
		case eAlphaTestFunc_GreaterOrEqual:	return GL_GEQUAL;
		case eAlphaTestFunc_Equal:			return GL_EQUAL;
		case eAlphaTestFunc_NotEqual:			return GL_NOTEQUAL;
		case eAlphaTestFunc_Always:			return GL_ALWAYS;
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
		case eStencilOp_IncrementWrap:	return GL_INCR_WRAP_EXT;
		case eStencilOp_DecrementWrap:	return GL_DECR_WRAP_EXT;
		}
		return 0;
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::LoadMatrix(eMatrix aMtxType)
	{
		SetMatrixMode(aMtxType);
		// cMatrixf is row-major, GL wants columns
		cMatrixf mtxTranspose = mvMatrixStack[aMtxType].back().GetTranspose();
		glLoadMatrixf(mtxTranspose.v);
	}

	//-----------------------------------------------------------------------

	void cLowLevelGraphicsSDL::SetMatrixMode(eMatrix mType)
	{
		switch(mType)
		{
			case eMatrix_ModelView: glMatrixMode(GL_MODELVIEW);break;
			case eMatrix_Projection: glMatrixMode(GL_PROJECTION); break;
			case eMatrix_Texture: glMatrixMode(GL_TEXTURE); break;
		}
	}

	//-----------------------------------------------------------------------

}
