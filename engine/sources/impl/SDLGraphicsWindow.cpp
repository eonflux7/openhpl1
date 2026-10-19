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
#include "impl/SDLGraphicsWindow.h"
#include "impl/SDLBitmap2D.h"
#include "impl/ComparisonImage.h"
#include <algorithm>
#include "impl/GpuProfiler.h"
#include "system/LowLevelSystem.h"
#include "system/String.h"
#include <glad/gl.h>
#include <SDL3_image/SDL_image.h>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
namespace hpl
{
	static void GLAPIENTRY GLDebugCallback(GLenum aSource, GLenum aType, GLuint alId, GLenum aSeverity,
		GLsizei alLength, const GLchar *apMessage, const void *apUser)
	{
		// NVIDIA: "vertex shader is being recompiled based on GL state", a
		// performance note the first time a program is drawn with some state.
		if(alId == 131218) return;

		// The same message tends to repeat every frame; log each one a few times
		// only. Keyed by text, as drivers give one id to many different errors.
		static std::map<std::string, int> mapCount;
		if(++mapCount[std::string(apMessage, alLength >= 0 ? alLength : strlen(apMessage))] > 3) return;
		Log("GL DEBUG [type 0x%x id %u sev 0x%x]: %s\n", aType, alId, aSeverity, apMessage);
	}

	//-------------------------------------------------

	cSDLGraphicsWindow::~cSDLGraphicsWindow()
	{
		if(mGLContext) SDL_GL_DestroyContext(mGLContext);
		if(mpWindow) SDL_DestroyWindow(mpWindow);
	}

	bool cSDLGraphicsWindow::Init(int alWidth, int alHeight, int alBpp, int abFullscreen, const tString &asWindowCaption, int alDisplay)
	{
		mvScreenSize.x = alWidth;
		mvScreenSize.y = alHeight;


		//Set some GL Attributes
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

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

		const char *pGLDebug = SDL_getenv("OPENHPL_GL_DEBUG");
		bool bGLDebug = pGLDebug && pGLDebug[0] && pGLDebug[0] != '0';
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

		// Hidden windows support automated captures without a compositor.
		const char *pHidden = SDL_getenv("OPENHPL_HIDDEN");
		const bool bHidden = pHidden && pHidden[0] && pHidden[0] != '0';
		if(bHidden) abFullscreen = false;

		SDL_WindowFlags lFlags = SDL_WINDOW_OPENGL;
		if(abFullscreen) lFlags |= SDL_WINDOW_FULLSCREEN;
		if(bHidden) lFlags |= SDL_WINDOW_HIDDEN;

		Log(" Setting video mode: %d x %d - %d bpp\n", alWidth, alHeight, alBpp);
		SDL_PropertiesID lProps = SDL_CreateProperties();
		SDL_SetStringProperty(lProps, SDL_PROP_WINDOW_CREATE_TITLE_STRING, asWindowCaption.c_str());
		SDL_SetNumberProperty(lProps, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED_DISPLAY(lDisplay));
		SDL_SetNumberProperty(lProps, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED_DISPLAY(lDisplay));
		SDL_SetNumberProperty(lProps, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, alWidth);
		SDL_SetNumberProperty(lProps, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, alHeight);
		SDL_SetNumberProperty(lProps, SDL_PROP_WINDOW_CREATE_FLAGS_NUMBER, lFlags);
		mpWindow = SDL_CreateWindowWithProperties(lProps);
		SDL_DestroyProperties(lProps);
		if(mpWindow == nullptr)
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
		// asked for. GL 3.3 needs the ARB immutable-storage extensions.
		static const int vVersions[][2] = {{4, 6}, {4, 5}, {4, 3}, {4, 2}, {4, 1}, {3, 3}};
		for(const int *pVersion : vVersions)
		{
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, pVersion[0]);
			SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, pVersion[1]);
			mGLContext = SDL_GL_CreateContext(mpWindow);
			if(mGLContext) break;
		}
		if(mGLContext == nullptr)
		{
			FatalError("Unable to create OpenGL context: %s\n", SDL_GetError());
			return false;
		}

		int lPixelW = 0, lPixelH = 0;
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
			(const char *)glGetString(GL_VENDOR), (const char *)glGetString(GL_RENDERER));

		// The generated loader loads these with core 4.2/4.3 only. The ARB
		// extensions use the same names and also work on a 3.3 core context.
		if(!GLAD_GL_VERSION_4_2 && SDL_GL_ExtensionSupported("GL_ARB_texture_storage"))
		{
			glad_glTexStorage1D = (PFNGLTEXSTORAGE1DPROC)SDL_GL_GetProcAddress("glTexStorage1D");
			glad_glTexStorage2D = (PFNGLTEXSTORAGE2DPROC)SDL_GL_GetProcAddress("glTexStorage2D");
			glad_glTexStorage3D = (PFNGLTEXSTORAGE3DPROC)SDL_GL_GetProcAddress("glTexStorage3D");
		}
		if(!GLAD_GL_VERSION_4_3 && SDL_GL_ExtensionSupported("GL_ARB_texture_storage_multisample"))
			glad_glTexStorage2DMultisample = (PFNGLTEXSTORAGE2DMULTISAMPLEPROC)SDL_GL_GetProcAddress("glTexStorage2DMultisample");
		if(!glTexStorage1D || !glTexStorage2D || !glTexStorage3D || !glTexStorage2DMultisample)
		{
			FatalError("Immutable textures require OpenGL 4.3 or OpenGL 3.3 with ARB_texture_storage and ARB_texture_storage_multisample.\n");
			return false;
		}

		if(bGLDebug && GLAD_GL_KHR_debug)
		{
			glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
			glDebugMessageCallback(GLDebugCallback, nullptr);
			// Drop notifications, keep everything else
			glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
			Log("  OpenGL debug output enabled\n");
		}

		return true;
	}

	void cSDLGraphicsWindow::DebugDumpFrame()
	{
		static int lFrame = 0;
		static const char *pFrames = SDL_getenv("OPENHPL_DUMP_FRAMES");
		++lFrame;
		if(pFrames == nullptr || pFrames[0] == 0) return;

		bool bDump = false;
		for(const char *p = pFrames; *p;)
		{
			if(atoi(p) == lFrame)
			{
				bDump = true;
				break;
			}
			while(*p && *p != ',') ++p;
			if(*p == ',') ++p;
		}
		if(!bDump) return;

		const char *pDir = SDL_getenv("OPENHPL_DUMP_DIR");
		tString sFile = tString(pDir ? pDir : ".") + "/frame_" + cString::ToString(lFrame) + ".png";
		SaveFrameToPNG(sFile);
		Log(" Dumped frame %d to %s (GL error: 0x%x)\n", lFrame, sFile.c_str(), glGetError());
	}

	std::vector<unsigned char> cSDLGraphicsWindow::ReadFramePixels()
	{
		if(mvScreenSize.x <= 0 || mvScreenSize.y <= 0) return {};
		GLint lAlignment, lReadBuffer;
		glGetIntegerv(GL_PACK_ALIGNMENT, &lAlignment);
		glGetIntegerv(GL_READ_BUFFER, &lReadBuffer);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadBuffer(GL_BACK);
		const size_t lStride = (size_t)mvScreenSize.x * 3;
		std::vector<unsigned char> vPixels(lStride * mvScreenSize.y);
		glReadPixels(0,0,mvScreenSize.x,mvScreenSize.y,GL_RGB,GL_UNSIGNED_BYTE,vPixels.data());
		glPixelStorei(GL_PACK_ALIGNMENT, lAlignment);
		glReadBuffer(lReadBuffer);
		for(int y=0; y<mvScreenSize.y/2; ++y)
			std::swap_ranges(vPixels.begin() + y*lStride, vPixels.begin() + (y+1)*lStride,
				vPixels.begin() + (mvScreenSize.y-1-y)*lStride);
		return vPixels;
	}

	bool cSDLGraphicsWindow::SaveFrameToPNG(const tString& asFile)
	{
		std::vector<unsigned char> vPixels = ReadFramePixels();
		if(vPixels.empty()) return false;
		SDL_Surface *pSurface = SDL_CreateSurface(mvScreenSize.x, mvScreenSize.y, SDL_PIXELFORMAT_RGB24);
		if(pSurface==nullptr) return false;
		for(int y=0; y<mvScreenSize.y; ++y)
		{
			memcpy((unsigned char*)pSurface->pixels + y*pSurface->pitch,
					&vPixels[(size_t)y * mvScreenSize.x * 3], mvScreenSize.x * 3);
		}

		bool bRet = IMG_SavePNG(pSurface, asFile.c_str());
		SDL_DestroySurface(pSurface);
		if(bRet==false) Error("Couldn't save frame to '%s': %s\n", asFile.c_str(), SDL_GetError());
		return bRet;
	}

	bool SaveComparisonPNG(const tString& asFile, const cVector2l& avSize,
		const std::vector<unsigned char>& avVanilla, const std::vector<unsigned char>& avImproved)
	{
		cComparisonImage image;
		if(!ComposeComparisonImage(image, avSize, avVanilla, avImproved))
			return SDL_SetError("Invalid or mismatched comparison frames");
		SDL_Surface *pSurface = SDL_CreateSurfaceFrom(image.mvSize.x, image.mvSize.y,
			SDL_PIXELFORMAT_RGB24, image.mvPixels.data(), image.mvSize.x*3);
		if(!pSurface) return false;
		const bool bSaved = IMG_SavePNG(pSurface, asFile.c_str());
		SDL_DestroySurface(pSurface);
		return bSaved;
	}

	SDL_DisplayID cSDLGraphicsWindow::ChooseDisplay(int alDisplay)
	{
		int lCount = 0;
		SDL_DisplayID *pDisplays = SDL_GetDisplays(&lCount);
		if(pDisplays == nullptr || lCount == 0)
		{
			SDL_free(pDisplays);
			mlDisplay = 0;
			return SDL_GetPrimaryDisplay();
		}

		int lChosen = -1;

		// OPENHPL_DISPLAY=<index> goes before the setting.
		const char *pIndex = SDL_getenv("OPENHPL_DISPLAY");
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
			for(int i = 0; i < lCount; ++i)
			{
				const SDL_DisplayMode *pMode = SDL_GetDesktopDisplayMode(pDisplays[i]);
				if(pMode == nullptr) continue;
				long lArea = (long)pMode->w * pMode->h;
				if(lArea > lBestArea)
				{
					lBestArea = lArea;
					lChosen = i;
				}
			}
		}
		if(lChosen < 0) lChosen = 0;

		for(int i = 0; i < lCount; ++i)
		{
			const SDL_DisplayMode *pMode = SDL_GetDesktopDisplayMode(pDisplays[i]);
			Log(" Display %d: %s %dx%d%s\n", i, SDL_GetDisplayName(pDisplays[i]),
				pMode ? pMode->w : 0, pMode ? pMode->h : 0, i == lChosen ? " (used)" : "");
		}

		mlDisplay = lChosen;
		SDL_DisplayID lID = pDisplays[lChosen];
		SDL_free(pDisplays);
		return lID;
	}

	SDL_DisplayID cSDLGraphicsWindow::GetDisplayID(int alDisplay)
	{
		int lCount = 0;
		SDL_DisplayID *pDisplays = SDL_GetDisplays(&lCount);
		SDL_DisplayID lID = 0;
		if(pDisplays && alDisplay >= 0 && alDisplay < lCount) lID = pDisplays[alDisplay];
		SDL_free(pDisplays);
		return lID;
	}

	int cSDLGraphicsWindow::GetDisplayNum()
	{
		int lCount = 0;
		SDL_free(SDL_GetDisplays(&lCount));
		return lCount;
	}

	tString cSDLGraphicsWindow::GetDisplayName(int alDisplay)
	{
		const char *pName = SDL_GetDisplayName(GetDisplayID(alDisplay));
		return pName ? pName : "";
	}

	cVector2l cSDLGraphicsWindow::GetDisplaySize(int alDisplay)
	{
		const SDL_DisplayMode *pMode = SDL_GetDesktopDisplayMode(GetDisplayID(alDisplay));
		if(pMode == nullptr) return cVector2l(0, 0);
		return cVector2l(pMode->w, pMode->h);
	}

	void cSDLGraphicsWindow::GetDisplayModes(int alDisplay, std::vector<cVector2l> &avModes)
	{
		avModes.clear();

		int lCount = 0;
		SDL_DisplayMode **pModes = SDL_GetFullscreenDisplayModes(GetDisplayID(alDisplay), &lCount);
		if(pModes == nullptr) return;

		// One entry per size; the list has every refresh rate and pixel format.
		std::map<long, cVector2l> mapSizes;
		for(int i = 0; i < lCount; ++i)
			mapSizes[(long)pModes[i]->w * 100000 + pModes[i]->h] = cVector2l(pModes[i]->w, pModes[i]->h);
		SDL_free(pModes);

		for(std::map<long, cVector2l>::iterator it = mapSizes.begin(); it != mapSizes.end(); ++it)
			avModes.push_back(it->second);
	}

	int cSDLGraphicsWindow::GetDisplay()
	{
		return mlDisplay;
	}

	void cSDLGraphicsWindow::ShowCursor(bool abX)
	{
		if(abX)
			SDL_ShowCursor();
		else
			SDL_HideCursor();
	}

	void cSDLGraphicsWindow::SetVsyncActive(bool abX)
	{
		SDL_GL_SetSwapInterval(abX ? 1 : 0);
	}

	void cSDLGraphicsWindow::SaveScreenToBMP(const tString &asFile, cSDLPixelFormat *mpPixelFormat, GLuint alOutputFramebuffer)
	{
		glFinish();

		// Called between frames: the output framebuffer still holds the
		// last one (without the gamma pass).
		glBindFramebuffer(GL_READ_FRAMEBUFFER, alOutputFramebuffer);

		cSDLBitmap2D *pBmp = hplNew(cSDLBitmap2D, (mpPixelFormat));
		pBmp->Create(cVector2l(mvScreenSize.x, mvScreenSize.y), 32);

		unsigned char *pDestPixels = (unsigned char *)pBmp->GetSurface()->pixels;
		unsigned char *pSrcPixels = (unsigned char *)hplMalloc(mvScreenSize.x * mvScreenSize.y * 4);

		SDL_LockSurface(pBmp->GetSurface());
		glReadPixels(0, 0, mvScreenSize.x, mvScreenSize.y, GL_RGBA, GL_UNSIGNED_BYTE, pSrcPixels);

		for(int y = 0; y < mvScreenSize.y; ++y)
		{
			for(int x = 0; x < mvScreenSize.x; ++x)
			{
				unsigned char *pDestPix = &pDestPixels[((mvScreenSize.x * y) + x) * 4];
				unsigned char *pSrcPix = &pSrcPixels[((mvScreenSize.x * ((mvScreenSize.y - 1) - y)) + x) * 4];

				pDestPix[0] = pSrcPix[0];
				pDestPix[1] = pSrcPix[1];
				pDestPix[2] = pSrcPix[2];
				pDestPix[3] = 255;
			}
		}

		glBindFramebuffer(GL_READ_FRAMEBUFFER, alOutputFramebuffer);

		SDL_UnlockSurface(pBmp->GetSurface());
		SDL_SaveBMP(pBmp->GetSurface(), asFile.c_str());

		hplFree(pSrcPixels);
		hplDelete(pBmp);
	}

	void cSDLGraphicsWindow::CaptureFrame(cGpuProfiler &aProfiler)
	{
		// Read the window's back buffer after gamma and the overlay.
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		DebugDumpFrame();
		if(mFrameCapture)
		{
			auto capture = std::move(mFrameCapture);
			mFrameCapture = {};
			capture(ReadFramePixels(), mvScreenSize);
		}
		if(msSaveNextFrame != "")
		{
			aProfiler.Mark(msSaveNextFrame);
			SaveFrameToPNG(msSaveNextFrame);
			msSaveNextFrame = "";
		}
	}
} // namespace hpl
