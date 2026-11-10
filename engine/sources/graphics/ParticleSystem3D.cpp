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
#include "graphics/ParticleSystem3D.h"

#include "tinyxml.h"

#include "graphics/ParticleEmitter3D_UserData.h"

#include "resources/ParticleManager.h"

#include "game/Game.h"

#include "scene/Scene.h"
#include "scene/World3D.h"

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CREATOR
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cParticleSystemData3D::cParticleSystemData3D(const tString &asName,
												cResources* apResources,cGraphics *apGraphics)
												: iResourceBase(asName,0)
	{
		mpResources = apResources;
		mpGraphics = apGraphics;
		mpOriginal = nullptr;
	}

	//-----------------------------------------------------------------------

	cParticleSystemData3D::~cParticleSystemData3D()
	{
		STLDeleteAll(mvEmitterData);
		if(mpOriginal) hplDelete(mpOriginal);
	}

	//-----------------------------------------------------------------------

	void cParticleSystemData3D::SetOriginal(cParticleSystemData3D *apData)
	{
		if(mpOriginal) hplDelete(mpOriginal);
		mpOriginal = apData;
	}

	//-----------------------------------------------------------------------

	bool cParticleSystemData3D::UsesReplacedMaterial() const
	{
		for(iParticleEmitterData *pData : mvEmitterData)
			if(pData->UsesReplacedMaterial()) return true;
		return false;
	}

	//-----------------------------------------------------------------------

	void cParticleSystemData3D::CreateEmitters(cParticleSystem3D *apPS, const tString &asName,
												const cVector3f &avSize)
	{
		for(size_t i=0; i<mvEmitterData.size(); ++i)
		{
			iParticleEmitter3D *pPE = static_cast<iParticleEmitter3D*>(mvEmitterData[i]->Create(asName, avSize));
			apPS->AddEmitter(pPE);
			pPE->SetSystem(apPS);
		}
	}

	//-----------------------------------------------------------------------

	void cParticleSystemData3D::AddEmitterData(iParticleEmitterData *apData)
	{
		mvEmitterData.push_back(apData);
	}

	//-----------------------------------------------------------------------

	cParticleSystem3D* cParticleSystemData3D::Create(tString asName, cVector3f avSize,
													const cMatrixf& a_mtxTransform, bool abOriginal)
	{
		cParticleSystemData3D *pEmitterData = abOriginal && mpOriginal ? mpOriginal : this;
		if(pEmitterData->mvEmitterData.empty())
		{
			Warning("Particle system '%s' has no emitters.\n",msName.c_str());
			return nullptr;
		}

		cParticleSystem3D *pPS = hplNew( cParticleSystem3D, (asName,this,mpResources,mpGraphics) );
		pPS->SetMatrix(a_mtxTransform);
		pPS->SetOriginal(pEmitterData != this);

		pEmitterData->CreateEmitters(pPS, asName, avSize);

		return pPS;
	}

	//-----------------------------------------------------------------------

	bool cParticleSystemData3D::LoadFromFile(const tString &asFile, bool abOriginal)
	{
		TiXmlDocument* pXmlDoc = hplNew( TiXmlDocument,(asFile.c_str()) );
		if(pXmlDoc->LoadFile()==false)
		{
			Warning("Couldn't open XML file %s\n",asFile.c_str());
			hplDelete(pXmlDoc);
			return false;
		}

		TiXmlElement *pRootElem = pXmlDoc->RootElement();

		TiXmlElement *pEmitterElem = pRootElem->FirstChildElement("ParticleEmitter");
		for(; pEmitterElem != nullptr; pEmitterElem = pEmitterElem->NextSiblingElement("ParticleEmitter"))
		{
			cParticleEmitterData3D_UserData *pPE = hplNew( cParticleEmitterData3D_UserData,("",
																	mpResources,mpGraphics) );

			pPE->SetOriginal(abOriginal);
			pPE->LoadFromElement(pEmitterElem);

			mvEmitterData.push_back(pPE);
		}


		hplDelete(pXmlDoc);
		return true;
	}

	//-----------------------------------------------------------------------


	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cParticleSystem3D::cParticleSystem3D(const tString asName,
										cParticleSystemData3D *apData,
										cResources *apResources, cGraphics *apGraphics)
		: iEntity3D(asName)
	{
		mpResources = apResources;
		mpGraphics = apGraphics;
		mpParticleManager = nullptr;
		mpData = apData;

		mbFirstUpdate = true;
		mbOriginal = false;
	}

	//-----------------------------------------------------------------------

	cParticleSystem3D::~cParticleSystem3D()
	{
		for(size_t i=0; i< mvEmitters.size(); ++i)
		{
			hplDelete(mvEmitters[i]);
		}
		if(mpParticleManager) mpParticleManager->Destroy(mpData);
	}

	//-----------------------------------------------------------------------

	bool cParticleSystem3D::HasOriginal()
	{
		return mpData->GetOriginal() != nullptr;
	}

	cParticleSystemData3D *cParticleSystem3D::GetEmitterData()
	{
		return mbOriginal && mpData->GetOriginal() ? mpData->GetOriginal() : mpData;
	}

	//-----------------------------------------------------------------------

	void cParticleSystem3D::DeleteEmitters()
	{
		for(iParticleEmitter3D *pPE : mvEmitters)
		{
			RemoveChild(pPE);
			hplDelete(pPE);
		}
		mvEmitters.clear();
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void cParticleSystem3D::SetVisible(bool abVisible)
	{
		if(mbIsVisible == abVisible) return;

		mbIsVisible = abVisible;

		for(size_t i=0; i< mvEmitters.size(); ++i)
		{
			mvEmitters[i]->SetVisible(mbIsVisible);
		}

	}

	//-----------------------------------------------------------------------

	bool cParticleSystem3D::IsDead()
	{
		size_t lCount =0;

		for(size_t i=0; i< mvEmitters.size(); ++i)
		{
			iParticleEmitter3D *pPE = mvEmitters[i];

			if(pPE->IsDead()) lCount++;
		}

		if(lCount == mvEmitters.size()) return true;

		return false;
	}

	bool cParticleSystem3D::IsDying()
	{
		size_t lCount =0;

		for(size_t i=0; i< mvEmitters.size(); ++i)
		{
			iParticleEmitter3D *pPE = mvEmitters[i];

			if(pPE->IsDying()) lCount++;
		}

		if(lCount == mvEmitters.size()) return true;

		return false;
	}

	//-----------------------------------------------------------------------

	void cParticleSystem3D::Kill()
	{
		SetIsSaved(false);
		for(size_t i=0; i< mvEmitters.size(); ++i)
		{
			iParticleEmitter3D *pPE = mvEmitters[i];

			pPE->Kill();
		}
	}

	//-----------------------------------------------------------------------

	void cParticleSystem3D::KillInstantly()
	{
		SetIsSaved(false);
		for(size_t i=0; i< mvEmitters.size(); ++i)
		{
			iParticleEmitter3D *pPE = mvEmitters[i];

			pPE->KillInstantly();
		}
	}

	//-----------------------------------------------------------------------

	void cParticleSystem3D::UpdateLogic(float afTimeStep)
	{
		if(IsActive()==false) return;

		for(size_t i=0; i< mvEmitters.size(); ++i)
		{
			iParticleEmitter3D *pPE = mvEmitters[i];

			//////////////////////////
			//Warm Up, every emitter (the original only did the first)
			if(mbFirstUpdate && pPE->IsActive())
			{
				iParticleEmitterData *pData =  GetEmitterData()->GetEmitterData((int)i);
				pPE->WarmUp(pData->GetWarmUpTime(), pData->GetWarmUpStepsPerSec());
			}

			//////////////////////////
			//Update
			pPE->UpdateLogic(afTimeStep);
		}
		mbFirstUpdate = false;
	}

	//-----------------------------------------------------------------------

	void cParticleSystem3D::AddEmitter(iParticleEmitter3D* apEmitter)
	{
		mvEmitters.push_back(apEmitter);

		AddChild(apEmitter);
	}

	//-----------------------------------------------------------------------

	iParticleEmitter3D* cParticleSystem3D::GetEmitter(int alIdx)
	{
		return mvEmitters[alIdx];
	}

	//-----------------------------------------------------------------------

	int cParticleSystem3D::GetEmitterNum()
	{
		return (int)mvEmitters.size();
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// SAVE OBJECT STUFF
	//////////////////////////////////////////////////////////////////////////

	kBeginSerializeBase(cSaveData_ParticleEmitter3D)
		kSerializeVar(mbActive, eSerializeType_Bool)
		kSerializeVar(mbDying, eSerializeType_Bool)
	kEndSerialize()

	//-----------------------------------------------------------------------

	kBeginSerialize(cSaveData_cParticleSystem3D,cSaveData_iEntity3D)
		kSerializeVar(msDataName, eSerializeType_String)
		kSerializeVar(mvDataSize, eSerializeType_Vector3f)

		kSerializeClassContainer(mvEmitters,cSaveData_ParticleEmitter3D,eSerializeType_Class)
	kEndSerialize()

	//-----------------------------------------------------------------------

	iSaveObject* cSaveData_cParticleSystem3D::CreateSaveObject(cSaveObjectHandler *apSaveObjectHandler,cGame *apGame)
	{
		cWorld3D *pWorld = apGame->GetScene()->GetWorld3D();

		cParticleSystem3D *pPS = pWorld->CreateParticleSystem(msName,msDataName,mvDataSize,
															m_mtxLocalTransform);

		return pPS;
	}

	//-----------------------------------------------------------------------

	int cSaveData_cParticleSystem3D::GetSaveCreatePrio()
	{
		return 3;
	}

	//-----------------------------------------------------------------------

	iSaveData* cParticleSystem3D::CreateSaveData()
	{
		return hplNew( cSaveData_cParticleSystem3D, () );
	}

	//-----------------------------------------------------------------------

	void cParticleSystem3D::SaveToSaveData(iSaveData *apSaveData)
	{
		kSaveData_SaveToBegin(cParticleSystem3D);

		kSaveData_SaveTo(msDataName);
		kSaveData_SaveTo(mvDataSize);

		pData->mvEmitters.Resize(GetEmitterNum());
		for(int i=0; i< GetEmitterNum(); ++i)
		{
			iParticleEmitter3D *pPE = GetEmitter(i);

			pData->mvEmitters[i].mbDying = pPE->IsDying();
			pData->mvEmitters[i].mbActive = pPE->IsActive();
		}
	}

	//-----------------------------------------------------------------------

	void cParticleSystem3D::LoadFromSaveData(iSaveData *apSaveData)
	{
		kSaveData_LoadFromBegin(cParticleSystem3D);

		kSaveData_LoadFrom(msDataName);
		kSaveData_LoadFrom(mvDataSize);

		if(GetEmitterNum() != pData->mvEmitters.Size())
		{
			Error("Saved emitter number in %s/%d does not match loaded, killing system!\n",
																			GetName().c_str(),
																			GetSaveObjectId());

			for(int i=0; i<GetEmitterNum();++i)
			{
				GetEmitter(i)->KillInstantly();
			}
			return;
		}

		for(int i=0; i< GetEmitterNum(); ++i)
		{
			iParticleEmitter3D *pPE = GetEmitter(i);

			pPE->SetActive(pData->mvEmitters[i].mbActive);

			if(pData->mvEmitters[i].mbDying) pPE->KillInstantly();
		}
	}

	//-----------------------------------------------------------------------

	void cParticleSystem3D::SaveDataSetup(cSaveObjectHandler *apSaveObjectHandler, cGame *apGame)
	{
		kSaveData_SetupBegin(cParticleSystem3D);
	}

	//-----------------------------------------------------------------------
}
