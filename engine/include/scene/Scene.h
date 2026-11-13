// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_SCENE_H
#define HPL_SCENE_H

#include <list>

#include "system/SystemTypes.h"
#include "game/GameTypes.h"

#include "game/Updateable.h"
#include "scene/Camera3D.h"

#include "resources/MeshLoader.h"

namespace hpl {

	class cAI;
	class cGraphics;
	class cResources;
	class cSystem;
	class cSound;
	class cPhysics;
	class iCamera;
	class cUpdater;
	class cWorld3D;

	typedef std::list<iCamera*> tCameraList;
	typedef tCameraList::iterator tCameraListIt;

	typedef std::list<cWorld3D*> tWorld3DList;
	typedef tWorld3DList::iterator tWorld3DListIt;

	class cScene : public iUpdateable
	{
	public:
		cScene(cGraphics *apGraphics,cResources *apResources, cSound* apSound, cPhysics *apPhysics,
				cSystem *apSystem, cAI *apAI);
		~cScene() override;

		void Reset() override;

		/**
		* Called by cGame
		*/
		void UpdateRenderList(float afFrameTime);
		/**
		 * Called by cGame
		 */
		void Render(cUpdater* apUpdater, float afFrameTime);

		void Update(float afTimeStep) override;

		void ClearLoadedMaps(){m_setLoadedMaps.clear();}
		tStringSet* GetLoadedMapsSet(){ return &m_setLoadedMaps;}

		void SetDrawScene(bool abX);
		bool GetDrawScene(){ return mbDrawScene;}

		// SCRIPT VAR METHODS

		cScriptVar* CreateLocalVar(const tString& asName);
		cScriptVar* GetLocalVar(const tString& asName);
		tScriptVarMap* GetLocalVarMap();
		cScriptVar* CreateGlobalVar(const tString& asName);
		cScriptVar* GetGlobalVar(const tString& asName);
		tScriptVarMap* GetGlobalVarMap();

		// CAMERA METHODS

		cCamera3D* CreateCamera3D(eCameraMoveMode aMoveMode);

		void DestroyCamera(iCamera* apCam);

		/**
		 * This sets the current camera.
		 * \param pCam
		 */
		void SetCamera(iCamera* pCam);
		iCamera* GetCamera(){ return mpActiveCamera; }

		void SetCameraIsListener(bool abX){ mbCameraIsListener = abX; }

		/**
		 * Render interpolation of the 3D cameras and of what the physics
		 * of the current world moves, see cGame::Run().
		 */
		void StoreInterpolation();
		void ApplyInterpolation(float afT);
		void RestoreInterpolation();

		// WORLD METHODS

		cWorld3D* LoadWorld3D(const tString& asFile, bool abLoadScript, tWorldLoadFlag aFlags);
		cWorld3D* CreateWorld3D(const tString& asName);
		void DestroyWorld3D(cWorld3D* apWorld);

		void SetWorld3D(cWorld3D* apWorld);
		cWorld3D* GetWorld3D(){ return mpCurrentWorld3D;}

		bool HasLoadedWorld(const tString &asFile);

		void SetUpdateMap(bool abX){ mbUpdateMap = abX;}
		bool GetUpdateMap(){return mbUpdateMap;}

		cSystem* GetSystem(){ return mpSystem;}

	private:
		cGraphics *mpGraphics;
		cResources *mpResources;
		cSound *mpSound;
		cPhysics *mpPhysics;
		cSystem *mpSystem;
		cAI *mpAI;

		bool mbDrawScene;
		bool mbUpdateMap;

		tWorld3DList mlstWorld3D;
		cWorld3D* mpCurrentWorld3D;

		tCameraList mlstCamera;
		iCamera *mpActiveCamera;
		bool mbCameraIsListener;

		tScriptVarMap m_mapLocalVars;
		tScriptVarMap m_mapGlobalVars;

		tStringSet m_setLoadedMaps;
	};

};
#endif // HPL_SCENE_H
