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
#ifndef HPL_RENDER_LIST_H
#define HPL_RENDER_LIST_H

#include <set>
#include <array>
#include <cstdint>
#include <vector>
#include "graphics/RenderState.h"

#include "math/MathTypes.h"
#include "graphics/GraphicsTypes.h"
#include "system/SystemTypes.h"
#include "scene/Light3D.h"
#include "graphics/Material.h"
#include "graphics/VertexBuffer.h"

namespace hpl {

	class iRenderable;
	class cCamera3D;
	class cGraphics;

	class iRenderState;
	class cRenderSettings;


	//-------------------------------------------------------------

	enum eRenderListDrawType
	{
		eRenderListDrawType_Normal,
		eRenderListDrawType_Trans,
		eRenderListDrawType_LastEnum
	};

	//-------------------------------------------------------------

	// One complete state and lexicographic key per draw. Queues retain their
	// storage between frames; no state nodes or per-level allocations.
	struct cRenderDraw
	{
		iRenderState mState{};
		std::array<uintptr_t, 12 + MAX_TEXTUREUNITS> mKey{};
		eRenderStateType FirstChangedState(const cRenderDraw* apPrevious) const;
	};

	class cRenderQueue
	{
	public:
		std::vector<cRenderDraw> mvDraws;
		bool mbSector = false;
		void Clear(){ mvDraws.clear(); }
		void Sort();
		void Render(cRenderSettings* apSettings);
	};

	class cFogObject_Compare
	{
	public:
		bool operator()(iRenderable* pObjectA,iRenderable* pObjectB) const;
	};

	typedef std::multiset<iRenderable*,cFogObject_Compare> tFogObjectSet;
	typedef tFogObjectSet::iterator tFogObjectSetIt;

	typedef cSTLIterator<iRenderable*, tFogObjectSet,
							tFogObjectSetIt> cFogObjectIterator;

	//-------------------------------------------------------------


	class cTransperantObject_Compare
	{
	public:
		bool operator()(iRenderable* pObjectA,iRenderable* pObjectB) const;
	};

	typedef std::multiset<iRenderable*,cTransperantObject_Compare> tTransperantObjectSet;
	typedef tTransperantObjectSet::iterator tTransperantObjectSetIt;

	typedef cSTLIterator<iRenderable*, tTransperantObjectSet, tTransperantObjectSetIt> cTransperantObjectIterator;


	//-------------------------------------------------------------

	typedef std::set<iRenderable*> tRenderableSet;
	typedef tRenderableSet::iterator tRenderableSetIt;

	typedef std::set<iLight3D*> tLight3DSet;
	typedef tLight3DSet::iterator tLight3DSetIt;


	typedef cSTLIterator<iLight3D*,tLight3DSet, tLight3DSetIt> cLight3DIterator;
	typedef cSTLIterator<iRenderable*,tRenderableSet, tRenderableSetIt> cRenderableIterator;

	//-------------------------------------------------------------

	class cRenderList
	{
	public:
		cRenderList(cGraphics *apGraphics);
		~cRenderList();

		void SetCamera(cCamera3D* apCamera){ mpCamera = apCamera;}
		cCamera3D* GetCamera(){ return mpCamera;}

		bool Add(iRenderable* apObject);
		void Clear();

		cFogObjectIterator GetFogIterator();

		cTransperantObjectIterator GetTransperantIterator();

		cLight3DIterator GetLightIt();
		cRenderableIterator GetObjectIt();

		int GetLightNum();
		int GetObjectNum();

		void Compile();

		int GetRenderCount(){ return mlRenderCount;}
		int GetLastRenderCount(){ return mlLastRenderCount;}

		int GetLightObjects(int alLightIdx){ return mvObjectsPerLight[alLightIdx];}

		void SetFrameTime(float afTime){ mfFrameTime = afTime;}

		cRenderQueue* GetQueue(eRenderListDrawType aObjectType ,eMaterialRenderType aPassType, int alLightNum);

		static inline int GetGlobalRenderCount(){ return mlGlobalRenderCount;}

	private:

		void AddDraw(iRenderable* apObject,eRenderListDrawType aObjectType,
						eMaterialRenderType mPassType,int alLightNum,iLight3D* apLight,
						bool abUseDepth, int alPass);

		static int mlGlobalRenderCount;

		tLight3DSet m_setLights;
		int mvObjectsPerLight[MAX_NUM_OF_LIGHTS];

		tRenderableSet m_setObjects;

		tFogObjectSet m_setFogObjects;
		tTransperantObjectSet m_setTransperantObjects;

		cRenderQueue mDepthQueue;
		cRenderQueue mDiffuseQueue;
		cRenderQueue mvLightQueues[MAX_NUM_OF_LIGHTS];
		cRenderQueue mTransQueue;


		int mlRenderCount;
		int mlLastRenderCount;

		float mfFrameTime;


		cCamera3D* mpCamera;

		cGraphics *mpGraphics;
	};
};
#endif // HPL_RENDER_LIST_H
