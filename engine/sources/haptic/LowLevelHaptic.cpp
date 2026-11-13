// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "haptic/LowLevelHaptic.h"

#include "haptic/HapticSurface.h"
#include "haptic/HapticShape.h"
#include "haptic/HapticForce.h"

#include "system/LowLevelSystem.h"
#include "system/SystemTypes.h"

#include "scene/Camera3D.h"
#include "math/Math.h"
#include "physics/PhysicsBody.h"
#include "scene/SubMeshEntity.h"

namespace hpl {

	iLowLevelHaptic::iLowLevelHaptic()
	{
		mpCamera = nullptr;
		mvCameraOffset = 0;

		mvMinMousePos = cVector2f(-50,-50);
		mvMaxMousePos = cVector2f(50,50);
		mvScreenSize = cVector2f(800,600);

		mbUpdateShapes = true;
	}

	iLowLevelHaptic::~iLowLevelHaptic()
	{

	}

	iHapticSurface* iLowLevelHaptic::GetSurfaceFromName(const tString &asName)
	{
		tHapticSurfaceMapIt it = m_mapSurfaces.find(asName);
		if(it == m_mapSurfaces.end()){
			return nullptr;
		}

		iHapticSurface* pSurface =it->second;

		return pSurface;
	}

	bool iLowLevelHaptic::Init(cResources *apResources)
	{
		mpResources = apResources;

		if(InitLowLevel()==false) return false;

		//Init variables
		mvCurrentHardwarePos = GetHardwarePosition();
		mvPreviousHardwarePos = mvCurrentHardwarePos;
		mbScreenPosFirstTime = true;

		return true;
	}

	void iLowLevelHaptic::Update(float afTimeStep)
	{
		UpdateLowLevel(afTimeStep);

		//Hardware position calculations
		static bool bHardwareSet = false;

		mvPreviousHardwarePos = mvCurrentHardwarePos;
		mvCurrentHardwarePos = GetHardwarePosition();
		if(bHardwareSet==false){
			mvPreviousHardwarePos = mvCurrentHardwarePos;
			bHardwareSet = true;
		}

		//Proxy screen postion calculations
		if(mpCamera)
		{
			mvPreviousScreenPos = mvCurrentScreenPos;

			cVector3f vProjPos = cMath::MatrixMul(mpCamera->GetViewMatrix(), GetProxyPosition());
			vProjPos = cMath::MatrixMulDivideW(mpCamera->GetProjectionMatrix(),vProjPos);

			cVector2f vPos2D(	(vProjPos.x+1) * 0.5f, (-vProjPos.y+1)* 0.5f);

			mvCurrentScreenPos = vPos2D;

			if(mbScreenPosFirstTime)
			{
				mvPreviousScreenPos = mvCurrentScreenPos;
				mbScreenPosFirstTime = false;
			}
		}

		//Update shape transforms
		if(mbUpdateShapes)
		{
			tHapticShapeListIt it = mlstShapes.begin();
			for(; it != mlstShapes.end(); ++it)
			{
				iHapticShape *pShape = *it;

				//If a body is attached, use it to update shape
				if(pShape->GetBody())
				{
					iPhysicsBody *pBody = pShape->GetBody();
					if(pBody->GetTransformUpdateCount() != pShape->GetTransformCount())
					{
						pShape->SetTransformCount(pBody->GetTransformUpdateCount());
						pShape->SetTransform(pBody->GetLocalMatrix());
					}
				}
				else if(pShape->GetSubMeshEntity())
				{
					cSubMeshEntity *pSubEntity = pShape->GetSubMeshEntity();
					if(pSubEntity->GetTransformUpdateCount() != pShape->GetTransformCount())
					{
						pShape->SetTransformCount(pSubEntity->GetTransformUpdateCount());
						pShape->SetTransform(pSubEntity->GetWorldMatrix());
					}
				}
			}
		}
	}

	cVector3f iLowLevelHaptic::GetHardwarePosDelta()
	{
		return mvCurrentHardwarePos - mvPreviousHardwarePos;
	}

	cVector2f iLowLevelHaptic::GetProxyScreenPos(const cVector2f& avScreenSize)
	{
		return mvCurrentScreenPos * avScreenSize;
	}

	cVector2f iLowLevelHaptic::GetProxyScreenDeltaPos(const cVector2f& avScreenSize)
	{
		return (mvCurrentScreenPos - mvPreviousScreenPos) * avScreenSize;
	}

	cVector2f iLowLevelHaptic::GetRelativeVirtualMousePos()
	{
		cVector3f vRel = GetHardwarePosDelta();
		cVector2f vPos =  cVector2f(vRel.x,vRel.y)/ (mvMaxMousePos - mvMinMousePos);
		vPos.y = -vPos.y;
		vPos = vPos * mvScreenSize;
		return vPos;
	}

	cVector2f iLowLevelHaptic::GetVirtualMousePos()
	{
		cVector2f vPos =	(cVector2f(mvCurrentHardwarePos.x,mvCurrentHardwarePos.y) - mvMinMousePos) /
							(mvMaxMousePos - mvMinMousePos);
		vPos.y = 1 - vPos.y;
		vPos = vPos * mvScreenSize;
		return vPos;
	}
	void iLowLevelHaptic::SetVirtualMousePosBounds(const cVector2f &avMin,const cVector2f &avMax,
														const cVector2f &avScreenSize)
	{
		mvMinMousePos = avMin;
		mvMaxMousePos = avMax;
		mvScreenSize = avScreenSize;
	}

	void iLowLevelHaptic::DestroyShape(iHapticShape* apShape)
	{
		STLFindAndDelete(mlstShapes, apShape);
	}

	bool iLowLevelHaptic::ShapeExists(iHapticShape* apShape)
	{
		tHapticShapeListIt it = mlstShapes.begin();
		for(; it != mlstShapes.end(); ++it)
		{
			if(apShape == *it) return true;
		}
		return false;
	}

	cHapticShapeIterator iLowLevelHaptic::GetShapeIterator()
	{
		return cHapticShapeIterator(&mlstShapes);
	}

	void iLowLevelHaptic::DestroyForce(iHapticForce* apForce)
	{
		STLFindAndDelete(mlstForces, apForce);
	}

	void iLowLevelHaptic::StopAllForces()
	{
		tHapticForceListIt it = mlstForces.begin();
		for(; it != mlstForces.end(); ++it)
		{
			iHapticForce *pForce = *it;
			pForce->SetActive(false);
		}
	}

	void iLowLevelHaptic::DestroyAllShapes()
	{
		STLDeleteAll(mlstShapes);

	}

	void iLowLevelHaptic::DestroyAll()
	{
		STLMapDeleteAll(m_mapSurfaces);
		STLDeleteAll(mlstForces);
		STLDeleteAll(mlstShapes);
	}
}
