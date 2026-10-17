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
#ifndef HPL_LIGHT3D_H
#define HPL_LIGHT3D_H

#include <cstdint>

#include <list>
#include <set>
#include "scene/Entity3D.h"
#include "scene/Light.h"
#include "graphics/GraphicsTypes.h"
#include "graphics/Renderable.h"

class TiXmlElement;

namespace hpl {

	enum eLight3DType
	{
		eLight3DType_Point,
		eLight3DType_Spot,
		eLight3DType_LastEnum
	};

	class iLowLevelGraphics;
	class cRenderSettings;
	class cCamera3D;
	class cFrustum;
	class iGpuProgram;
	class iTexture;
	class cTextureManager;
	class cResources;
	class cFileSearcher;
	class cBillboard;
	class cSectorVisibilityContainer;


	typedef std::set<iRenderable*> tCasterCacheSet;
	typedef tCasterCacheSet::iterator tCasterCacheSetIt;

	//------------------------------------------

	kSaveData_ChildClass(iRenderable,iLight3D)
	{
		kSaveData_ClassInit(iLight3D)
	public:
		tString msFalloffMap;
		cContainerList<int> mlstBillboardIds;

		cColor mDiffuseColor;
		cColor mSpecularColor;
		float mfIntensity;
		float mfFarAttenuation;
		float mfNearAttenuation;
		float mfSourceRadius;

		bool mbCastShadows;
		bool mbAffectMaterial;

		cColor mColAdd;
		float mfRadiusAdd;
		cColor mDestCol;
		float mfDestRadius;
		float mfFadeTime;

		bool mbFlickering;
		tString msFlickerOffSound;
		tString msFlickerOnSound;
		tString msFlickerOffPS;
		tString msFlickerOnPS;
		float mfFlickerOnMinLength;
		float mfFlickerOffMinLength;
		float mfFlickerOnMaxLength;
		float mfFlickerOffMaxLength;
		cColor mFlickerOffColor;
		float mfFlickerOffRadius;
		bool mbFlickerFade;
		float mfFlickerOnFadeLength;
		float mfFlickerOffFadeLength;

		cColor mFlickerOnColor;
		float mfFlickerOnRadius;

		bool mbFlickerOn;
		float mfFlickerTime;
		float mfFlickerStateLength;
	};

	//------------------------------------------

	class iLight3D : public iLight, public iRenderable
	{
	#ifdef __GNUC__
		typedef iRenderable __super;
	#endif
	public:
		iLight3D(tString asName, cResources *apResources);
		~iLight3D() override;

		void UpdateLogic(float afTimeStep) override;

		void SetFarAttenuation(float afX) override;
		void SetNearAttenuation(float afX) override;

		cVector3f GetLightPosition() override;

		virtual bool BeginDraw(cRenderSettings *apRenderSettings,iLowLevelGraphics *apLowLevelGraphics);
		virtual void EndDraw(cRenderSettings *apRenderSettings,iLowLevelGraphics *apLowLevelGraphics);

		bool CheckObjectIntersection(iRenderable *apObject);

		void AddShadowCaster(iRenderable* apObject, cFrustum* apFrustum, bool abStatic,cRenderList *apRenderList);
		bool HasStaticCasters();
		void ClearCasters(bool abClearStatic);
		tCasterCacheSet* GetStaticCasters(){ return &m_setStaticCasters;}
		tCasterCacheSet* GetDynamicCasters(){ return &m_setDynamicCasters;}

		void SetAllStaticCastersAdded(bool abX){ mbStaticCasterAdded = abX;}
		bool AllStaticCastersAdded(){ return mbStaticCasterAdded;}

		/// A light that the map file itself has, not one of an entity or made by the game.
		void SetPartOfMap(bool abX){ mbPartOfMap = abX;}
		bool IsPartOfMap(){ return mbPartOfMap;}

		eLight3DType GetLightType(){ return mLightType;}
		// Runtime identity, independent of reused addresses and serialized IDs.
		uint64_t GetShadowCacheId() const { return mlShadowCacheId; }

		//iEntity implementation
		tString GetEntityType() override{ return "iLight3D";}

		bool IsVisible() override;
		void SetVisible(bool abVisible) override;

		iTexture *GetFalloffMap();
		void SetFalloffMap(iTexture* apTexture);

		//Renderable implementation:
		iMaterial *GetMaterial() override{ return nullptr;}
		iVertexBuffer* GetVertexBuffer() override{ return nullptr;}

		bool IsShadowCaster() override{ return false;}
		eRenderableType GetRenderType() override{ return eRenderableType_Light;}

		cBoundingVolume* GetBoundingVolume() override;

		int GetMatrixUpdateCount() override{ return GetTransformUpdateCount();}

		cMatrixf* GetModelMatrix(cCamera3D* apCamera) override;

		void LoadXMLProperties(const tString asFile);

		void AttachBillboard(cBillboard *apBillboard);
		void RemoveBillboard(cBillboard *apBillboard);
		std::vector<cBillboard*>* GetBillboardVec(){ return &mvBillboards;}

		inline iTexture* GetTempTexture(size_t alIdx){ return mvTempTextures[alIdx];}
		inline void SetTempTexture(size_t alIdx,iTexture* apTex){ mvTempTextures[alIdx]=apTex;}

		void SetOnlyAffectInSector(bool abX){ mbOnlyAffectInInSector = abX;}
		bool GetOnlyAffectInSector(){return mbOnlyAffectInInSector;}

		//SaveObject implementation
		iSaveData* CreateSaveData() override;
		void SaveToSaveData(iSaveData *apSaveData) override;
		void LoadFromSaveData(iSaveData *apSaveData) override;
		void SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame) override;

	protected:
		inline static uint64_t mlNextShadowCacheId = 0;
		uint64_t mlShadowCacheId = ++mlNextShadowCacheId;
		void OnFlickerOff() override;
		void OnFlickerOn() override;
		void OnSetDiffuse() override;

		virtual cSectorVisibilityContainer* CreateSectorVisibility()=0;
		virtual void ExtraXMLProperties(TiXmlElement *apMainElem){}
		virtual void UpdateBoundingVolume()=0;
		void ReachChanged();
		virtual bool CreateClipRect(cRect2l &aCliprect, cRenderSettings *apRenderSettings,iLowLevelGraphics *apLowLevelGraphics)=0;

		eLight3DType mLightType;

		cTextureManager *mpTextureManager;
		cFileSearcher *mpFileSearcher;

		iTexture *mpFalloffMap;

		iTexture *mvTempTextures[3];

		std::vector<cBillboard*> mvBillboards;

		cMatrixf mtxTemp;

		tCasterCacheSet m_setStaticCasters;
		tCasterCacheSet m_setDynamicCasters;

		bool mbStaticCasterAdded;
		bool mbPartOfMap;

		bool mbOnlyAffectInInSector;

		int mlSectorVisibilityCount;
		cSectorVisibilityContainer *mpVisSectorCont;
	};

	typedef std::list<iLight3D*> tLight3DList;
	typedef tLight3DList::iterator tLight3DListIt;
};
#endif // HPL_LIGHT3D_H
