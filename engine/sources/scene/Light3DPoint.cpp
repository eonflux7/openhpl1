// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "scene/Light3DPoint.h"

#include "graphics/LowLevelGraphics.h"
#include "graphics/Renderer3D.h"
#include "scene/Camera3D.h"
#include "math/Math.h"

#include "scene/World3D.h"
#include "scene/Scene.h"
#include "game/Game.h"

#include "scene/SectorVisibility.h"
#include "scene/PortalContainer.h"

namespace hpl {

	cLight3DPoint::cLight3DPoint(tString asName, cResources *apResources) : iLight3D(asName,apResources)
	{
		mLightType = eLight3DType_Point;

		UpdateBoundingVolume();
	}

	cSectorVisibilityContainer* cLight3DPoint::CreateSectorVisibility()
	{
		return mpWorld3D->GetPortalContainer()->CreateVisibilityFromBV(GetBoundingVolume());
	}

	void cLight3DPoint::UpdateBoundingVolume()
	{
		mBoundingVolume.SetSize(mfFarAttenuation*2);
		mBoundingVolume.SetPosition(GetWorldPosition());
	}

	bool cLight3DPoint::CreateClipRect(cRect2l &aClipRect, cRenderSettings *apRenderSettings,
											iLowLevelGraphics *apLowLevelGraphics)
	{
		cVector2f vScreenSize = apLowLevelGraphics->GetRenderSize();
		bool bVisible = cMath::GetClipRectFromBV(aClipRect,*GetBoundingVolume(),
										apRenderSettings->mpCamera->GetViewMatrix(),
										apRenderSettings->mpCamera->GetProjectionMatrix(),
										apRenderSettings->mpCamera->GetNearClipPlane(),
										cVector2l((int)vScreenSize.x,(int)vScreenSize.y));
		return bVisible;
	}

	kBeginSerialize(cSaveData_cLight3DPoint,cSaveData_iLight3D)
	kEndSerialize()

	iSaveObject* cSaveData_cLight3DPoint::CreateSaveObject(cSaveObjectHandler *apSaveObjectHandler,cGame *apGame)
	{
		cWorld3D *pWorld = apGame->GetScene()->GetWorld3D();

		return pWorld->CreateLightPoint(msName);
	}

	int cSaveData_cLight3DPoint::GetSaveCreatePrio()
	{
		return 3;
	}

	iSaveData* cLight3DPoint::CreateSaveData()
	{
		return new cSaveData_cLight3DPoint();
	}

	void cLight3DPoint::SaveToSaveData(iSaveData *apSaveData)
	{
		kSaveData_SaveToBegin(cLight3DPoint);
	}

	void cLight3DPoint::LoadFromSaveData(iSaveData *apSaveData)
	{
		kSaveData_LoadFromBegin(cLight3DPoint);
	}

	void cLight3DPoint::SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame)
	{
		kSaveData_SetupBegin(cLight3DPoint);
	}
}
