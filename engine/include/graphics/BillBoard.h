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
#ifndef HPL_BILLBOARD_H
#define HPL_BILLBOARD_H

#include "math/MathTypes.h"
#include "graphics/GraphicsTypes.h"
#include "system/SystemTypes.h"
#include "scene/Entity3D.h"
#include "graphics/Renderable.h"
#include "RenderList.h"

namespace hpl {

	enum eBillboardType
	{
		eBillboardType_Point,
		eBillboardType_Axis,
		eBillboardType_LastEnum
	};

	class cMaterialManager;
	class cResources;
	class cGraphics;
	class cMeshCreator;
	class cFileSearcher;
	class iLowLevelGraphics;
	class iMaterial;
	class iVertexBuffer;
	class iOcclusionQuery;
	class iGpuProgram;
	class cGpuProgramManager;
	class cResources;

	//------------------------------------------

	kSaveData_ChildClass(iRenderable,cBillboard)
	{
		kSaveData_ClassInit(cBillboard)
	public:
		tString msMaterial;

		int mType;
		cVector2f mvSize;
		cVector3f mvAxis;
		float mfForwardOffset;
		cColor mColor;
		float mfHaloAlpha;

		bool mbIsHalo;
		cVector3f mvHaloSourceSize;
		bool mbHaloSourceIsParent;

		iSaveObject* CreateSaveObject(cSaveObjectHandler *apSaveObjectHandler,cGame *apGame) override;
		int GetSaveCreatePrio() override;
	};

	//------------------------------------------

	class cBillboard : public iRenderable
	{
	#ifdef __GNUC__
		typedef iRenderable __super;
	#endif
	public:
		cBillboard(const tString asName,const cVector2f& avSize, cResources *apResources,
					cGraphics *apGraphics);
		~cBillboard() override;

		void SetMaterial(iMaterial * apMaterial);

		void SetSize(const cVector2f& avSize);
		cVector2f GetSize(){ return mvSize;}

		void SetType(eBillboardType aType);
		eBillboardType GetType(){ return mType;}

		void SetAxis(const cVector3f& avAxis);
		cVector3f GetAxis(){ return mvAxis;}

		void SetForwardOffset(float afOffset);
		float GetForwardOffset(){ return mfForwardOffset;}

		void SetColor(const cColor &aColor);
		const cColor& GetColor(){ return mColor;}

		void SetHaloAlpha(float afX);
		float GetHaloAlpha(){ return mfHaloAlpha;}

		void LoadXMLProperties(const tString asFile);

		/////////////////////////////////
		//Halo stuff
		void SetIsHalo(bool abX);
		bool IsHalo(){ return mbIsHalo;}

		void SetHaloSourceSize(const cVector3f &avSize);
		cVector3f GetHaloSourceSize(){return mvHaloSourceSize;}

		void SetHaloSourceIsParent(bool abX);
		bool GetHaloSourceIsParent(){ return mbHaloSourceIsParent;}

		iOcclusionQuery* GetQuery(){ return mQueryObject.mpQuery;}
		iOcclusionQuery* GetMaxQuery(){ return mMaxQueryObject.mpQuery;}

		/////////////////////////////////
		//Entity implementation
		tString GetEntityType() override{ return "Billboard";}

		bool IsVisible() override;
		void SetVisible(bool abVisible) override{ SetRendered(abVisible); }

		//Renderable implementations
		iMaterial *GetMaterial() override{ return mpMaterial;}
		iVertexBuffer* GetVertexBuffer() override{return mpVtxBuffer;}

		void UpdateGraphics(cCamera3D *apCamera,float afFrameTime, cRenderList *apRenderList) override;

		// Halos and shafts drawn by shaders of their own, see cEffectSettings
		bool GetTransDraw(cCamera3D *apCamera, cTransDraw &aDraw) override;
		void SetupTransDraw(cCamera3D *apCamera, cRenderSettings *apSettings, cTransDraw &aDraw) override;

		bool IsShadowCaster() override{ return false;}

		cBoundingVolume* GetBoundingVolume() override;

		cMatrixf* GetModelMatrix(cCamera3D *apCamera) override;

		int GetMatrixUpdateCount() override;

		eRenderableType GetRenderType() override{ return eRenderableType_ParticleSystem;}

		//SaveObject implementation
		iSaveData* CreateSaveData() override;
		void SaveToSaveData(iSaveData *apSaveData) override;
		void LoadFromSaveData(iSaveData *apSaveData) override;
		void SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame) override;

		/// The number of distances a halo's picture is measured at, see hpl1_Halo.fragment
		static const int kHaloKnots = 32;

	private:
		eBillboardType ToType(const char* apString);

		void UpdateSourceBufferSize();

		bool DrawnAsHalo();
		bool DrawnAsShaft();
		const cColor* GetHaloProfile();

		cResources *mpResources;
		cGpuProgramManager *mpProgramManager;

		iGpuProgram *mpHaloVtxProgram;
		iGpuProgram *mpHaloFragProgram;
		/// The picture's colour by distance from its middle; null until asked for.
		const cColor *mpHaloProfile;

		iGpuProgram *mpShaftVtxProgram;
		iGpuProgram *mpShaftFragProgram;
		/// The box round the shaft, y along its axis
		iVertexBuffer *mpShaftBuffer;
		cVector2f mvShaftBufferSize;
		cMatrixf m_mtxShaft;

		cMaterialManager* mpMaterialManager;
		cFileSearcher *mpFileSearcher;
		iLowLevelGraphics* mpLowLevelGraphics;
		cMeshCreator *mpMeshCreator;

		iMaterial *mpMaterial;
		iVertexBuffer* mpVtxBuffer;

		iVertexBuffer* mpHaloSourceBuffer;

		cMatrixf m_mtxTempTransform;

		eBillboardType mType;
		cVector2f mvSize;
		cVector3f mvAxis;

		int mlLastRenderCount;

		bool mbIsHalo;
		cVector3f mvHaloSourceSize;
		bool mbHaloSourceIsParent;
		cOcclusionQueryObject mQueryObject;
		cOcclusionQueryObject mMaxQueryObject;

		float mfForwardOffset;
		cColor mColor;
		float mfHaloAlpha;
	};

};
#endif // HPL_BILLBOARD_H
