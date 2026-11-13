// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_GRAPHICS_H
#define HPL_GRAPHICS_H

#include "system/SystemTypes.h"

namespace hpl {

	class cResources;
	class cRenderer3D;
	class cRendererPostEffects;
	class cRenderList;
	class cGraphicsDrawer;
	class iLowLevelResources;
	class iLowLevelGraphics;
	class cMeshCreator;
	class cMaterialHandler;

	class cGraphics
	{
	public:
		cGraphics(iLowLevelGraphics *apLowLevelGraphics,iLowLevelResources *apLowLevelResources);
		~cGraphics();

		bool Init(int alWidth, int alHeight, int alBpp, int abFullscreen, int alMultisampling,
					const tString &asWindowCaption,cResources* apResources, int alDisplay=-1);

		/**
		 * Get low level graphics routines
		 * \return pointer to the low level class
		 */
		iLowLevelGraphics* GetLowLevel();

		/**
		 * Get the drawer
		 * \return
		 */
		cGraphicsDrawer* GetDrawer();

		cRenderer3D* GetRenderer3D(){ return mpRenderer3D;}
		cRendererPostEffects* GetRendererPostEffects(){ return mpRendererPostEffects;}
		cMeshCreator* GetMeshCreator(){return mpMeshCreator;}
		cMaterialHandler* GetMaterialHandler(){return mpMaterialHandler;}

		/**
		 * Size of the 3D scene relative to the window, 0.25 to 2. The scene
		 * is scaled to the window before the HUD is drawn.
		 */
		void SetRenderScale(float afScale);
		float GetRenderScale();
		// Follows a window resized by hand (between frames); true if it was
		bool ApplyWindowSize();

	private:
		iLowLevelGraphics *mpLowLevelGraphics;
		iLowLevelResources *mpLowLevelResources;
		cGraphicsDrawer *mpDrawer;
		cMeshCreator *mpMeshCreator;
		cMaterialHandler *mpMaterialHandler;
		cRenderer3D* mpRenderer3D;
		cRendererPostEffects* mpRendererPostEffects;
		cRenderList *mpRenderList;
	};

};
#endif // HPL_GRAPHICS_H
