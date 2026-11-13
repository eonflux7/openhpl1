// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "physics/PhysicsWorld.h"
#include <algorithm>

#include "physics/CollideShape.h"
#include "physics/PhysicsBody.h"
#include "physics/PhysicsMaterial.h"
#include "physics/CharacterBody.h"
#include "physics/PhysicsJoint.h"
#include "physics/PhysicsController.h"
#include "physics/SurfaceData.h"
#include "system/LowLevelSystem.h"
#include "system/System.h"
#include "math/Math.h"
#include "graphics/LowLevelGraphics.h"
#include "scene/World3D.h"
#include "scene/PortalContainer.h"

namespace hpl {

	float iPhysicsMaterial::mfContactSkin = 0;

	iPhysicsWorld::iPhysicsWorld()
	{
		mbLogDebug = false;
		mbSaveContactPoints = false;
	}

	iPhysicsWorld::~iPhysicsWorld()
	{

	}

	void iPhysicsWorld::Update(float afTimeStep)
	{
		//Clear all contact points.
		mvContactPoints.clear();

		//Update controllers
		tPhysicsControllerListIt CtrlIt = mlstControllers.begin();
		for(; CtrlIt != mlstControllers.end(); ++CtrlIt)
		{
			iPhysicsController *pCtrl = *CtrlIt;

			pCtrl->Update(afTimeStep);
		}

		//Update character bodies
		unsigned int lTime = GetApplicationTime();
		tCharacterBodyListIt CharIt = mlstCharBodies.begin();
		for(; CharIt != mlstCharBodies.end(); ++CharIt)
		{
			iCharacterBody *pBody = *CharIt;

			{
				pBody->Update(afTimeStep);//20.0f);
			}
		}

		//Update the rigid bodies before simulation.
		tPhysicsBodyListIt BodyIt = mlstBodies.begin();
		for(; BodyIt != mlstBodies.end(); ++BodyIt)
		{
			iPhysicsBody *pBody = *BodyIt;

			pBody->UpdateBeforeSimulate(afTimeStep);

		}

		//Simulate the physics
		lTime = GetApplicationTime();
		Simulate(afTimeStep);

		//Bodies out of the static bodies they started inside collide with them again
		for(iPhysicsBody *pBody : mlstBodies)
		{
			std::vector<iPhysicsBody*> &vEmbedding = pBody->GetEmbeddingBodies();
			for(size_t i=0; i<vEmbedding.size(); )
			{
				if(GetBodyOverlap(pBody, vEmbedding[i]) < 0) vEmbedding.erase(vEmbedding.begin() + i);
				else ++i;
			}
		}

		//Update the joints after simulation.
		tPhysicsJointListIt JointIt = mlstJoints.begin();
		for(; JointIt != mlstJoints.end(); )
		{
			iPhysicsJoint *pJoint = *JointIt;

			pJoint->OnPhysicsUpdate();

			if(pJoint->CheckBreakage())
			{
				JointIt = mlstJoints.erase(JointIt);
				delete pJoint;
			}
			else
			{
				++JointIt;
			}
		}
		//Update the rigid bodies after simulation.
		BodyIt = mlstBodies.begin();
		for(; BodyIt != mlstBodies.end(); ++BodyIt)
		{
			iPhysicsBody *pBody = *BodyIt;

			pBody->UpdateAfterSimulate(afTimeStep);
		}
	}

	void iPhysicsWorld::DestroyShape(iCollideShape *apShape)
	{
		apShape->DecUserCount();
		if(apShape->HasUsers()==false)
		{
			STLFindAndDelete(mlstShapes, apShape);
		}
	}

	void iPhysicsWorld::DestroyBody(iPhysicsBody* apBody)
	{
		for(iPhysicsBody *pBody : mlstBodies)
		{
			std::vector<iPhysicsBody*> &vEmbedding = pBody->GetEmbeddingBodies();
			vEmbedding.erase(std::remove(vEmbedding.begin(), vEmbedding.end(), apBody), vEmbedding.end());
		}

		tPhysicsBodyListIt it = mlstBodies.begin();
		for(; it != mlstBodies.end(); ++it)
		{
			iPhysicsBody *pBody = *it;
			if(pBody == apBody)
			{
				if(mpWorld3D) mpWorld3D->GetPortalContainer()->RemoveEntity(pBody);
				pBody->Destroy();
				delete pBody;
				mlstBodies.erase(it);
				return;
			}
		}
	}

	iPhysicsBody *iPhysicsWorld::GetBody(const tString &asName)
	{
		return (iPhysicsBody*)STLFindByName(mlstBodies, asName);
	}

	cPhysicsBodyIterator iPhysicsWorld::GetBodyIterator()
	{
		return cPhysicsBodyIterator(&mlstBodies);
	}

	void iPhysicsWorld::StoreInterpolation()
	{
		for(tPhysicsBodyListIt it = mlstBodies.begin(); it != mlstBodies.end(); ++it)
			(*it)->StoreInterpolation();
	}

	void iPhysicsWorld::ApplyInterpolation(float afT)
	{
		for(tPhysicsBodyListIt it = mlstBodies.begin(); it != mlstBodies.end(); ++it)
			(*it)->ApplyInterpolation(afT);
	}

	void iPhysicsWorld::RestoreInterpolation()
	{
		for(tPhysicsBodyListIt it = mlstBodies.begin(); it != mlstBodies.end(); ++it)
			(*it)->RestoreInterpolation();
	}

	void iPhysicsWorld::GetBodiesInBV(cBoundingVolume *apBV, tPhysicsBodyList *apBodyList)
	{
		tPhysicsBodyListIt BodyIt = mlstBodies.begin();
		for(; BodyIt != mlstBodies.end(); ++BodyIt)
		{
			iPhysicsBody *pBody = *BodyIt;
			if(pBody->GetMass() > 0 && cMath::CheckCollisionBV(*apBV,*pBody->GetBV()))
			{
				apBodyList->push_back(pBody);
			}
		}
	}

	void iPhysicsWorld::EnableBodiesInBV(cBoundingVolume *apBV, bool abEnabled)
	{
		tPhysicsBodyListIt BodyIt = mlstBodies.begin();
		for(; BodyIt != mlstBodies.end(); ++BodyIt)
		{
			iPhysicsBody *pBody = *BodyIt;

			if(pBody->GetMass() > 0 && cMath::CheckCollisionBV(*apBV,*pBody->GetBV()))
			{

				pBody->SetEnabled(abEnabled);
			}
		}
	}

	void iPhysicsWorld::DestroyJoint(iPhysicsJoint* apJoint)
	{
		STLFindAndDelete(mlstJoints, apJoint);
	}

	iPhysicsJoint *iPhysicsWorld::GetJoint(const tString &asName)
	{
		return (iPhysicsJoint*)STLFindByName(mlstJoints, asName);
	}

	cPhysicsJointIterator iPhysicsWorld::GetJointIterator()
	{
		return cPhysicsJointIterator(&mlstJoints);
	}

	void iPhysicsWorld::DestroyCharacterBody(iCharacterBody* apBody)
	{
		STLFindAndDelete(mlstCharBodies, apBody);
	}

	iPhysicsBody *iPhysicsWorld::GetCharacterBody(const tString &asName)
	{
		return (iPhysicsBody*)STLFindByName(mlstCharBodies, asName);
	}

	iPhysicsMaterial* iPhysicsWorld::GetMaterialFromName(const tString &asName)
	{
		tPhysicsMaterialMapIt it = m_mapMaterials.find(asName);
		if(it == m_mapMaterials.end()){
			return nullptr;
		}

		iPhysicsMaterial *pMaterial =it->second;

		if(pMaterial->IsPreloaded()==false && pMaterial->GetSurfaceData())
		{
			pMaterial->SetPreloaded(true);
			pMaterial->GetSurfaceData()->PreloadData();
		}

		return pMaterial;
	}

	cPhysicsMaterialIterator iPhysicsWorld::GetMaterialIterator()
	{
		return cPhysicsMaterialIterator(&m_mapMaterials);
	}

	void iPhysicsWorld::DestroyAll()
	{
		STLDeleteAll(mlstCharBodies);

		//Bodies
		tPhysicsBodyListIt it = mlstBodies.begin();
		for(; it != mlstBodies.end(); ++it)
		{
			iPhysicsBody *pBody = *it;
			pBody->Destroy();
			delete pBody;
		}
		mlstBodies.clear();

		STLDeleteAll(mlstShapes);
		STLDeleteAll(mlstJoints);
		STLDeleteAll(mlstControllers);
		STLMapDeleteAll(m_mapMaterials);
	}

	void iPhysicsWorld::AddSaveData(cSaveDataHandler* apHandler)
	{
		//Add all bodies
		tPhysicsBodyListIt BodyIt = mlstBodies.begin();
		for(; BodyIt != mlstBodies.end(); ++BodyIt)
		{
			iPhysicsBody *pBody = *BodyIt;

			if(pBody->IsSaved())
			{
				iSaveData *pData =pBody->CreateSaveData();
				pBody->SaveToSaveData(pData);
				apHandler->Add(pData);
			}
		}

		//Add all character bodies
		tCharacterBodyListIt CharIt = mlstCharBodies.begin();
		for(; CharIt != mlstCharBodies.end(); ++CharIt)
		{
			iCharacterBody *pBody = *CharIt;

			if(pBody->IsSaved())
			{
				iSaveData *pData = pBody->CreateSaveData();
				pBody->SaveToSaveData(pData);
				apHandler->Add(pData);
			}
		}

		//Add all joints
		tPhysicsJointListIt JointIt = mlstJoints.begin();
		for(; JointIt != mlstJoints.end(); ++JointIt)
		{
			iPhysicsJoint *pJoint = *JointIt;

			if(pJoint->IsSaved())
			{
				iSaveData *pData =pJoint->CreateSaveData();
				pJoint->SaveToSaveData(pData);
				apHandler->Add(pData);
			}
		}
	}

	void iPhysicsWorld::DestroyController(iPhysicsController *apController)
	{
		STLFindAndDelete(mlstControllers, apController);
	}

	void iPhysicsWorld::RenderContactPoints(iLowLevelGraphics *apLowLevel, const cColor& aPointColor,
											const cColor& aLineColor)
	{
		for(size_t i=0; i < mvContactPoints.size(); i++)
		{
			apLowLevel->DrawSphere(mvContactPoints[i].mvPoint,0.2f, aPointColor);
			apLowLevel->DrawLine(mvContactPoints[i].mvPoint,
								mvContactPoints[i].mvNormal * mvContactPoints[i].mfDepth *0.2f,
								aLineColor);
		}
	}

	float iPhysicsWorld::GetBodyOverlap(iPhysicsBody *apBodyA, iPhysicsBody *apBodyB)
	{
		if(cMath::CheckCollisionBV(*apBodyA->GetBV(), *apBodyB->GetBV())==false) return -1;

		cCollideData data;
		data.SetMaxSize(8);
		if(CheckShapeCollision(apBodyA->GetShape(), apBodyA->GetLocalMatrix(),
				apBodyB->GetShape(), apBodyB->GetLocalMatrix(), data, 8)==false) return -1;

		float fDepth = 0;
		for(int i=0; i<data.mlNumOfPoints; ++i) fDepth = cMath::Max(fDepth, data.mvContactPoints[i].mfDepth);
		return fDepth;
	}

	float iPhysicsWorld::mfScriptReleaseDepth = 0;

	int iPhysicsWorld::LetOutOfStaticBodies(iPhysicsBody *apBody, float afMinDepth)
	{
		apBody->GetEmbeddingBodies().clear();
		for(iPhysicsBody *pOther : mlstBodies)
		{
			if(pOther == apBody || pOther->GetMass() != 0 || pOther->IsCharacter() || pOther->GetCollide()==false) continue;
			if(GetBodyOverlap(apBody, pOther) >= afMinDepth) apBody->AddEmbeddingBody(pOther);
		}
		return (int)apBody->GetEmbeddingBodies().size();
	}

	bool iPhysicsWorld::CheckShapeWorldCollision(cVector3f *apNewPos,
							iCollideShape* apShape, const cMatrixf& a_mtxTransform,
							iPhysicsBody *apSkipBody, bool abSkipStatic, bool abIsCharacter,
							iPhysicsWorldCollisionCallback *apCallback,
							bool abCollideCharacter,
							bool abDebug)
	{
		cCollideData collideData;

		cVector3f vPushVec(0,0,0);
		bool bCollide = false;

		cBoundingVolume boundingVolume = apShape->GetBoundingVolume();
		boundingVolume.SetTransform(cMath::MatrixMul(a_mtxTransform, boundingVolume.GetTransform()));

		cPortalContainerEntityIterator entIt = mpWorld3D->GetPortalContainer()->GetEntityIterator(&boundingVolume);
		while(entIt.HasNext())
		{
			iPhysicsBody *pBody = static_cast<iPhysicsBody*>(entIt.Next());




			if(pBody->IsCharacter() && abCollideCharacter==false) continue;
			if(pBody->IsActive()==false)continue;
			if(pBody == apSkipBody) continue;
			if(abSkipStatic && pBody->GetMass()==0) continue;
			if(abIsCharacter && pBody->GetCollideCharacter()==false) continue;
			if(abIsCharacter==false && pBody->GetCollide()==false) continue;

			if(cMath::CheckCollisionBV(boundingVolume,*pBody->GetBV())==false)
			{
				continue;

			}

			collideData.SetMaxSize(32);
			bool bRet = CheckShapeCollision(apShape,a_mtxTransform, pBody->GetShape(),pBody->GetLocalMatrix(),
											collideData, 32);

			if(bRet)
			{

				if(apCallback) apCallback->OnCollision(pBody, &collideData);

				for(int i=0; i< collideData.mlNumOfPoints; i++)
				{
					cCollidePoint &point = collideData.mvContactPoints[i];

					cVector3f vPush = point.mvNormal*point.mfDepth;

					if(std::abs(vPushVec.x) < std::abs(vPush.x)) vPushVec.x = vPush.x;
					if(std::abs(vPushVec.y) < std::abs(vPush.y)) vPushVec.y = vPush.y;
					if(std::abs(vPushVec.z) < std::abs(vPush.z)) vPushVec.z = vPush.z;
				}
				bCollide = true;
			}
		}

		if(apNewPos)
			*apNewPos = a_mtxTransform.GetTranslation() + vPushVec;
		return bCollide;
	}
}
