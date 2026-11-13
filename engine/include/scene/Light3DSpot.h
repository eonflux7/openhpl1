// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_LIGHT3D_SPOT_H
#define HPL_LIGHT3D_SPOT_H

#include "scene/Light3D.h"

namespace hpl {

	class cResources;
	class iTexture;
	class cFrustum;

	kSaveData_ChildClass(iLight3D,cLight3DSpot)
	{
		kSaveData_ClassInit(cLight3DSpot)
	public:
		tString msTexture;

		float mfFOV;
		float mfAspect;
		float mfNearClipPlane;

		iSaveObject* CreateSaveObject(cSaveObjectHandler *apSaveObjectHandler,cGame *apGame) override;
		int GetSaveCreatePrio() override;
	};

	class cLight3DSpot : public iLight3D
	{
	#ifdef __GNUC__
		typedef iLight3D __super;
	#endif
	public:
		cLight3DSpot(tString asName, cResources *apResources);
		~cLight3DSpot() override;

		const cMatrixf& GetViewMatrix();
		const cMatrixf& GetProjectionMatrix();
		const cMatrixf& GetViewProjMatrix();

		void SetTexture(iTexture *apTexture);
		iTexture* GetTexture();

		void SetFOV(float afAngle) { mfFOV = afAngle; mbProjectionUpdated = true;}
		float GetFOV() { return mfFOV;}

		void SetAspect(float afAngle) { mfAspect = afAngle; mbProjectionUpdated = true;}
		float GetAspect() { return mfAspect;}

		void SetNearClipPlane(float afX) { mfNearClipPlane = afX; mbProjectionUpdated = true;}
		float GetNearClipPlane() { return mfNearClipPlane;}

		void SetFarAttenuation(float afX) override;
		void SetNearAttenuation(float afX) override;

		cFrustum* GetFrustum();

		bool CollidesWithBV(cBoundingVolume *apBV) override;

		//SaveObject implementation
		iSaveData* CreateSaveData() override;
		void SaveToSaveData(iSaveData *apSaveData) override;
		void LoadFromSaveData(iSaveData *apSaveData) override;
		void SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame) override;

	private:
		cSectorVisibilityContainer* CreateSectorVisibility() override;
		void ExtraXMLProperties(TiXmlElement *apMainElem) override;
		void UpdateBoundingVolume() override;
		bool CreateClipRect(cRect2l &aCliprect,cRenderSettings *apRenderSettings,iLowLevelGraphics *apLowLevelGraphics) override;


		cMatrixf m_mtxProjection;
		cMatrixf m_mtxViewProj;
		cMatrixf m_mtxView;

		cFrustum *mpFrustum;

		iTexture *mpTexture;

		float mfFOV;
		float mfAspect;
		float mfNearClipPlane;

		bool mbProjectionUpdated;
		bool mbViewProjUpdated;
		bool mbFrustumUpdated;

		int mlViewProjMatrixCount;
		int mlViewMatrixCount;
		int mlFrustumMatrixCount;
	};

};
#endif // HPL_LIGHT3D_SPOT_H
