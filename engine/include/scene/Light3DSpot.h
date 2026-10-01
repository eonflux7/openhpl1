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
#ifndef HPL_LIGHT3D_SPOT_H
#define HPL_LIGHT3D_SPOT_H

#include "scene/Light3D.h"

namespace hpl {

	class cResources;
	class iTexture;
	class cFrustum;

	//------------------------------------------

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

	//------------------------------------------

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
