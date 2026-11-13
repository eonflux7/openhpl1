// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "physics/PhysicsJoint.h"

#include "physics/CollideShape.h"
#include "physics/PhysicsWorld.h"
#include "physics/PhysicsBody.h"
#include "physics/PhysicsController.h"
#include "system/LowLevelSystem.h"

#include "sound/Sound.h"
#include "scene/SoundEntity.h"
#include "sound/SoundHandler.h"
#include "sound/SoundChannel.h"

#include "scene/World3D.h"
#include "scene/Scene.h"

#include "game/Game.h"

#include "math/Math.h"

#include "game/ScriptFuncs.h"

namespace hpl {

	iPhysicsJoint::iPhysicsJoint(const tString &asName, iPhysicsBody *apParentBody, iPhysicsBody *apChildBody,
		iPhysicsWorld *apWorld,const cVector3f &avPivotPoint)
		: msName(asName), mpParentBody(apParentBody), mpChildBody(apChildBody), mpWorld(apWorld)
	{
		mMaxLimit.msSound = "";
		mMinLimit.msSound = "";

		if(apParentBody) {
			apParentBody->AddJoint(this);
			m_mtxParentBodySetup = apParentBody->GetLocalMatrix();
		}
		else{
			m_mtxParentBodySetup = cMatrixf::Identity;
		}

		m_mtxPrevChild = cMatrixf::Identity;
		m_mtxPrevParent = cMatrixf::Identity;

		apChildBody->AddJoint(this);
		m_mtxChildBodySetup = apChildBody->GetLocalMatrix();

		cMatrixf m_mtxInvChild = cMath::MatrixInverse(apChildBody->GetLocalMatrix());
		mvLocalPivot = cMath::MatrixMul(m_mtxInvChild,avPivotPoint);
		mvStartPivotPoint = avPivotPoint;

		msMoveSound = "";

		mbHasCollided = false;
		mpSound = nullptr;

		mpCallback = nullptr;

		mbAutoDeleteCallback = false;

		mpUserData = nullptr;

		mbBreakable = false;
		mfBreakForce = 0;
		msBreakSound = "";
		mbBroken = false;

		mfStickyMinDistance =0;
		mfStickyMaxDistance =0;

		mlLimitStepCount =0;

		mlSpeedCount =0;

		mbLimitAutoSleep = false;
		mfLimitAutoSleepDist = 0.02f;
		mlLimitAutoSleepNumSteps = 10;

	}

	iPhysicsJoint::~iPhysicsJoint()
	{
		if(mbAutoDeleteCallback && mpCallback) delete mpCallback;

		//Destroy all controllers.
		tPhysicsControllerMapIt it = m_mapControllers.begin();
		for(; it != m_mapControllers.end(); ++it)
		{
			mpWorld->DestroyController(it->second);
		}

		if(mpChildBody) mpChildBody->RemoveJoint(this);
		if(mpParentBody) mpParentBody->RemoveJoint(this);

		if(mpSound) mpWorld->GetWorld3D()->DestroySoundEntity(mpSound);

	}

	/**
	 * This should only be used by PhysicsBody.
	 */
	void iPhysicsJoint::RemoveBody(iPhysicsBody *apBody)
	{
		if(mpParentBody == apBody) mpParentBody = nullptr;
		if(mpChildBody == apBody) mpChildBody = nullptr;
	}

	void iPhysicsJoint::AddController(iPhysicsController *apController)
	{

		//Add controller top map
		m_mapControllers.insert(tPhysicsControllerMap::value_type(apController->GetName(), apController));

		//Set properties
		apController->SetBody(mpChildBody);
		apController->SetJoint(this);
	}

	iPhysicsController* iPhysicsJoint::GetController(const tString &asName)
	{
		tPhysicsControllerMapIt it = m_mapControllers.find(asName);
		if(it == m_mapControllers.end()) return nullptr;

		return it->second;
	}

	bool iPhysicsJoint::ChangeController(const tString &asName)
	{
		iPhysicsController *pNewCtrl = GetController(asName);
		if(pNewCtrl==nullptr) return false;

		tPhysicsControllerMapIt it = m_mapControllers.begin();
		for(; it != m_mapControllers.end(); ++it)
		{
			iPhysicsController *pCtrl = it->second;

			if(pCtrl == pNewCtrl)
			{
				pCtrl->SetActive(true);
			}
			else
			{
				pCtrl->SetActive(false);
			}
		}

		return true;
	}

	void iPhysicsJoint::SetAllControllersPaused(bool abX)
	{
		tPhysicsControllerMapIt it = m_mapControllers.begin();
		for(; it != m_mapControllers.end(); ++it)
		{
			iPhysicsController *pCtrl = it->second;

			pCtrl->SetPaused(abX);
		}
	}

	cPhysicsControllerIterator iPhysicsJoint::GetControllerIterator()
	{
		return cPhysicsControllerIterator(&m_mapControllers);
	}

	void iPhysicsJoint::OnMaxLimit()
	{
		if(mbHasCollided==false && mpCallback)
		{
			mpCallback->OnMaxLimit(this);
		}

		// Check if any of the controllers has a OnMax end.
		if(mbHasCollided==false)
		{
			tPhysicsControllerMapIt it = m_mapControllers.begin();
			for(; it != m_mapControllers.end(); ++it)
			{
				iPhysicsController *pCtrl = it->second;

				if(pCtrl->IsActive() && pCtrl->GetEndType() == ePhysicsControllerEnd_OnMax)
				{
					pCtrl->SetActive(false);
					iPhysicsController *pNextCtrl = GetController(pCtrl->GetNextController());
					if(pNextCtrl) pNextCtrl->SetActive(true);
					else Warning("Controller '%s' does not exist in joint '%s'\n",pCtrl->GetNextController().c_str(),msName.c_str());
				}
			}
		}

		LimitEffect(&mMaxLimit);
	}

	void iPhysicsJoint::OnMinLimit()
	{
		if(mbHasCollided==false && mpCallback)
		{
			mpCallback->OnMinLimit(this);


		}
		// Check if any of the controllers has a OnMin end.
		if(mbHasCollided==false)
		{

			tPhysicsControllerMapIt it = m_mapControllers.begin();
			for(; it != m_mapControllers.end(); ++it)
			{
				iPhysicsController *pCtrl = it->second;

				if(pCtrl->IsActive() && pCtrl->GetEndType() == ePhysicsControllerEnd_OnMin)
				{
					pCtrl->SetActive(false);
					iPhysicsController *pNextCtrl = GetController(pCtrl->GetNextController());
					if(pNextCtrl) pNextCtrl->SetActive(true);
					else Warning("Controller '%s' does not exist in joint '%s'\n",pCtrl->GetNextController().c_str(),msName.c_str());
				}
			}
		}

		LimitEffect(&mMinLimit);
	}

	void iPhysicsJoint::CalcSoundFreq(float afSpeed,float *apFreq, float *apVol)
	{
		float fAbsSpeed = std::abs(afSpeed);
		float fFreq = 1;
		float fVolume = 1;

		//Higher than middle
		if(fAbsSpeed >= mfMiddleMoveSpeed)
		{
			if(fAbsSpeed >= mfMaxMoveFreqSpeed)
			{
				fFreq = mfMaxMoveFreq;
				fVolume = mfMaxMoveVolume;
			}
			else
			{
				//Calculate how close the speed is to max.
				float fT = (fAbsSpeed-mfMiddleMoveSpeed) /
					(mfMaxMoveFreqSpeed-mfMiddleMoveSpeed);

				fFreq = (1 - fT) + fT * mfMaxMoveFreq;
				fVolume = mfMiddleMoveVolume*(1 - fT) + fT * mfMaxMoveVolume;
			}
		}
		//Below middle
		else
		{
			if(fAbsSpeed <= mfMinMoveFreqSpeed)
			{
				fFreq = mfMinMoveFreq;
				fVolume = mfMinMoveVolume;
			}
			else
			{
				//Calculate how close the speed is to max.
				float fT = (mfMiddleMoveSpeed - fAbsSpeed) /
					(mfMiddleMoveSpeed - mfMinMoveFreqSpeed);

				fFreq = (1 - fT) + fT * mfMinMoveFreq;
				fVolume = mfMiddleMoveVolume*(1 - fT) + fT * mfMinMoveVolume;
			}
		}

		*apFreq = fFreq;
		*apVol = fVolume;
	}

	void iPhysicsJoint::OnPhysicsUpdate()
	{
		//Get the pivot point, if there is no parent, it is stuck.
		if(mpParentBody)
			mvPivotPoint = cMath::MatrixMul(mpChildBody->GetLocalMatrix(),mvLocalPivot);

		cWorld3D *pWorld3D = mpWorld->GetWorld3D();
		if(pWorld3D == nullptr) return;
		if(msMoveSound == "") return;

		if(mpWorld->GetWorld3D()->GetSound()->GetSoundHandler()->GetSilent()) return;

		//Get the speed
		cVector3f vVel(0,0,0);
		//Linear
		if(mMoveSpeedType== ePhysicsJointSpeed_Linear)
		{
			if(mpParentBody)
			{
				vVel = mpChildBody->GetLinearVelocity() - mpParentBody->GetLinearVelocity();
			}
			else
			{
				vVel = mpChildBody->GetLinearVelocity();
			}
		}
		//Angular
		else
		{
			if(mpParentBody)
			{
				vVel = mpChildBody->GetAngularVelocity() - mpParentBody->GetAngularVelocity();
			}
			else
			{
				vVel = mpChildBody->GetAngularVelocity();
			}
		}


		//Check so the body is not still
		if(mpParentBody){
			if(	m_mtxPrevChild == mpChildBody->GetLocalMatrix() &&
				m_mtxPrevParent == mpParentBody->GetLocalMatrix())
			{
				vVel =0;
			}
			m_mtxPrevChild = mpChildBody->GetLocalMatrix();
			m_mtxPrevParent = mpParentBody->GetLocalMatrix();
		}
		else {
			if(m_mtxPrevChild == mpChildBody->GetLocalMatrix())
			{
				vVel =0;
			}
			m_mtxPrevChild = mpChildBody->GetLocalMatrix();
		}

		float fSpeed = vVel.Length();

		if(pWorld3D->SoundEntityExists(mpSound)==false)
		{
			mpSound = nullptr;
		}

		// Create and update sound if speed is high enough
		// Joint has sound
		if(mpSound)
		{
			float fMin = cMath::Max(mfMinMoveSpeed-0.2f, 0.1f);
			if(fSpeed <= fMin)
			{
				mpSound->FadeOut(4.3f);
				mpSound	= nullptr;
			}
			else
			{
				cSoundEntry *pEntry = mpSound->GetSoundEntry(eSoundEntityType_Main);
				if(pEntry)
				{
					float fFreq, fVolume;
					CalcSoundFreq(fSpeed, &fFreq,&fVolume);

					pEntry->mfNormalSpeed  = fFreq;
					pEntry->mfNormalVolumeMul = fVolume;

				}
				else
				{
				}


				mpSound->SetPosition(mvPivotPoint);
			}
		}
		// Joint has no sound
		else
		{
			// Speed is over limit
			if(fSpeed > mfMinMoveSpeed)
			{
				if(mlSpeedCount >= 3)
				{
					mlSpeedCount =0;
					mpSound = pWorld3D->CreateSoundEntity("MoveSound",msMoveSound, true);
					if(mpSound)	{
						mpSound->SetIsSaved(false);
						mpSound->FadeIn(3.3f);
					}
				}
				else
				{
					mlSpeedCount++;
				}
			}
			// Speed is under limit
			else
			{
				mlSpeedCount =0;
			}
		}

	}

	void iPhysicsJoint::LimitEffect(cJointLimitEffect *pEffect)
	{
		cWorld3D *pWorld3D = mpWorld->GetWorld3D();

		if(pWorld3D && pEffect->msSound!="")
		{
			cVector3f vVel(0,0,0);
			if(mpParentBody)
				vVel = mpChildBody->GetLinearVelocity() - mpParentBody->GetLinearVelocity();
			else
				vVel = mpChildBody->GetLinearVelocity();

			float fSpeed = vVel.Length();
			if(fSpeed > pEffect->mfMaxSpeed) fSpeed = pEffect->mfMaxSpeed;

			if(fSpeed >= pEffect->mfMinSpeed && mbHasCollided==false && pEffect->msSound != "")
			{
				float fVolume = (fSpeed - pEffect->mfMinSpeed) / (pEffect->mfMaxSpeed - pEffect->mfMinSpeed);

				cSoundEntity *pSound = pWorld3D->CreateSoundEntity("LimitSound", pEffect->msSound,true);
				if(pSound)
				{
					pSound->SetVolume(fVolume);
					pSound->SetPosition(mpChildBody->GetLocalPosition());
				}
			}
		}

		mbHasCollided = true;
	}

	void iPhysicsJoint::OnNoLimit()
	{
		mbHasCollided = false;
	}

	void iPhysicsJoint::Break()
	{
		mbBroken = true;
		mbBreakable = true;
	}

	bool iPhysicsJoint::CheckBreakage()
	{
		if(mbBreakable==false) return false;

		float fForcesSize = GetForce().Length();

		if(fForcesSize >= mfBreakForce || mbBroken)
		{
			if(msBreakSound != "")
			{
				cWorld3D *pWorld3D = mpWorld->GetWorld3D();
				cSoundEntity *pSound = pWorld3D->CreateSoundEntity("BreakSound", msBreakSound,true);
				if(pSound) pSound->SetPosition(mvPivotPoint);
			}
			return true;
		}

		return false;
	}

	void iPhysicsJoint::CheckLimitAutoSleep(iPhysicsJoint *apJoint,
											const float afMin, const float afMax,
											const float afDist)
	{
		if(apJoint->mbLimitAutoSleep)
		{
			float fMinDiff = std::abs(afMin - afDist);
			float fMaxDiff = std::abs(afMax - afDist);

			if(	fMaxDiff < apJoint->mfLimitAutoSleepDist ||
				fMinDiff < apJoint->mfLimitAutoSleepDist)
			{
				if(apJoint->mlLimitStepCount >= apJoint->mlLimitAutoSleepNumSteps)
					apJoint->mpChildBody->DisableAfterSimulation();
				else
					apJoint->mlLimitStepCount++;
			}
			else
			{
				apJoint->mlLimitStepCount =0;
			}
		}
	}

	kBeginSerializeBase(cJointLimitEffect)
	kSerializeVar(msSound, eSerializeType_String)
	kSerializeVar(mfMinSpeed, eSerializeType_Float32)
	kSerializeVar(mfMaxSpeed, eSerializeType_Float32)
	kEndSerialize()

	kBeginSerializeVirtual(cSaveData_iPhysicsJoint,iSaveData)
	kSerializeVar(msName, eSerializeType_String)

	kSerializeVar(mlParentBodyId, eSerializeType_Int32)
	kSerializeVar(mlChildBodyId, eSerializeType_Int32)

	kSerializeVar(m_mtxParentBodySetup, eSerializeType_Matrixf)
	kSerializeVar(m_mtxChildBodySetup, eSerializeType_Matrixf)

	kSerializeVar(mvPinDir, eSerializeType_Vector3f)
	kSerializeVar(mvStartPivotPoint, eSerializeType_Vector3f)

	kSerializeClassContainer(mlstControllers, cSaveData_iPhysicsController, eSerializeType_Class)

	kSerializeVar(mMaxLimit, eSerializeType_Class)
	kSerializeVar(mMinLimit, eSerializeType_Class)

	kSerializeVar(msMoveSound, eSerializeType_String)

	kSerializeVar(mfMinMoveSpeed, eSerializeType_Float32)
	kSerializeVar(mfMinMoveFreq, eSerializeType_Float32)
	kSerializeVar(mfMinMoveFreqSpeed, eSerializeType_Float32)
	kSerializeVar(mfMinMoveVolume, eSerializeType_Float32)
	kSerializeVar(mfMaxMoveFreq, eSerializeType_Float32)
	kSerializeVar(mfMaxMoveFreqSpeed, eSerializeType_Float32)
	kSerializeVar(mfMaxMoveVolume, eSerializeType_Float32)
	kSerializeVar(mfMiddleMoveSpeed, eSerializeType_Float32)
	kSerializeVar(mfMiddleMoveVolume, eSerializeType_Float32)
	kSerializeVar(mMoveSpeedType, eSerializeType_Int32)

	kSerializeVar(mbBreakable, eSerializeType_Bool)
	kSerializeVar(mfBreakForce, eSerializeType_Float32)
	kSerializeVar(msBreakSound, eSerializeType_String)
	kSerializeVar(mbBroken, eSerializeType_Bool)

	kSerializeVar(msCallbackMaxFunc, eSerializeType_String)
	kSerializeVar(msCallbackMinFunc, eSerializeType_String)

	kSerializeVar(mbAutoDeleteCallback, eSerializeType_Bool)
	kEndSerialize()

	iSaveData* iPhysicsJoint::CreateSaveData()
	{
		return nullptr;
	}

	void iPhysicsJoint::SaveToSaveData(iSaveData *apSaveData)
	{
		kSaveData_SaveToBegin(iPhysicsJoint);

		//Variables
		kSaveData_SaveTo(msName);

		kSaveData_SaveTo(m_mtxParentBodySetup);
		kSaveData_SaveTo(m_mtxChildBodySetup);

		kSaveData_SaveTo(mvPinDir);
		kSaveData_SaveTo(mvStartPivotPoint);

		kSaveData_SaveTo(mMaxLimit);
		kSaveData_SaveTo(mMinLimit);
		kSaveData_SaveTo(msMoveSound);
		kSaveData_SaveTo(mfMinMoveSpeed);
		kSaveData_SaveTo(mfMinMoveFreq);
		kSaveData_SaveTo(mfMinMoveFreqSpeed);
		kSaveData_SaveTo(mfMinMoveVolume);
		kSaveData_SaveTo(mfMaxMoveFreq);
		kSaveData_SaveTo(mfMaxMoveFreqSpeed);
		kSaveData_SaveTo(mfMaxMoveVolume);
		kSaveData_SaveTo(mfMiddleMoveSpeed);
		kSaveData_SaveTo(mfMiddleMoveVolume);
		kSaveData_SaveTo(mMoveSpeedType);
		kSaveData_SaveTo(mbBreakable);
		kSaveData_SaveTo(mfBreakForce);
		kSaveData_SaveTo(msBreakSound);
		kSaveData_SaveTo(mbBroken);

		kSaveData_SaveTo(mbAutoDeleteCallback);

		//Callback
		if(mpCallback && mpCallback->IsScript())
		{
			cScriptJointCallback *pScriptCallback = static_cast<cScriptJointCallback*>(mpCallback);
			pData->msCallbackMaxFunc = pScriptCallback->msMaxFunc;
			pData->msCallbackMinFunc = pScriptCallback->msMinFunc;
		}
		else
		{
			pData->msCallbackMaxFunc = "";
			pData->msCallbackMinFunc = "";
		}

		//Controllers
		pData->mlstControllers.Clear();
		tPhysicsControllerMapIt it = m_mapControllers.begin();
		for(; it != m_mapControllers.end(); ++it)
		{
			iPhysicsController *pController = it->second;
			cSaveData_iPhysicsController saveController;
			pController->SaveToSaveData(&saveController);

			pData->mlstControllers.Add(saveController);
		}

		//Pointers
		kSaveData_SaveObject(mpParentBody,mlParentBodyId);
		kSaveData_SaveObject(mpChildBody,mlChildBodyId);
	}

	void iPhysicsJoint::LoadFromSaveData(iSaveData *apSaveData)
	{
		kSaveData_LoadFromBegin(iPhysicsJoint);

		//Variables
		kSaveData_LoadFrom(msName);

		kSaveData_LoadFrom(m_mtxParentBodySetup);
		kSaveData_LoadFrom(m_mtxChildBodySetup);

		kSaveData_LoadFrom(mvPinDir);
		kSaveData_LoadFrom(mvStartPivotPoint);

		kSaveData_LoadFrom(mMaxLimit);
		kSaveData_LoadFrom(mMinLimit);
		kSaveData_LoadFrom(msMoveSound);
		kSaveData_LoadFrom(mfMinMoveSpeed);
		kSaveData_LoadFrom(mfMinMoveFreq);
		kSaveData_LoadFrom(mfMinMoveFreqSpeed);
		kSaveData_LoadFrom(mfMinMoveVolume);
		kSaveData_LoadFrom(mfMaxMoveFreq);
		kSaveData_LoadFrom(mfMaxMoveFreqSpeed);
		kSaveData_LoadFrom(mfMaxMoveVolume);
		kSaveData_LoadFrom(mfMiddleMoveSpeed);
		kSaveData_LoadFrom(mfMiddleMoveVolume);
		kSaveData_LoadFrom(mbBreakable);
		kSaveData_LoadFrom(mfBreakForce);
		kSaveData_LoadFrom(msBreakSound);
		kSaveData_LoadFrom(mbBroken);
		mMoveSpeedType = (ePhysicsJointSpeed)pData->mMoveSpeedType;

		kSaveData_LoadFrom(mbAutoDeleteCallback);

		//Controllers
		for(cSaveData_iPhysicsController &saveCtrl : pData->mlstControllers.GetIterator())
		{
			iPhysicsController *pController = mpWorld->CreateController(saveCtrl.msName);
			pController->LoadFromSaveData(&saveCtrl);

			AddController(pController);
		}
	}

	void iPhysicsJoint::SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame)
	{
		kSaveData_SetupBegin(iPhysicsJoint);

		if(pData->msCallbackMaxFunc != "" || pData->msCallbackMinFunc != "")
		{
			cScriptJointCallback *pCallback = new cScriptJointCallback(apGame->GetScene());
			pCallback->msMaxFunc =pData->msCallbackMaxFunc;
			pCallback->msMinFunc =pData->msCallbackMinFunc;
			SetCallback(pCallback, true);
		}
	}
}
