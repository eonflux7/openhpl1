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
#ifndef HPL_GRAPHICS_H
#define HPL_GRAPHICS_H

#include "system/SystemTypes.h"

namespace hpl {

	class cResources;
	class cRenderer2D;
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

		cRenderer2D* GetRenderer2D();
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

		/**
		 * Linear lighting: lights, transparent surfaces and effects add up
		 * and blend on values proportional to light instead of on screen
		 * values. Off is the original look.
		 */
		void SetLinearLighting(bool abX);
		bool GetLinearLighting();

	private:
		iLowLevelGraphics *mpLowLevelGraphics;
		iLowLevelResources *mpLowLevelResources;
		cGraphicsDrawer *mpDrawer;
		cMeshCreator *mpMeshCreator;
		cMaterialHandler *mpMaterialHandler;
		cRenderer2D* mpRenderer2D;
		cRenderer3D* mpRenderer3D;
		cRendererPostEffects* mpRendererPostEffects;
		cRenderList *mpRenderList;
	};

};
#endif // HPL_GRAPHICS_H
