// This file is part of OpenHPL1, licensed under the GNU GPL v3 or later.
#ifndef HPL_SDL_GRAPHICS_WINDOW_H
#define HPL_SDL_GRAPHICS_WINDOW_H
#include <SDL3/SDL.h>
#include <glad/gl.h>
#include "graphics/LowLevelGraphics.h"
#include "system/SystemTypes.h"
#include <vector>
namespace hpl
{
	class cSDLPixelFormat;
	class cGpuProfiler;
	// Owns SDL's window/context, display selection and frame captures. Construction needs no GL;
	// external-context tests can use the other backend components independently.
	class cSDLGraphicsWindow
	{
	public:
		cSDLGraphicsWindow() = default;
		~cSDLGraphicsWindow();
		cSDLGraphicsWindow(const cSDLGraphicsWindow &) = delete;
		cSDLGraphicsWindow &operator=(const cSDLGraphicsWindow &) = delete;
		bool Init(int alWidth, int alHeight, int alBpp, int abFullscreen,
			const tString &asWindowCaption, int alDisplay);
		SDL_Window *GetWindow() const { return mpWindow; }
		const cVector2l &GetSize() const { return mvScreenSize; }
		void SwapBuffers() { SDL_GL_SwapWindow(mpWindow); }
		int GetDisplayNum();
		tString GetDisplayName(int alDisplay);
		cVector2l GetDisplaySize(int alDisplay);
		void GetDisplayModes(int alDisplay, std::vector<cVector2l> &avModes);
		int GetDisplay();
		void ShowCursor(bool abX);
		void SetVsyncActive(bool abX);
		void SaveScreenToBMP(const tString &asFile, cSDLPixelFormat *apPixelFormat, GLuint alOutputFramebuffer);
		void SaveNextFrameToPNG(const tString &asFile) { msSaveNextFrame = asFile; }
		void CaptureNextFrame(iLowLevelGraphics::tFrameCapture aCapture) { mFrameCapture = std::move(aCapture); }
		void CaptureFrame(cGpuProfiler &aProfiler);
		bool SaveFrameToPNG(const tString &asFile);

	private:
		void DebugDumpFrame();
		std::vector<unsigned char> ReadFramePixels();
		iLowLevelGraphics::tFrameCapture mFrameCapture;
		tString msSaveNextFrame;
		SDL_DisplayID ChooseDisplay(int alDisplay);
		SDL_DisplayID GetDisplayID(int alDisplay);
		SDL_Window *mpWindow = nullptr;
		SDL_GLContext mGLContext = nullptr;
		cVector2l mvScreenSize = 0;
		int mlDisplay = 0;
	};
} // namespace hpl
#endif
