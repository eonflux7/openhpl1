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
	class iGpuProgram;
	class iMaterialProgramSetup;
	class iTexture;

	class iRenderState;
	class cRenderSettings;


	// One complete state and lexicographic key per draw. Queues retain their
	// storage between frames; no state nodes or per-level allocations.
	struct cRenderDraw
	{
		iRenderState mState{};
		std::array<uintptr_t, 11 + MAX_TEXTUREUNITS> mKey{};
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

	// One visible light, its compiled draws and how many objects it reached.
	// The list grows to the frame's light count and keeps queue storage.
	struct cRenderLight
	{
		iLight3D* mpLight = nullptr;
		cRenderQueue mQueue;
		int mlObjectCount = 0;
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

	// One see-through draw, in the order it is executed. The material state
	// is captured when the object enters the list; object callbacks are
	// still asked at execution time, as before. Matrices are copied so a
	// retained record never points at a scratch matrix.
	struct cTransparentDraw
	{
		// Matrix pointers refer to this retained record, or a custom draw's
		// live overrides. Apply the returned state before the next callback.
		iRenderState GetState();

		iRenderable *mpObject = nullptr;
		float mfZ = 0;
		bool mbDepthTest = true;

		eMaterialAlphaMode mAlphaMode = eMaterialAlphaMode_Solid;
		eMaterialBlendMode mBlendMode = eMaterialBlendMode_None;
		iGpuProgram *mpProgram = nullptr;
		iMaterialProgramSetup *mpProgramSetup = nullptr;
		iTexture *mpTexture[MAX_TEXTUREUNITS] = {};
		iVertexBuffer *mpVtxBuffer = nullptr;

		bool mbHasModelMatrix = false;
		cMatrixf mtxModelMatrix;
		bool mbHasInvModelMatrix = false;
		cMatrixf mtxInvModelMatrix;

		iTexture *mpRefraction = nullptr;
		iGpuProgram *mpRefractionProgram = nullptr;
		eMaterialTexture mRefractionDiffuseTexture = eMaterialTexture_Diffuse;
		iTexture *mpSpecular = nullptr;
		iTexture *mpDiffuse = nullptr;
		iTexture *mpRefractionDiffuse = nullptr;
		float mfValue = 1;
		bool mbRefractionUsesEye = false;
		bool mbRefractionUsesTime = false;
		bool mbRefractionUsesDiffuse = false;
		bool mbRefractionSkipsStandardTrans = false;
	};

	bool cTransparentDraw_Compare(const cTransparentDraw &aDrawA, const cTransparentDraw &aDrawB);

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

		int GetTransparentDrawNum(){ return (int)mvTransparentDraws.size(); }
		cTransparentDraw& GetTransparentDraw(int alIdx){ return mvTransparentDraws[alIdx]; }

		cLight3DIterator GetLightIt();
		cRenderableIterator GetObjectIt();

		int GetLightNum(){ return mlLightNum; }
		iLight3D* GetLight(int alLightIdx){ return mvLights[alLightIdx].mpLight; }
		int GetLightObjects(int alLightIdx){ return mvLights[alLightIdx].mlObjectCount; }
		int GetObjectNum();

		void Compile();

		int GetRenderCount(){ return mlRenderCount;}
		int GetLastRenderCount(){ return mlLastRenderCount;}

		void SetFrameTime(float afTime){ mfFrameTime = afTime;}

		cRenderQueue* GetQueue(eMaterialRenderType aPassType, int alLightNum);

		static inline int GetGlobalRenderCount(){ return mlGlobalRenderCount;}

	private:

		void AddDraw(iRenderable* apObject,eMaterialRenderType mPassType,int alLightNum,
						iLight3D* apLight, int alPass);

		static int mlGlobalRenderCount;

		tLight3DSet m_setLights;
		std::vector<cRenderLight> mvLights;
		int mlLightNum = 0;

		tRenderableSet m_setObjects;

		tFogObjectSet m_setFogObjects;
		std::vector<cTransparentDraw> mvTransparentDraws;

		cRenderQueue mDepthQueue;
		cRenderQueue mDiffuseQueue;


		int mlRenderCount;
		int mlLastRenderCount;

		float mfFrameTime;


		cCamera3D* mpCamera;

		cGraphics *mpGraphics;
	};
};
#endif // HPL_RENDER_LIST_H
