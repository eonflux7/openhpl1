// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "graphics/Renderable.h"

#include "math/Math.h"
#include "math/Frustum.h"
#include "system/LowLevelSystem.h"
#include "graphics/RenderList.h"

namespace hpl {

	iRenderable::iRenderable(const tString &asName) : iEntity3D(asName)
	{
		mbRendered = true;
		mlLastMatrixCount = -1;

		mbStatic = false;

		mlRenderCount = -1;

		mlCalcScaleMatrixCount = -1;
		mvCalcScale = cVector3f(1,1,1);

		mbForceShadow = false;

		mbIsOneSided = false;
		mvOneSidedNormal =0;
	}

	cMatrixf* iRenderable::GetInvModelMatrix()
	{
		cMatrixf *pModelMatrix = GetModelMatrix(nullptr);
		if(pModelMatrix==nullptr) return nullptr;

		if(mlLastMatrixCount != GetMatrixUpdateCount())
		{
			mlLastMatrixCount = GetMatrixUpdateCount();

			m_mtxInvModel = cMath::MatrixInverse(*pModelMatrix);
		}

		return &m_mtxInvModel;
	}

	const cVector3f& iRenderable::GetCalcScale()
	{
		cMatrixf *pModelMatrix = GetModelMatrix(nullptr);

		if(pModelMatrix != nullptr && mlCalcScaleMatrixCount != GetMatrixUpdateCount())
		{
			mlCalcScaleMatrixCount = GetMatrixUpdateCount();
			mvCalcScale.x = pModelMatrix->GetRight().Length();
			mvCalcScale.y = pModelMatrix->GetUp().Length();
			mvCalcScale.z = pModelMatrix->GetForward().Length();
		}

		return mvCalcScale;
	}

	bool iRenderable::CollidesWithBV(cBoundingVolume *apBV)
	{
		return cMath::CheckCollisionBV(*GetBoundingVolume(), *apBV);
	}

	bool iRenderable::CollidesWithFrustum(cFrustum *apFrustum)
	{
		return apFrustum->CollideBoundingVolume(GetBoundingVolume()) != eFrustumCollision_Outside;
	}

	kBeginSerializeVirtual(cSaveData_iRenderable,cSaveData_iEntity3D)
	kSerializeVar(mbStatic, eSerializeType_Bool)
	kSerializeVar(mbRendered, eSerializeType_Bool)
	kSerializeVar(mfZ, eSerializeType_Float32)
	kEndSerialize()

	iSaveData* iRenderable::CreateSaveData()
	{
		return nullptr;
	}

	void iRenderable::SaveToSaveData(iSaveData *apSaveData)
	{
		kSaveData_SaveToBegin(iRenderable);

		kSaveData_SaveTo(mbStatic);
		kSaveData_SaveTo(mbRendered);
		kSaveData_SaveTo(mfZ);
	}

	void iRenderable::LoadFromSaveData(iSaveData *apSaveData)
	{
		kSaveData_LoadFromBegin(iRenderable);

		kSaveData_LoadFrom(mbStatic);
		kSaveData_LoadFrom(mbRendered);
		kSaveData_LoadFrom(mfZ);
	}

	void iRenderable::SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame)
	{
		kSaveData_SetupBegin(iRenderable);
	}
}
