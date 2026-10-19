/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of Penumbra Overture.
 *
 * Penumbra Overture is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Penumbra Overture is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Penumbra Overture.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "core/MapHandler.h"

#include <SDL3/SDL.h>

#include "core/Init.h"
#include "ui/GameGui.h"
#include "systems/GameSoundEnvHandler.h"
#include "player/Player.h"
#include "player/PlayerHelper.h"
#include "player/PlayerHands.h"
#include "entities/GameEntity.h"
#include "enemies/GameEnemy.h"
#include "entities/GameArea.h"
#include "entities/GameLink.h"
#include "entities/GameItem.h"
#include "entities/GameLadder.h"
#include "entities/GameStickArea.h"
#include "entities/GameDamageArea.h"
#include "entities/GameForceArea.h"
#include "entities/GameSaveArea.h"
#include "entities/GameLiquidArea.h"
#include "systems/FadeHandler.h"
#include "core/SaveHandler.h"
#include "systems/TriggerHandler.h"
#include "systems/Triggers.h"
#include "core/GraphicsHelper.h"
#include "core/SaveTypes.h"
#include "systems/GameMusicHandler.h"
#include "systems/GameMessageHandler.h"
#include "systems/RadioHandler.h"
#include "entities/GameVentArea.h"
#include "ui/Inventory.h"
#include "ui/MapLoadText.h"

//////////////////////////////////////////////////////////////////////////
// WORLD CACHE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cWorldCache::cWorldCache(cInit *apInit)
{
	mpInit = apInit;
	mpResources = mpInit->mpGame->GetResources();

	mlCount =0;
}

cWorldCache::~cWorldCache()
{
	DecResources();
}

//-----------------------------------------------------------------------

void cWorldCache::AddResources()
{
	////////////////////////////
	// Materials
	{
		cResourceBaseIterator it = mpResources->GetMaterialManager()->GetResourceBaseIterator();
		while(it.HasNext())
		{
			iResourceBase *pResource = it.Next();
			pResource->IncUserCount();
			mlstMaterials.push_back(pResource);
		}
	}
	////////////////////////////
	// Meshes
	{
		cResourceBaseIterator it = mpResources->GetMeshManager()->GetResourceBaseIterator();
		while(it.HasNext())
		{
			iResourceBase *pResource = it.Next();
			pResource->IncUserCount();
			mlstMeshes.push_back(pResource);
		}
	}
	////////////////////////////
	// Animations
	{
		cResourceBaseIterator it = mpResources->GetAnimationManager()->GetResourceBaseIterator();
		while(it.HasNext())
		{
			iResourceBase *pResource = it.Next();
			pResource->IncUserCount();
			mlstAnimations.push_back(pResource);
		}
	}

	//mlCount++;
}

//-----------------------------------------------------------------------

void cWorldCache::DecResources()
{
	////////////////////////////
	// Materials
	{
		tResourceBaseListIt it = mlstMaterials.begin();
		for(; it != mlstMaterials.end(); ++it)
		{
			iResourceBase *pResource = *it;
			mpResources->GetMaterialManager()->Destroy(pResource);
		}
		mlstMaterials.clear();
	}
	////////////////////////////
	// Meshes
	{
		tResourceBaseListIt it = mlstMeshes.begin();
		for(; it != mlstMeshes.end(); ++it)
		{
			iResourceBase *pResource = *it;
			mpResources->GetMeshManager()->Destroy(pResource);
		}
		mlstMeshes.clear();
	}
	////////////////////////////
	// Animations
	{
		tResourceBaseListIt it = mlstAnimations.begin();
		for(; it != mlstAnimations.end(); ++it)
		{
			iResourceBase *pResource = *it;
			mpResources->GetAnimationManager()->Destroy(pResource);
		}
		mlstAnimations.clear();
	}

	//mlCount--;
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// SOUND CALLBACK
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cMapHandlerSoundCallback::cMapHandlerSoundCallback(cInit *apInit)
{
	mpInit = apInit;
	
	///////////////////////////////////////////
	//Load all sounds that can heard by enemies
	tString sFile = "sounds/EnemySounds.dat";
	TiXmlDocument* pXmlDoc = hplNew( TiXmlDocument, (sFile.c_str()) );
	if(pXmlDoc->LoadFile()==false){
		Error("Couldn't load XML file '%s'!\n",sFile.c_str());
		hplDelete( pXmlDoc );	return;
	}

	//Get the root.
	TiXmlElement* pRootElem = pXmlDoc->RootElement();

	TiXmlElement* pChildElem = pRootElem->FirstChildElement();
	for(; pChildElem != nullptr; pChildElem = pChildElem->NextSiblingElement())
	{
		tString sName = cString::ToString(pChildElem->Attribute("name"),"");
		mvEnemyHearableSounds.push_back(sName);
	}

	hplDelete( pXmlDoc );
}

//-----------------------------------------------------------------------

void cMapHandlerSoundCallback::OnStart(cSoundEntity *apSoundEntity)
{
	///////////////////////////
	//Check if the sound is something to worry bout
	tString sTypeName = apSoundEntity->GetData()->GetName();
	
	bool bUsed=false;
	for(size_t i=0; i< mvEnemyHearableSounds.size(); ++i)
	{
		tString &sName = mvEnemyHearableSounds[i];
		if(sTypeName.size() >= sName.size() && sName == sTypeName.substr(0,sName.size()))
		{
			bUsed = true;
		}
	}
	if(bUsed == false) return;

	//Add a sound trigger
	cGameTrigger_Sound *pSound = hplNew( cGameTrigger_Sound, () );
	pSound->mpSound = apSoundEntity->GetData();
	mpInit->mpTriggerHandler->Add(pSound, eGameTriggerType_Sound,
									apSoundEntity->GetWorldPosition(),
									10, 1.0f/60.0f, apSoundEntity->GetData()->GetMaxDistance());
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// LIGHT FLASH
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cEffectLightFlash::cEffectLightFlash(cInit *apInit,const cVector3f& avPos,
									float afRadius, const cColor &aColor, 
									float afFadeInSeconds, float afFadeOutSeconds)
{
	mpInit = apInit;

	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();

	mpLight = pWorld->CreateLightPoint("Flash");
	mpLight->SetFarAttenuation(afRadius);
	mpLight->SetDiffuseColor(cColor(0,0));
	mpLight->SetCastShadows(true);
	mpLight->SetIsSaved(false);
	mpLight->SetPosition(avPos);

	mfRadius = afRadius;
	mfFadeOutSeconds = afFadeOutSeconds;
	
	//Log("Fade to %s : %f in time %f\n",aColor.ToString().c_str(),mfRadius,afFadeInSeconds);
	mpLight->FadeTo(aColor,mfRadius,afFadeInSeconds);

	mbIsDying = false;
	mbDead = false;
}

cEffectLightFlash::~cEffectLightFlash()
{
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	
	pWorld->DestroyLight(mpLight);
}

//-----------------------------------------------------------------------

void cEffectLightFlash::Update(float afTimeStep)
{
	cColor Col = mpLight->GetDiffuseColor();
	//Log("Update...\n");
	if(mbIsDying==false)
	{
		if(mpLight->IsFading()==false)
		{
			//Log("Fade to black\n");
			mpLight->FadeTo(cColor(0,0),mfRadius,mfFadeOutSeconds);
			mbIsDying = true;
		}
	}
	else
	{
		if(mpLight->IsFading()==false)
		{
			//Log("Kill\n");
			mbDead = true;
		}
	}
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cMapHandler::cMapHandler(cInit *apInit) : iUpdateable("MapHandler")
{
	mpInit = apInit;

	mbPreUpdating = false;

	mpScene = apInit->mpGame->GetScene();
	mpResources = apInit->mpGame->GetResources();

	mpWorldCache = hplNew( cWorldCache, (apInit) );

	mfGameTime =0;

	mbDestroyingAll = false;

	Reset();

	mpSoundCallback = hplNew( cMapHandlerSoundCallback, (apInit) );
	cSoundEntity::AddGlobalCallback(mpSoundCallback);

	mpMapChangeTexture = mpInit->mpGame->GetResources()->GetTextureManager()->Create2D("other_mapchange.jpg",false);

	cGpuProgramManager *pProgramManager = mpResources->GetGpuProgramManager();
	mpFlashProgram = pProgramManager->CreateProgram("Ambient_Color", "", "Diffuse_Color");
}

//-----------------------------------------------------------------------

cMapHandler::~cMapHandler(void)
{
	if(mpMapChangeTexture)mpInit->mpGame->GetResources()->GetTextureManager()->Destroy(mpMapChangeTexture);
	if(mpFlashProgram) mpResources->GetGpuProgramManager()->Destroy(mpFlashProgram);
	
	hplDelete(mpSoundCallback );
	hplDelete( mpWorldCache );
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////


//-----------------------------------------------------------------------

#ifdef DEMO_VERSION
	static int glNumOfLoads = 0;
#endif

bool cMapHandler::Load(const tString &asFile,const tString& asStartPos)
{
	tString sMapName = cString::ToLowerCase(cString::SetFileExt(asFile,""));
	cWorld3D *pWorld=nullptr;
	cWorld3D *pLastMap = nullptr;
	bool bFirstTime = false;
	double fTimeSinceVisit=0;

#ifdef DEMO_VERSION
	glNumOfLoads++;
	if(glNumOfLoads > 5){
		CreateMessageBoxW(	_W("Demo not playable any more!\n"),
							_W("The limits of the demo have been exceeded!\n"));
		exit(0);
	}
#endif
	
	unsigned long lStartTime = mpInit->mpGame->GetSystem()->GetLowLevel()->GetTime();

	if(sMapName != msCurrentMap)
	{
		////////////////////////////////////////
		// DESTRUCTION OF STUFF NOT SAVED //////

		//remove all local timer
		//RemoveLocalTimers();

		//Exit script
		if(mpScene->GetWorld3D())
		{
			//OnMapLoad for all entities
			tGameEntityMapIt GIt = m_mapGameEntities.begin();
			for(; GIt != m_mapGameEntities.end(); ++GIt)
			{
				iGameEntity *pEntity = GIt->second;

				pEntity->OnWorldExit();
			}

			if(mpScene->GetWorld3D()->GetScript())
				mpScene->GetWorld3D()->GetScript()->Run("OnExit()");
			
			pLastMap = mpScene->GetWorld3D();
		}

		mpInit->mpMusicHandler->OnWorldExit();
		//Destroy the player objects so they are no saved.
		mpInit->mpPlayer->OnWorldExit();
		mpInit->mpPlayerHands->OnWorldExit();
		mpInit->mpGameMessageHandler->OnWorldExit();
		mpInit->mpRadioHandler->OnWorldExit();
		mpInit->mpSoundEnvHandler->OnWorldExit();

		////////////////////////////////////////
		// SAVING /////////////////////////////

		//Save the map
		if(msCurrentMap != "" && mpScene->GetWorld3D() != nullptr)
		{
			mpInit->mpSaveHandler->SaveData(msCurrentMap);
		}
		
		msCurrentMap = sMapName;

		////////////////////////////////////////
		// DESTRUCTION OF SAVED STUFF //////////

		mpInit->mpInventory->ClearCallbacks();

		//Reset the rendering.
		mpInit->mpGame->GetGraphics()->GetRenderer3D()->SetAmbientColor(cColor(0,1));
		mpInit->mpGame->GetGraphics()->GetRenderer3D()->SetSkyBoxActive(false);
		mpInit->mpGame->GetGraphics()->GetRenderer3D()->SetFogActive(false);

		//Remove all current objects
		mpInit->mbDestroyGraphics =false;
		DestroyAll();
		mpInit->mbDestroyGraphics =true;
		
		//Destroy all sound entities on previous map.
		if(mpScene->GetWorld3D())
		{
			mpScene->GetWorld3D()->DestroyAllSoundEntities();
		}
		
		mpInit->mpPlayer->ClearCollideScripts();

		//Stop all sound
		mpInit->mpGame->GetSound()->GetSoundHandler()->StopAll(eSoundDest_World);
		mpInit->mpGame->GetSound()->Update(1.0f/60.0f);

		//Destroy Haptic shapes
		if(mpInit->mbHasHaptics)
			mpInit->mpGame->GetHaptic()->GetLowLevel()->DestroyAllShapes();
		
		////////////////////////////////////////
		// LOAD THE MAP ////////////////////////

		//Load 
		if(mpScene->HasLoadedWorld(asFile))
		{
			//Log("-------- Loaded NOT first time! ----------------\n");
			pWorld = mpScene->LoadWorld3D(asFile,true,	eWorldLoadFlag_NoGameEntities);
														//eWorldLoadFlag_NoLights | 
														//eWorldLoadFlag_NoEntities |
			
			//eWorldLoadFlag_NoGameEntities);
			if(pWorld)
				mpScene->SetWorld3D(pWorld);
			else {
				Error("Couldn't load map '%s'\n",asFile.c_str());	
				return false;
			}
			mpInit->mpSaveHandler->LoadData(msCurrentMap);
			mpInit->mpGame->GetResources()->GetSoundManager()->DestroyUnused(mpInit->mlMaxSoundDataNum);
			mpInit->mpGame->GetResources()->GetParticleManager()->DestroyUnused(mpInit->mlMaxPSDataNum);
		}
		else
		{
			//Log("-------- FIRST TIME first time! ----------------\n");
			pWorld = mpScene->LoadWorld3D(asFile,true,0);
			mpInit->mpGame->GetResources()->GetSoundManager()->DestroyUnused(mpInit->mlMaxSoundDataNum);
			mpInit->mpGame->GetResources()->GetParticleManager()->DestroyUnused(mpInit->mlMaxPSDataNum);
			
			if(pWorld)
				mpScene->SetWorld3D(pWorld);
			else {
				Error("Couldn't load map '%s'\n",asFile.c_str());	
				return false;
			}
			bFirstTime = true;
		}

		///////////////////////////
		//Add to cache
		mpWorldCache->DecResources();
        mpWorldCache->AddResources();
		
		///////////////////////////
		//Destroy old map
		if(pLastMap)
		{
			mpScene->DestroyWorld3D(pLastMap);
		}

		fTimeSinceVisit = AddLoadedMap(pWorld);

		pWorld->GetPhysicsWorld()->SetMaxTimeStep(mpInit->mfMaxPhysicsTimeStep);

		//OnMapLoad for all entities
		tGameEntityMapIt GIt = m_mapGameEntities.begin();
		for(; GIt != m_mapGameEntities.end(); ++GIt)
		{
			iGameEntity *pEntity = GIt->second;

			pEntity->OnWorldLoad();
		}

		//OnMapLoad for player
		mpInit->mpPlayer->OnWorldLoad();

		mpInit->mpPlayerHands->OnWorldLoad();

		mpInit->mpMusicHandler->OnWorldLoad();
		mpInit->mpSoundEnvHandler->OnWorldLoad();

		if(bFirstTime)
		{
			LiftBodiesOutOfFloors(pWorld);

			//Set a default name
			msMapGameName = cString::To16Char(cString::SetFileExt(asFile,""));

			//Init script
			if(pWorld->GetScript()) pWorld->GetScript()->Run("OnStart()");
		}
	}
	else
	{
		if(mpScene->GetWorld3D() != nullptr)
		{
			pWorld = mpScene->GetWorld3D();
		}
		else
		{
			Error("No world has been loaded!\n");
			return false;
		}
	}

	mpInit->mpPlayer->SetStartPos(asStartPos);

	//Run global script
	if(mpInit->mpGlobalScript)
	{
		if(bFirstTime) mpInit->mpGlobalScript->Run("OnMapStart()");
		mpInit->mpGlobalScript->Run("OnMapLoad()");
	}
	if(pWorld->GetScript())
	{
		pWorld->GetScript()->Run("OnLoad()");
	}

	
	//After script has been run callback
	tGameEntityMapIt GIt = m_mapGameEntities.begin();
	for(; GIt != m_mapGameEntities.end(); ++GIt)
	{
		iGameEntity *pEntity = GIt->second;

		pEntity->OnPostLoadScripts();
	}

	//Log("After load and before preupdate:\n");

	unsigned long lTime = mpInit->mpGame->GetSystem()->GetLowLevel()->GetTime() - lStartTime;
	Log("Loading map '%s' took: %d ms\n",pWorld->GetFileName().c_str(),lTime);
	
	PreUpdate(fTimeSinceVisit);

	mpInit->mpGame->ResetLogicTimer();

	//mpInit->mpGame->GetSound()->GetSoundHandler()->ResumeAll(eSoundDest_World | eSoundDest_Gui);
	
	//Log("After load and preupdate:\n");

	//Set physics accuracy
	pWorld->GetPhysicsWorld()->SetAccuracyLevel(mpInit->mPhysicsAccuracy);
	
	return true;
}

//-----------------------------------------------------------------------

bool cMapHandler::LoadSimple(const tString &asFile, bool abLoadEntities)
{	
	tString sMapName = cString::ToLowerCase(cString::SetFileExt(asFile,""));
	cWorld3D *pWorld=nullptr;

    DestroyAll();
	mpInit->mpGame->GetSound()->GetSoundHandler()->StopAll(eSoundDest_World);
	mpInit->mpGame->GetSound()->Update(1.0f/60.0f);

    mpInit->mpGame->GetGraphics()->GetRenderer3D()->SetAmbientColor(cColor(0,1)); 
	
	cWorld3D *pOldWorld = mpScene->GetWorld3D();

	//Haptic
	if(mpInit->mbHasHaptics)
		mpInit->mpGame->GetHaptic()->GetLowLevel()->DestroyAllShapes();

	//Delete all sound entities
	if(pOldWorld)
	{
		pOldWorld->DestroyAllSoundEntities();
	}
	
	//Set the current map.
	msCurrentMap = sMapName;

	if(abLoadEntities==false)
	{
		pWorld = mpScene->LoadWorld3D(asFile,true,	eWorldLoadFlag_NoGameEntities);
													//eWorldLoadFlag_NoLights | 
													//eWorldLoadFlag_NoEntities |
													//eWorldLoadFlag_NoGameEntities);
		if(pWorld==nullptr){
			Error("Couldn't load map '%s'\n",asFile.c_str());	
			return false;
		}
		mpInit->mpSaveHandler->LoadData(msCurrentMap);
		mpInit->mpGame->GetResources()->GetSoundManager()->DestroyUnused(mpInit->mlMaxSoundDataNum);
		mpInit->mpGame->GetResources()->GetParticleManager()->DestroyUnused(mpInit->mlMaxPSDataNum);
	}
	else
	{
		pWorld = mpScene->LoadWorld3D(asFile,true,0);
		mpInit->mpGame->GetResources()->GetSoundManager()->DestroyUnused(mpInit->mlMaxSoundDataNum);
		mpInit->mpGame->GetResources()->GetParticleManager()->DestroyUnused(mpInit->mlMaxPSDataNum);

		if(pWorld==nullptr){
			Error("Couldn't load map '%s'\n",asFile.c_str());	
			return false;
		}
	}

	//Destroy old world, if there is any.
	if(pOldWorld)
	{
		mpScene->DestroyWorld3D(pOldWorld);
	}

		
	//Set physics update
	if(pWorld->GetPhysicsWorld())pWorld->GetPhysicsWorld()->SetMaxTimeStep(mpInit->mfMaxPhysicsTimeStep);

	//OnMapLoad for all entities
	tGameEntityMapIt GIt = m_mapGameEntities.begin();
	for(; GIt != m_mapGameEntities.end(); ++GIt)
	{
		iGameEntity *pEntity = GIt->second;

		pEntity->OnWorldLoad();
	}

	//OnMapLoad for player
	mpInit->mpPlayer->OnWorldLoad();

	mpInit->mpGame->ResetLogicTimer();
	
	//Physics set up
	pWorld->GetPhysicsWorld()->SetAccuracyLevel(mpInit->mPhysicsAccuracy);

	return true;
}

//-----------------------------------------------------------------------

void cMapHandler::ChangeMap(const tString &asMap, const tString &asPos, const tString &asStartSound, 
							const tString &asStopSound, float afFadeOutTime, float afFadeInTime,
							tString asLoadTextCat, tString asLoadTextEntry,
							tString asLoadImage)
{
	mMapChanger.msLoadImage = asLoadImage;
	mMapChanger.msNewMap = asMap;
    mMapChanger.msPosName = asPos;
	mMapChanger.msDoneSound = asStopSound;
	mMapChanger.mfFadeInTime = afFadeInTime;
	mMapChanger.mbActive = true;
	mMapChanger.msLoadTextCat = asLoadTextCat;
	mMapChanger.msLoadTextEntry = asLoadTextEntry;

	mpInit->mpFadeHandler->FadeOut(afFadeOutTime);
	
	if(asStartSound!="")
		mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui(asStartSound,false,1);

	mpInit->mpPlayer->SetActive(false);
}

//-----------------------------------------------------------------------

void cMapHandler::AddSaveData(cSavedWorld* apSavedWorld)
{	
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	
	/////////////////////////
	//Properties
	apSavedWorld->msGameName = msMapGameName;

	/////////////////////////
	//Local timers
	for(tGameTimerListIt it = mlstTimers.begin(); it != mlstTimers.end(); ++it)
	{
		cGameTimer *pTimer = *it;
		
		if(pTimer->mbGlobal==false)
		{
			apSavedWorld->mlstTimers.Add(*pTimer);
		}
	}

	/////////////////////////
	//Map script vars
	tScriptVarMap *pVarMap = mpScene->GetLocalVarMap();
	tScriptVarMapIt varIt = pVarMap->begin();
	for(; varIt !=  pVarMap->end(); ++varIt)
	{
		cScriptVar &scriptVar = varIt->second;
		scriptVar.mlVal = scriptVar.mlVal;
		scriptVar.msName = scriptVar.msName;
		apSavedWorld->mlstVars.Add(scriptVar);
	}

	//////////////////
	//Engine data
	
	//Lights
	{
		cLight3DListIterator it = pWorld->GetLightIterator();
		while(it.HasNext())
		{
			iLight3D *pLight = it.Next();
			cEngineLight_SaveData saveLight;

			if(pLight->IsSaved() && pLight->GetParent() == nullptr && pLight->GetEntityParent()==nullptr)
			{
				saveLight.FromLight(pLight);
				apSavedWorld->mlstLights.Add(saveLight);
			}
		}
	}

	//Particle systems
	{
		cParticleSystem3DIterator it = pWorld->GetParticleSystemIterator();
		while(it.HasNext())
		{
			cParticleSystem3D *pPS = it.Next();
			cEnginePS_SaveData savePS;

			if( pPS->IsSaved() && pPS->GetParent() == nullptr && pPS->GetEntityParent()==nullptr
				&& pPS->IsDying()==false)
			{
				savePS.FromPS(pPS);
				apSavedWorld->mlstPS.Add(savePS);
			}
		}
	}

	//Beams
	{
		cBeamIterator it = pWorld->GetBeamIterator();
		while(it.HasNext())
		{
			cBeam *pBeam = it.Next();
			cEngineBeam_SaveData saveBeam;

			if(pBeam->IsSaved() && pBeam->GetParent() == nullptr && pBeam->GetEntityParent()==nullptr)
			{
				saveBeam.FromBeam(pBeam);
				apSavedWorld->mlstBeams.Add(saveBeam);
			}
		}
	}

	//Sounds
	{
		cSoundEntityIterator it = pWorld->GetSoundEntityIterator();
		while(it.HasNext())
		{
			cSoundEntity *pSound = it.Next();
			cEngineSound_SaveData saveSound;

			if(pSound->IsSaved() && pSound->GetParent() == nullptr && pSound->GetEntityParent()==nullptr)
			{
				//Check if the sound should be saved.
				if(pSound->GetRemoveWhenOver() && pSound->GetData()->GetLoop()==false) continue;

				saveSound.FromSound(pSound);
				apSavedWorld->mlstSounds.Add(saveSound);
			}
		}
	}
	
	//Joints
	{
		cPhysicsJointIterator it = pWorld->GetPhysicsWorld()->GetJointIterator();
		while(it.HasNext())
		{
			iPhysicsJoint *pJoint = it.Next();
			cEngineJoint_SaveData saveJoint;

			if(pJoint->IsSaved())
			{
				//Check if the joint should be saved.
				if(pJoint->IsBroken()) continue;

				saveJoint.FromJoint(pJoint);
				apSavedWorld->mlstJoints.Add(saveJoint);
			}
		}
	}

	//////////////////
	//Map handler
	tGameEntityMapIt GIt = m_mapGameEntities.begin();
	for(; GIt != m_mapGameEntities.end(); ++GIt)
	{
		iGameEntity *pEntity = GIt->second;
		
		if(pEntity->IsSaved())
		{
			//Log("Saving entity: '%s'\n",pEntity->GetName().c_str());

			iGameEntity_SaveData *pSaveData = pEntity->CreateSaveData();
			pEntity->SaveToSaveData(pSaveData);
			apSavedWorld->mlstEntities.Add(pSaveData);			
			//Log("Adding save data: %d\n", pSaveData);
		}
		else
		{
			//Log("Skipping entity: '%s'\n",pEntity->GetName().c_str());
		}
	}
}

//-----------------------------------------------------------------------

// Black Plague finds entities by name whatever the case of the letters
static tString EntityKey(const tString &asName)
{
	return cInit::IsOverture() ? asName : cString::ToLowerCase(asName);
}

void cMapHandler::LoadSaveData(cSavedWorld* apSavedWorld)
{
	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	iPhysicsWorld *pPhysicsWorld = pWorld->GetPhysicsWorld();
	std::list<iGameEntity*> lstEntities;
	
	/////////////////////////
	//Properties
	msMapGameName = apSavedWorld->msGameName;


	////////////////////////////////
	//Script variables
	cContainerListIterator<cScriptVar> scriptVar = apSavedWorld->mlstVars.GetIterator();
	while(scriptVar.HasNext())
	{
		cScriptVar &var = scriptVar.Next();
		cScriptVar *pVar = mpScene->CreateLocalVar(var.msName);
		pVar->mlVal = var.mlVal;
	}

	/////////////////////////
	//Local timers
	cContainerListIterator<cGameTimer> timerIt = apSavedWorld->mlstTimers.GetIterator();
	while(timerIt.HasNext())
	{
		cGameTimer& savedTimer = timerIt.Next();

        cGameTimer *pTimer = hplNew( cGameTimer, () );
		*pTimer = savedTimer;

		mlstTimers.push_back(pTimer);
	}

	////////////////////////////////
	//Engine data
	
	{
		///////////////////////
		//Lights
		cContainerListIterator<cEngineLight_SaveData> it = apSavedWorld->mlstLights.GetIterator();
		while(it.HasNext())
		{
			cEngineLight_SaveData &saveLight = it.Next();

			iLight3D *pLight = pWorld->GetLight(saveLight.msName);
			if(pLight)
			{
				saveLight.ToLight(pLight);
			}
			else
			{
				//Create light
				//This is most probably a temp light so skip it.
			}
		}
	}

	
	{
		//////////////////////////
		//Particle systems

		//Go through particle systems and throw away those not present.
		cParticleSystem3DIterator psIt = pWorld->GetParticleSystemIterator();
		while(psIt.HasNext())
		{
			cParticleSystem3D *pPS = psIt.Next();
			if(apSavedWorld->PSExists(pPS)==false)
			{
				pPS->KillInstantly();
			}
		}
		//Set data to map particle systems or create if no system exist.
		cContainerListIterator<cEnginePS_SaveData> it = apSavedWorld->mlstPS.GetIterator();
		while(it.HasNext())
		{
			cEnginePS_SaveData &savePS = it.Next();

			cParticleSystem3D *pPS = pWorld->GetParticleSystem(savePS.msName);
			if(pPS)
			{
				savePS.ToPS(pPS);
			}
			else
			{
				pPS = pWorld->CreateParticleSystem(savePS.msName,savePS.msType,savePS.mvSize,
													savePS.m_mtxTransform);
				savePS.ToPS(pPS);
			}
		}
	}

	//////////////////////////
	//Beams
	{
		//Go through beams and throw away those not present.
		cBeamIterator beamIt = pWorld->GetBeamIterator();
		while(beamIt.HasNext())
		{
			cBeam *pBeam = beamIt.Next();
			if(apSavedWorld->BeamExists(pBeam)==false)
			{
				mpScene->GetWorld3D()->DestroyBeam(pBeam);
			}
		}
		//Set data to map particle systems or create if no system exist.
		cContainerListIterator<cEngineBeam_SaveData> it = apSavedWorld->mlstBeams.GetIterator();
		while(it.HasNext())
		{
			cEngineBeam_SaveData &saveBeam = it.Next();

			cBeam *pBeam = pWorld->GetBeam(saveBeam.msName);
			if(pBeam)
			{
				saveBeam.ToBeam(pBeam);
			}
			else
			{
				pBeam = pWorld->CreateBeam(saveBeam.msName);
				saveBeam.ToBeam(pBeam);
			}
		}
	}

	{
		//////////////////////////
		//Sounds

		//Go through sound entities and throw away those not present.
		cSoundEntityIterator soundIt = pWorld->GetSoundEntityIterator();
		while(soundIt.HasNext())
		{
			cSoundEntity *pSound = soundIt.Next();
			if(apSavedWorld->SoundExists(pSound)==false)
			{
				mpScene->GetWorld3D()->DestroySoundEntity(pSound);
			}
		}
		//Set data to map sound entities
		cContainerListIterator<cEngineSound_SaveData> it = apSavedWorld->mlstSounds.GetIterator();
		while(it.HasNext())
		{
			cEngineSound_SaveData &saveSound = it.Next();

			cSoundEntity *pSound = pWorld->GetSoundEntity(saveSound.msName);
			if(pSound)
			{
				saveSound.ToSound(pSound);
			}
		}
	}
	
	////////////////////////////////
	//Create Entities
	std::set<tString> setSavedNames;
	{
		cContainerListIterator<iGameEntity_SaveData*> nameIt = apSavedWorld->mlstEntities.GetIterator();
		while(nameIt.HasNext()) setSavedNames.insert(EntityKey(nameIt.Next()->msName));
	}

	cContainerListIterator<iGameEntity_SaveData*> it = apSavedWorld->mlstEntities.GetIterator();
	while(it.HasNext())
	{
		iGameEntity_SaveData *pSaveEntity = it.Next();

		//////////////////////////////////
		//The entity is loaded from file
		if(pSaveEntity->msFileName!="")
		{
			std::set<iGameEntity*> setBefore;
			for(auto &entity : m_mapGameEntities) setBefore.insert(entity.second);

			if(pWorld->CreateEntity(pSaveEntity->msName,pSaveEntity->m_mtxTransform,pSaveEntity->msFileName, true))
			{
				//A model can hold other entities (Requiem level 3's broken key
				//box is only its key): the latest one made is then one of them.
				//They were saved on their own and are made from that, so the
				//ones made here go. It crashed: the box's save was loaded into
				//the key, and the key was there twice.
				iGameEntity *pGameEntity = nullptr;
				std::vector<iGameEntity*> vMade;
				for(auto &entity : m_mapGameEntities)
					if(setBefore.count(entity.second) == 0) vMade.push_back(entity.second);
				for(iGameEntity *pMade : vMade)
					if(pMade->GetName() == pSaveEntity->msName) pGameEntity = pMade;
				if(pGameEntity == nullptr) pGameEntity = mpInit->mpMapHandler->GetLatestEntity();
				for(iGameEntity *pMade : vMade)
				{
					if(pMade != pGameEntity && setSavedNames.count(EntityKey(pMade->GetName())))
						DestroyGameEntityNow(pMade);
				}

				pGameEntity->LoadFromSaveData(pSaveEntity);

				pGameEntity->SetSaveData(pSaveEntity);
				lstEntities.push_back(pGameEntity);
			}
		}
		//////////////////////////////////
		//The entity is an area.
		else
		{
			cVector3f vSize =0;
			tString sType = "";

			//Get type and size
			switch(pSaveEntity->mType)
			{
			case eGameEntityType_Area:
				vSize = static_cast<cGameArea_SaveData*>(pSaveEntity)->mvSize;
				sType = "script";
				break;
			case eGameEntityType_Link:
				vSize = static_cast<cGameLink_SaveData*>(pSaveEntity)->mvSize;
				sType = "link";
				break;
			case eGameEntityType_StickArea:
				vSize = static_cast<cGameStickArea_SaveData*>(pSaveEntity)->mvSize;
				sType = "stick";
				break;
			case eGameEntityType_SaveArea:
				vSize = static_cast<cGameSaveArea_SaveData*>(pSaveEntity)->mvSize;
				sType = "save";
				break;
			case eGameEntityType_Ladder:
				vSize = static_cast<cGameLadder_SaveData*>(pSaveEntity)->mvSize;
				sType = "ladder";
				break;
			case eGameEntityType_DamageArea:
				vSize = static_cast<cGameDamageArea_SaveData*>(pSaveEntity)->mvSize;
				sType = "damage";
				break;
			case eGameEntityType_ForceArea:
				vSize = static_cast<cGameForceArea_SaveData*>(pSaveEntity)->mvSize;
				sType = "force";
				break;
			case eGameEntityType_LiquidArea:
				vSize = static_cast<cGameLiquidArea_SaveData*>(pSaveEntity)->mvSize;
				sType = "liquid";
				break;
			case eGameEntityType_VentArea:
				vSize = static_cast<cGameVentArea_SaveData*>(pSaveEntity)->mvSize;
				sType = "vent";
				break;
			}

			//Load entity
			if(sType!="")
			{
				iArea3DLoader *pLoader = pWorld->GetResources()->GetArea3DLoader(sType);
				if(pLoader)
				{
					pLoader->Load(pSaveEntity->msName,vSize,pSaveEntity->m_mtxTransform,pWorld);

					iGameEntity *pGameEntity = mpInit->mpMapHandler->GetLatestEntity();
					pGameEntity->LoadFromSaveData(pSaveEntity);

					pGameEntity->SetSaveData(pSaveEntity);
					lstEntities.push_back(pGameEntity);
				}
			}
		}
	}

	////////////////////////////////
	//Setup Entities
	std::list<iGameEntity*>::iterator entIt = lstEntities.begin();
	for(; entIt != lstEntities.end(); ++entIt)
	{
		iGameEntity* pEntity = *entIt;

		pEntity->SetupSaveData(pEntity->GetSaveData());
	}

	{
		//////////////////////////
		//Joints
		// this need to be done after entity creation so all joints are created.

		//Go through joints and throw away those not present.
		cPhysicsJointIterator jointIt = pPhysicsWorld->GetJointIterator();
		while(jointIt.HasNext())
		{
			iPhysicsJoint *pJoint = jointIt.Next();
			if(apSavedWorld->JointExists(pJoint)==false)
			{
				pPhysicsWorld->DestroyJoint(pJoint);
			}
		}
		//Set data to map Joint entities
		cContainerListIterator<cEngineJoint_SaveData> it = apSavedWorld->mlstJoints.GetIterator();
		while(it.HasNext())
		{
			cEngineJoint_SaveData &saveJoint = it.Next();

			iPhysicsJoint *pJoint = pPhysicsWorld->GetJoint(saveJoint.msName);
			if(pJoint)
			{
				saveJoint.ToJoint(pJoint);
			}
		}
	}

}
//-----------------------------------------------------------------------

void cMapHandler::OnStart()
{

}

//-----------------------------------------------------------------------

void cMapHandler::OnWorldLoad()
{

}

//-----------------------------------------------------------------------

void cMapHandler::OnDraw()
{
	tGameEntityMapIt GIt = m_mapGameEntities.begin();
	for(; GIt != m_mapGameEntities.end(); ++GIt)
	{
		iGameEntity *pEntity = GIt->second;

		pEntity->OnDraw();
	}
}

void cMapHandler::RenderItemEffect()
{
	if(mpInit->mbFlashItems == false) return;

	//Check if any item needs the effect.
	bool bFound = false;
	tGameItemListIt it = mlstGameItems.begin();
	for(; it != mlstGameItems.end(); ++it)
	{
		cGameItem *pItem = *it;
		if(pItem->IsActive() && pItem->GetFlashAlpha() >0)
		{
			bFound = true;
			break;
		}
	}
	if(bFound==false) return;

	cCamera3D *pCam = static_cast<cCamera3D*>(mpScene->GetCamera());
	iLowLevelGraphics *pLowGfx = mpInit->mpGame->GetGraphics()->GetLowLevel();

	pLowGfx->SetDepthTestActive(true);
	pLowGfx->SetDepthWriteActive(false);

	pLowGfx->SetBlendActive(true);
	pLowGfx->SetBlendFunc(eBlendFunc_One,eBlendFunc_One);

	it = mlstGameItems.begin();
	for(; it != mlstGameItems.end(); ++it)
	{
		cGameItem *pItem = *it;
		
		if(pItem->IsActive()==false) continue;
		if(pItem->GetFlashAlpha() <=0) continue;

		cMeshEntity *pMeshEntity = pItem->GetMeshEntity();

		pLowGfx->SetDrawMatrices(pMeshEntity->GetWorldMatrix(), pCam->GetViewMatrix(), pCam->GetProjectionMatrix());
		for(int i=0; i< pMeshEntity->GetMesh()->GetSubMeshNum(); i++)
		{
			cSubMeshEntity *pSubEntity = pMeshEntity->GetSubMeshEntity(i);
			cSubMesh *pSubMesh = pMeshEntity->GetMesh()->GetSubMesh(i);
			iVertexBuffer *pVtxBuffer = pSubEntity->GetVertexBuffer();
			iMaterial *pMaterial = pSubEntity->GetMaterial();
			
			iGpuProgram* program = pMaterial->GetProgram(eMaterialRenderType_Z,0,nullptr);
			if(!program) program = mpFlashProgram;
			if(program){
				program->SetColor3f(eProgramParam::ambientColor, pItem->GetFlashAlpha());
				program->Bind();
			}

			pLowGfx->SetTexture(0,pMaterial->GetTexture(eMaterialTexture_Diffuse));

			pVtxBuffer->Bind();
			pVtxBuffer->Draw();
			pVtxBuffer->UnBind();

			if(program) program->UnBind();
		}
	}

	pLowGfx->SetTexture(0,nullptr);

	pLowGfx->SetBlendActive(false);
	pLowGfx->SetDepthTestActive(true);
	pLowGfx->SetDepthWriteActive(true);

}
//-----------------------------------------------------------------------

void cMapHandler::OnPostSceneDraw()
{
	////////////////////////////////////
	// Draw effect on items
	RenderItemEffect();


	cCamera3D *pCam = static_cast<cCamera3D*>(mpScene->GetCamera());
	mpInit->mpGame->GetGraphics()->GetLowLevel()->SetDrawMatrices(cMatrixf::Identity, pCam->GetViewMatrix(), pCam->GetProjectionMatrix());

	//mpScene->GetWorld3D()->GetPhysicsWorld()->RenderDebugGeometry(
	//	mpInit->mpGame->GetGraphics()->GetLowLevel(),cColor(1,0.5f,1));

	
	/*mpInit->mpGame->GetGraphics()->GetLowLevel()->SetDepthTestActive(false);
	cParticleSystem3DIterator PIt = mpScene->GetWorld3D()->GetParticleSystemIterator();
	while(PIt.HasNext())
	{
		cParticleSystem3D*pPS = PIt.Next();

		//if(pPS->GetName() == "tripod01_tripodflame")
		//{
		//	static bool bStatic = true;
		//	if(bStatic){//pPS->SetTransformUpdated(true);
		//		Log("Once!!!\n");
		//	}
		//	bStatic =false;

			//pPS->SetPosition(pPS->GetLocalPosition());
			mpInit->mpGame->GetGraphics()->GetLowLevel()->DrawSphere(pPS->GetWorldPosition(),
																0.3f,cColor(1,0,1));	

			mpInit->mpGame->GetGraphics()->GetLowLevel()->DrawBoxMaxMin(
												pPS->GetEmitter(0)->GetBoundingVolume()->GetMax(),
												pPS->GetEmitter(0)->GetBoundingVolume()->GetMin(),
												cColor(0,1,1));
		
	}
	mpInit->mpGame->GetGraphics()->GetLowLevel()->SetDepthTestActive(true);*/
	
	////////////////////////////////////
	// Let Entities post draw
	tGameEntityMapIt GIt = m_mapGameEntities.begin();
	for(; GIt != m_mapGameEntities.end(); ++GIt)
	{
		iGameEntity *pEntity = GIt->second;

		pEntity->OnPostSceneDraw();
	}
	
	return;
	mpInit->mpGame->GetGraphics()->GetLowLevel()->SetTexture(0,nullptr);
	mpInit->mpGame->GetGraphics()->GetLowLevel()->SetBlendActive(false);
	
	mpInit->mpGame->GetGraphics()->GetLowLevel()->SetDepthTestActive(false);

	cVector3f vMin = mpScene->GetWorld3D()->GetPhysicsWorld()->GetWorldSizeMin();
	cVector3f vMax = mpScene->GetWorld3D()->GetPhysicsWorld()->GetWorldSizeMax();

	//mpInit->mpGame->GetGraphics()->GetLowLevel()->DrawBoxMaxMin(vMax,vMin,cColor(1,0,1));

	//mpScene->GetWorld3D()->DrawMeshBoundingBoxes(cColor(1,0.5f,1),false);
	//mpScene->GetWorld3D()->GetPhysicsWorld()->RenderDebugGeometry(
	//						mpInit->mpGame->GetGraphics()->GetLowLevel(),cColor(1,0.5f,1));
	//mpScene->GetWorld3D()->GetPhysicsWorld()->SetSaveContactPoints(true);
	//mpScene->GetWorld3D()->GetPhysicsWorld()->RenderContactPoints(mpInit->mpGame->GetGraphics()->GetLowLevel(),cColor(1,0.5f,1),
	//											cColor(1,1,0.5f));
	
	cLight3DListIterator lightIt = mpScene->GetWorld3D()->GetLightIterator();
	while(lightIt.HasNext())
	{
		iLight3D *pLight = lightIt.Next();

		if(pLight->IsVisible() && pLight->IsActive() && pLight->GetLightType() == eLight3DType_Spot)
		{
			cBoundingVolume *pBV = pLight->GetBoundingVolume();
			mpInit->mpGame->GetGraphics()->GetLowLevel()->DrawBoxMaxMin(
									pBV->GetMax(),pBV->GetMin(),cColor(1,0,1));
		}
	}

	
	/*mpInit->mpGame->GetGraphics()->GetLowLevel()->SetDepthTestActive(false);
	cBillboardIterator billIt = mpScene->GetWorld3D()->GetBillboardIterator();
	while(billIt.HasNext())
	{
		cBillboard *pBillboard = billIt.Next();

		mpInit->mpGame->GetGraphics()->GetLowLevel()->DrawBoxMaxMin(
								pBillboard->GetBoundingVolume()->GetMax(),
								pBillboard->GetBoundingVolume()->GetMin(),cColor(1,0,1));
	}*/

	GIt = m_mapGameEntities.begin();
	for(; GIt != m_mapGameEntities.end(); ++GIt)
	{
		iGameEntity *pEntity = GIt->second;

		//if(pEntity->GetBodyNum()<=0) continue;
		
		//mpInit->mpGame->GetGraphics()->GetLowLevel()->SetDepthTestActive(false);

		//mpInit->mpGame->GetGraphics()->GetLowLevel()->DrawBoxMaxMin(
		//						pEntity->GetMeshEntity()->GetBoundingVolume()->GetMax(),
		//						pEntity->GetMeshEntity()->GetBoundingVolume()->GetMin(),cColor(1,0,1));

		/*for(int i=0; i < pEntity->GetBodyNum(); ++i)
		{
			iPhysicsBody* pBody = pEntity->GetBody(i);
			if(pBody){
				cBoundingVolume *pBV = pBody->GetBV();
				mpInit->mpGame->GetGraphics()->GetLowLevel()->DrawBoxMaxMin(pBV->GetMax(),pBV->GetMin(),
																cColor(1,0.5f,1));
				//pBody->RenderDebugGeometry(mpInit->mpGame->GetGraphics()->GetLowLevel(),cColor(1,0.5f,1));
			}
		}*/
	}
	

	mpInit->mpGame->GetGraphics()->GetLowLevel()->SetDepthTestActive(true);
}

//-----------------------------------------------------------------------

// For the "rays" test command: logs every body a ray hits
class cLogRayCallback : public iPhysicsRayCallback
{
public:
	bool OnIntersect(iPhysicsBody *apBody, cPhysicsRayParams *apParams)
	{
		iGameEntity *pEntity = (iGameEntity*)apBody->GetUserData();
		Log("   %.4f m: body '%s' material '%s' entity '%s' type %d active %d collide %d id %d\n",
			apParams->mfDist, apBody->GetName().c_str(), apBody->GetMaterial()->GetName().c_str(),
			pEntity ? pEntity->GetName().c_str() : "", pEntity ? (int)pEntity->GetType() : -1,
			pEntity ? (int)pEntity->IsActive() : -1, (int)apBody->GetCollide(), apBody->GetSaveObjectId());
		return true;
	}
};

// For tests: OPENHPL_SCRIPT="2:DoThis();|5.5:DoThat();|7:gui:InScreen();" runs
// script commands that many seconds after the game has started, and says so
// in the log. With "gui:" before it a command is for the open computer terminal;
// "save" writes an auto save, "load" loads the newest auto save, "pos:x,y,z"
// puts the player there, "where:body" logs a body's place, "lookat:body"
// turns the camera to it, "rays" logs what the pick and ground rays hit,
// "joint:name" where a joint is, "sounds" the sound entities playing and
// "click" uses what the player looks at.
// Returns true when a save was loaded (the world was changed under the update).
static bool UpdateTestScript(cInit *apInit, float afTimeStep)
{
	struct cCommand { float mfTime; tString msCommand; };
	static std::vector<cCommand> vCommands;
	static bool bRead = false;
	static float fTime = 0;

	if(bRead==false)
	{
		bRead = true;
		const char *pSpec = SDL_getenv("OPENHPL_SCRIPT");
		tStringVec vParts;
		tString sSep = "|";
		if(pSpec) cString::GetStringVec(pSpec, vParts, &sSep);
		for(const tString &sPart : vParts)
		{
			size_t lColon = sPart.find(':');
			if(lColon == tString::npos) continue;
			vCommands.push_back({cString::ToFloat(sPart.substr(0, lColon).c_str(), 0), sPart.substr(lColon+1)});
		}
	}
	if(vCommands.empty()) return false;

	float fLastTime = fTime;
	fTime += afTimeStep;
	for(const cCommand &command : vCommands)
	{
		if(command.mfTime < fLastTime || command.mfTime >= fTime) continue;

		Log("TEST SCRIPT at %.2f: %s\n", fTime, command.msCommand.c_str());

		// "gui:Command();" is run in the script of the open computer terminal
		if(command.msCommand.compare(0, 4, "gui:") == 0)
		{
			cGameGui *pGui = apInit->mpGameGuiHandler->GetScreenGui();
			if(pGui) pGui->RunScriptCommand(command.msCommand.substr(4));
			else Log(" no computer terminal is open\n");
			continue;
		}
		// "pos:x,y,z" puts the player there (feet), "where:body" logs where a body is
		if(command.msCommand.compare(0, 4, "pos:") == 0)
		{
			tFloatVec vPos;
			cString::GetFloatVec(command.msCommand.substr(4), vPos);
			if(vPos.size() == 3)
				apInit->mpPlayer->GetCharacterBody()->SetFeetPosition(cVector3f(vPos[0], vPos[1], vPos[2]));
			continue;
		}
		// "ray:x,y,z,x,y,z" logs what a ray between two points hits, and what
		// the player's pick would choose of it
		if(command.msCommand.compare(0, 4, "ray:") == 0)
		{
			tFloatVec vNum;
			cString::GetFloatVec(command.msCommand.substr(4), vNum, nullptr);
			if(vNum.size() < 6) continue;
			cVector3f vStart(vNum[0],vNum[1],vNum[2]), vEnd(vNum[3],vNum[4],vNum[5]);
			iPhysicsWorld *pPhysicsWorld = apInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();
			cLogRayCallback callback;
			pPhysicsWorld->CastRay(&callback, vStart, vEnd, true, false, true);
			cPlayerPickRayCallback pick;
			pick.mfMaxDistance = cMath::Vector3Dist(vStart, vEnd);
			pick.Clear();
			pPhysicsWorld->CastRay(&pick, vStart, vEnd, true, false, true);
			pick.CalculateResults();
			Log(" pick: '%s' at %.4f m\n", pick.mpPickedBody ? pick.mpPickedBody->GetName().c_str() : "", pick.mfPickedDist);
			continue;
		}
		if(command.msCommand.compare(0, 6, "where:") == 0)
		{
			tString sName = command.msCommand.substr(6);
			cPhysicsBodyIterator it = apInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld()->GetBodyIterator();
			while(it.HasNext())
			{
				iPhysicsBody *pBody = it.Next();
				if(pBody->GetName() == sName)
					Log(" %s at %s, speed %.2f\n", sName.c_str(), pBody->GetWorldPosition().ToString().c_str(),
						pBody->GetLinearVelocity().Length());
			}
			continue;
		}
		// "sounds" logs the sound entities that are not stopped
		if(command.msCommand == "sounds")
		{
			cSoundEntityIterator it = apInit->mpGame->GetScene()->GetWorld3D()->GetSoundEntityIterator();
			while(it.HasNext())
			{
				cSoundEntity *pSound = it.Next();
				if(pSound->IsStopped()) continue;
				cSoundEntry *pEntry = pSound->GetSoundEntry(eSoundEntityType_Main);
				Log(" sound '%s' (%s) playing %d volume %.2f fading out %d\n", pSound->GetName().c_str(),
					pSound->GetData() ? pSound->GetData()->GetName().c_str() : "", pEntry != nullptr,
					pEntry ? pEntry->mfNormalVolumeMul : 0.0f, (int)pSound->IsFadingOut());
			}
			continue;
		}
		// "joint:name" logs where a joint is between its limits
		if(command.msCommand.compare(0, 6, "joint:") == 0)
		{
			tString sName = command.msCommand.substr(6);
			cPhysicsJointIterator it = apInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld()->GetJointIterator();
			while(it.HasNext())
			{
				iPhysicsJoint *pJoint = it.Next();
				if(pJoint->GetName() != sName) continue;
				iPhysicsBody *pChild = pJoint->GetChildBody();
				iPhysicsJointSlider *pSlider = pJoint->GetType() == ePhysicsJointType_Slider ? (iPhysicsJointSlider*)pJoint : nullptr;
				Log(" %s: dist %.4f angle %.4f limits %.4f..%.4f speed %.3f child '%s' at %s frozen %d sleeping %d\n", sName.c_str(),
					pJoint->GetDistance(), pJoint->GetAngle(), pSlider ? pSlider->GetMinDistance() : 0.0f, pSlider ? pSlider->GetMaxDistance() : 0.0f,
					pJoint->GetVelocity().Length(), pChild ? pChild->GetName().c_str() : "",
					pChild ? pChild->GetWorldPosition().ToString().c_str() : "", pChild ? (int)pChild->IsFrozen() : -1, pChild ? (int)pChild->IsSleeping() : -1);
			}
			continue;
		}
		// "lookat:body" turns the camera to look at a body's centre
		if(command.msCommand.compare(0, 7, "lookat:") == 0)
		{
			// or "lookat:x,y,z" at a point
			tString sName = command.msCommand.substr(7);
			tFloatVec vNum;
			cString::GetFloatVec(sName, vNum, nullptr);
			cPhysicsBodyIterator it = apInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld()->GetBodyIterator();
			bool bPoint = vNum.size() == 3;
			while(bPoint || it.HasNext())
			{
				iPhysicsBody *pBody = bPoint ? nullptr : it.Next();
				if(pBody && pBody->GetName() != sName) continue;
				cVector3f vTarget = pBody ? pBody->GetBoundingVolume()->GetWorldCenter() : cVector3f(vNum[0],vNum[1],vNum[2]);
				bPoint = false;
				cCamera3D *pCamera = apInit->mpPlayer->GetCamera();
				cVector3f vDir = vTarget - pCamera->GetPosition();
				pCamera->SetYaw(atan2(-vDir.x, -vDir.z));
				pCamera->SetPitch(atan2(vDir.y, sqrt(vDir.x*vDir.x + vDir.z*vDir.z)));
				apInit->mpPlayer->GetCharacterBody()->SetYaw(pCamera->GetYaw());
				Log(" looking at %s from %s, %.2f m away\n", sName.c_str(), pCamera->GetPosition().ToString().c_str(), vDir.Length());
			}
			continue;
		}
		// "rays" logs what the pick ray and the ground ray hit
		if(command.msCommand == "rays")
		{
			iPhysicsWorld *pPhysicsWorld = apInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();
			cCamera3D *pCamera = apInit->mpPlayer->GetCamera();
			cLogRayCallback callback;
			iPhysicsBody *pPicked = apInit->mpPlayer->GetPickedBody();
			Log(" player active %d state %d move state %d picked '%s' at %.3f m crosshair %d, pick ray from %s\n",
				(int)apInit->mpPlayer->IsActive(), (int)apInit->mpPlayer->GetState(), (int)apInit->mpPlayer->GetMoveState(),
				pPicked ? pPicked->GetName().c_str() : "", apInit->mpPlayer->GetPickedDist(),
				(int)apInit->mpPlayer->GetCrossHairState(), pCamera->GetPosition().ToString().c_str());
			pPhysicsWorld->CastRay(&callback, pCamera->GetPosition(), pCamera->GetPosition() + pCamera->GetForward()*10, true, false, true);
			iCharacterBody *pCharBody = apInit->mpPlayer->GetCharacterBody();
			cVector3f vStart = pCharBody->GetPosition() - cVector3f(0, pCharBody->GetShape()->GetSize().y/2-0.3f, 0);
			Log(" ground ray from %s\n", vStart.ToString().c_str());
			pPhysicsWorld->CastRay(&callback, vStart, vStart + cVector3f(0,-0.6f,0), true, false, false);
			// and any NaN body: one hides most of the map from ray casts
			cPhysicsBodyIterator it = pPhysicsWorld->GetBodyIterator();
			while(it.HasNext())
			{
				iPhysicsBody *pBody = it.Next();
				if(std::isnan(pBody->GetWorldPosition().x) || std::isnan(pBody->GetBV()->GetMin().x))
					Log("   NaN body: '%s'\n", pBody->GetName().c_str());
			}
			continue;
		}
		// "click" presses and lets go of Interact, as the left mouse button does
		if(command.msCommand == "click")
		{
			apInit->mpPlayer->StartInteract();
			apInit->mpPlayer->StopInteract();
			continue;
		}
		if(command.msCommand == "save")
		{
			apInit->mpSaveHandler->AutoSave(_W("auto"), 10);
			continue;
		}
		if(command.msCommand == "load")
		{
			apInit->mpSaveHandler->AutoLoad(_W("auto"));
			return true;
		}

		apInit->RunScriptCommand(command.msCommand);
	}
	return false;
}

//-----------------------------------------------------------------------

void cMapHandler::Update(float afTimeStep)
{
	mfGameTime += (double)afTimeStep;

	if(UpdateTestScript(mpInit, afTimeStep)) return;

	//iLowLevelSystem *pLowLevelSystem =mpInit->mpGame->GetSystem()->GetLowLevel();
	
	//LogUpdate(" Flashes!\n");
	
	////////////////////////////
	//Update light flashes
	tEffectLightFlashListIt FlashIt = mlstLightFlashes.begin();
	for(; FlashIt != mlstLightFlashes.end(); )
	{
		cEffectLightFlash *pFlash = *FlashIt;

		pFlash->Update(afTimeStep);
		if(pFlash->IsDead())
		{
			hplDelete( pFlash );
			FlashIt = mlstLightFlashes.erase(FlashIt);
		}
		else
		{
			++FlashIt;
		}
	}
	
	//LogUpdate(" Timers\n");
	////////////////////////////
	//Update timers
	UpdateTimers(afTimeStep);

	//LogUpdate(" Entities!\n");
	////////////////////////////
	//Update entities.
	const bool bOverture = cInit::IsOverture();
	tGameEntityMapIt GIt = m_mapGameEntities.begin();
	for(; GIt != m_mapGameEntities.end();)
	{
		iGameEntity *pEntity = GIt->second;

		//unsigned int lTime = pLowLevelSystem->GetTime();
		//LogUpdate("  %s\n",pEntity->GetName().c_str());

		if(pEntity->IsActive()){
			pEntity->OnUpdate(afTimeStep);
		}

		// Black Plague destroys entities only after all have been updated
		if(bOverture && (pEntity->GetDestroyMe() || pEntity->GetBreakMe()))
		{
			//LogUpdate("    destroying\n");
			if(pEntity->GetBreakMe()) pEntity->BreakAction();
			
			m_mapGameEntities.erase(GIt++);
			hplDelete( pEntity );
			//LogUpdate("    done destroying\n");
		}
		else
		{
			////LogUpdate("  updating %s took %d ms\n",	pEntity->GetName().c_str(),
			//										pLowLevelSystem->GetTime() - lTime);
			++GIt;
		}
	}

	if(bOverture==false)
	{
		for(GIt = m_mapGameEntities.begin(); GIt != m_mapGameEntities.end();)
		{
			iGameEntity *pEntity = GIt->second;
			if(pEntity->GetDestroyMe() || pEntity->GetBreakMe())
			{
				if(pEntity->GetBreakMe()) pEntity->BreakAction();

				m_mapGameEntities.erase(GIt++);
				hplDelete( pEntity );
			}
			else
			{
				++GIt;
			}
		}
	}

	////////////////////////////
	//Check for map change
	//LogUpdate("  Map change\n");
	if(mMapChanger.mbActive && mpInit->mpFadeHandler->IsActive()==false)
	{
		mMapChanger.mbActive = false;

		//Log("---------- CHANGING TO MAP: '%s' ------------------\n",mMapChanger.msNewMap.c_str());
		
		//Make sure that the graphics are not destroyed at the end of the map so
		//That the next map doesn't have to reload em.
		mpInit->mbDestroyGraphics = false;

		//Draw Loading image
		if(mMapChanger.msLoadTextCat != "")
		{
			mpInit->mpMapLoadText->SetText(mMapChanger.msLoadTextCat,mMapChanger.msLoadTextEntry);
			mpInit->mpMapLoadText->SetBackground(mMapChanger.msLoadImage != "" ? mMapChanger.msLoadImage
																				: tString("loadscreen_default.jpg"));
			mpInit->mpMapLoadText->SetActive(true);
		}
		else
		{
			if(cInit::IsOverture())
				mpInit->mpGraphicsHelper->DrawLoadingScreen("other_loading.jpg");
			else if(mMapChanger.msLoadImage != "")
				mpInit->mpGraphicsHelper->DrawLoadingScreen(mMapChanger.msLoadImage);
			//Requiem: black without "Loading" (its check for an image called
			//"black" can never be true, so this is all it does)
			else if(cInit::GetGameId() == eGameId_Requiem)
				mpInit->mpGraphicsHelper->DrawLoadingScreen("", false);
			else
				mpInit->mpGraphicsHelper->DrawLoadingScreen("loadscreen_default.jpg");
		}
		
		//Fade in here so that script will over ride it.
		mpInit->mpFadeHandler->FadeIn(mMapChanger.mfFadeInTime);

		Load(mMapChanger.msNewMap,mMapChanger.msPosName);

		mpInit->mbDestroyGraphics = true;

		if(mMapChanger.msDoneSound!="")
			mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui(mMapChanger.msDoneSound,false,1);
		
		mpInit->mpPlayer->SetActive(true);

		//Log("Map change over...\n");
	}
	
}

//-----------------------------------------------------------------------

void cMapHandler::Reset()
{
#ifdef DEMO_VERSION
	glNumOfLoads = 0;
#endif

	mMapChanger.mbActive = false;
	msCurrentMap = "";

	if(mpInit->mbResetCache) mpWorldCache->DecResources();

	mvLoadedMaps.clear();

	//Timers
	STLDeleteAll(mlstTimers);

	DestroyAll();

	//Haptic
	if(mpInit->mbHasHaptics)
	{
		mpInit->mpGame->GetHaptic()->GetLowLevel()->DestroyAllShapes();
		mpInit->mpGame->GetHaptic()->GetLowLevel()->StopAllForces();
	}

	//World3D
	if(mpScene->GetWorld3D()) mpScene->DestroyWorld3D(mpScene->GetWorld3D());
	mpScene->SetWorld3D(nullptr);

	//Make sure no occulssion queries are left.
	mpInit->mpGame->GetGraphics()->GetRenderer3D()->GetRenderList()->Clear();
}

//-----------------------------------------------------------------------

// Hits of world geometry and other bodies that do not move, with normals
class cStaticRayCallback : public iPhysicsRayCallback
{
public:
	iPhysicsBody *mpSkip = nullptr;
	float mfMinT = 2;
	cVector3f mvNormal;
	bool OnIntersect(iPhysicsBody *apBody, cPhysicsRayParams *apParams) override
	{
		if(apBody == mpSkip || apBody->GetMass() != 0 || apBody->IsCharacter() || !apBody->GetCollide()) return true;
		iGameEntity *pEntity = (iGameEntity*)apBody->GetUserData();
		if(pEntity && pEntity->GetType() != eGameEntityType_Object) return true; //areas
		if(apParams->mfT < mfMinT){ mfMinT = apParams->mfT; mvNormal = apParams->mvNormal; }
		return true;
	}
};

// Some bodies are placed with their middle a little under the floor (level
// 7's painkillers in a locker, 1.6 cm). Newton 1 pushed them up; Newton 2
// only pushes a body out of a floor whose face it is in front of, so these
// fell through the world before the player arrived. Lift them onto it.
void cMapHandler::LiftBodiesOutOfFloors(cWorld3D *apWorld)
{
	iPhysicsWorld *pPhysicsWorld = apWorld->GetPhysicsWorld();
	cPhysicsBodyIterator it = pPhysicsWorld->GetBodyIterator();
	while(it.HasNext())
	{
		iPhysicsBody *pBody = it.Next();
		if(pBody->GetMass() == 0 || pBody->IsCharacter() || pBody->IsRagDoll() || pBody->GetJointNum() > 0) continue;
		if(!pBody->GetCollide()) continue;

		cBoundingVolume *pBV = pBody->GetBV();
		cVector3f vCenter = pBV->GetWorldCenter();
		float fTop = pBV->GetMax().y;
		if(fTop <= vCenter.y) continue;

		//From the top of the body down to its middle: a floor in between
		//means the middle is under it
		cStaticRayCallback callback;
		callback.mpSkip = pBody;
		cVector3f vStart(vCenter.x, fTop, vCenter.z);
		pPhysicsWorld->CastRay(&callback, vStart, vCenter, true, true, true);
		if(callback.mfMinT > 0.99f || callback.mvNormal.y < 0.7f) continue;

		float fFloorY = fTop + (vCenter.y - fTop) * callback.mfMinT;
		float fLift = fFloorY - pBV->GetMin().y + 0.002f;
		if(fLift <= 0) continue;

		Log("Lifting body '%s' %.3f m out of the floor under it\n", pBody->GetName().c_str(), fLift);
		cMatrixf mtxBody = pBody->GetLocalMatrix();
		mtxBody.SetTranslation(mtxBody.GetTranslation() + cVector3f(0, fLift, 0));
		pBody->SetMatrix(mtxBody);
	}
}

//-----------------------------------------------------------------------

void cMapHandler::DestroyAll()
{
	mbDestroyingAll = true;
	RemoveLocalTimers();

	//Game entities
	STLMapDeleteAll(m_mapGameEntities);
	m_mapGameEntities.clear();
	mlstGameEnemies.clear();
	mlstGameItems.clear();

	//Light flashes
	STLDeleteAll(mlstLightFlashes);
	mlstLightFlashes.clear();
	mbDestroyingAll = false;
}

//-----------------------------------------------------------------------

cGameTimer * cMapHandler::CreateTimer(const tString& asName,float afTime, const tString& asCallback, bool abGlobal)
{
	cGameTimer *pTimer = hplNew( cGameTimer, () );

	pTimer->msName = asName;
	pTimer->msCallback = asCallback;
	pTimer->mfTime = afTime;
	pTimer->mbGlobal = abGlobal;

	mlstTimers.push_back(pTimer);

	return pTimer;
}

cGameTimer * cMapHandler::GetTimer(const tString& asName)
{
	return (cGameTimer*)STLFindByName(mlstTimers,asName);
}

//-----------------------------------------------------------------------

void cMapHandler::AddLightFlash(const cVector3f& avPos,float afRadius, const cColor &aColor, float afFadeInSeconds, float afFadeOutSeconds)
{
	cEffectLightFlash *pFlash = hplNew( cEffectLightFlash, (mpInit,avPos,afRadius,aColor,afFadeInSeconds,afFadeOutSeconds) );

	mlstLightFlashes.push_back(pFlash);
}

//-----------------------------------------------------------------------

void cMapHandler::AddStickArea(cGameStickArea *apArea)
{
	mlstGameStickAreas.push_back(apArea);
}

//-----------------------------------------------------------------------

void cMapHandler::RemoveStickArea(cGameStickArea *apArea)
{
	tGameStickAreaListIt it = mlstGameStickAreas.begin();
	for(; it != mlstGameStickAreas.end(); ++it)
	{
		if(*it == apArea)
		{
			mlstGameStickAreas.erase(it);
			break;
		}
	}
}

//-----------------------------------------------------------------------

cGameStickArea* cMapHandler::GetBodyStickArea(iPhysicsBody *apBody)
{
	tGameStickAreaListIt it = mlstGameStickAreas.begin();
	for(; it != mlstGameStickAreas.end(); ++it)
	{
		cGameStickArea *pArea = *it;

		if(pArea->GetAttachedBody() == apBody)
		{
			return pArea;
		}

	}
	
	return nullptr;
}

//-----------------------------------------------------------------------

void cMapHandler::AddGameEnemy(iGameEnemy *apEnemy)
{
	mlstGameEnemies.push_back(apEnemy);
}

tGameEnemyIterator cMapHandler::GetGameEnemyIterator()
{
	return tGameEnemyIterator(&mlstGameEnemies);
}

//-----------------------------------------------------------------------

void cMapHandler::AddGameItem(cGameItem *apItem)
{
	mlstGameItems.push_back(apItem);
}

void cMapHandler::RemoveGameItem(cGameItem *apItem)
{
	tGameItemListIt it = mlstGameItems.begin();
	for(; it != mlstGameItems.end(); ++it)
	{
		cGameItem *pItem = *it;
		if(pItem == apItem){
			mlstGameItems.erase(it);
			break;
		}
	}
}

//-----------------------------------------------------------------------

void cMapHandler::AddGameEntity(iGameEntity *apEntity)
{
	iGameEntity *pSameNameEntity = GetGameEntity(apEntity->GetName(),false);
	if(pSameNameEntity)
	{
		Warning("Entity '%s' with file '%s' has name already taken by '%s'!\n",	
															apEntity->GetName().c_str(),
															apEntity->GetFileName().c_str(),
															pSameNameEntity->GetFileName().c_str());
	}

	mpLatestEntity = apEntity;
	m_mapGameEntities.insert(tGameEntityMap::value_type(EntityKey(apEntity->GetName()),apEntity));
}

void cMapHandler::RemoveGameEntity(iGameEntity *apEntity)
{
	// Black Plague: switched off now, destroyed after the entities' update
	if(cInit::IsOverture()==false)
	{
		apEntity->SetActive(false);
		apEntity->DestroyMe();
		return;
	}

	m_mapGameEntities.erase(apEntity->GetName());

	//If the entity is an enemy remove that aswel
	{
		tGameEnemyListIt it = mlstGameEnemies.begin();
		for(; it != mlstGameEnemies.end(); ++it)
		{
			iGameEntity *pEnemy = *it;
			if(pEnemy == apEntity)
			{
				mlstGameEnemies.erase(it);
				break;
			}
		}
	}

	hplDelete( apEntity );
}

// Takes an entity out at once, not after the next update
void cMapHandler::DestroyGameEntityNow(iGameEntity *apEntity)
{
	for(tGameEntityMapIt it = m_mapGameEntities.begin(); it != m_mapGameEntities.end(); ++it)
	{
		if(it->second == apEntity){ m_mapGameEntities.erase(it); break; }
	}
	mlstGameEnemies.remove_if([apEntity](iGameEnemy *pEnemy){ return (iGameEntity*)pEnemy == apEntity; });
	if(mpLatestEntity == apEntity) mpLatestEntity = nullptr;
	hplDelete( apEntity );
}

iGameEntity* cMapHandler::GetGameEntity(const tString &asName, bool abErrorMessage)
{
	tGameEntityMapIt it = m_mapGameEntities.find(EntityKey(asName));
	if(it == m_mapGameEntities.end())
	{
		if(abErrorMessage) Error("Couldn't find game entity '%s'\n",asName.c_str());
		return nullptr;
	}

	return it->second;
}

//-----------------------------------------------------------------------

tGameEntityIterator cMapHandler::GetGameEntityIterator()
{
	return tGameEntityIterator(&m_mapGameEntities);
}

//-----------------------------------------------------------------------

iGameEntity* cMapHandler::GetLatestEntity()
{
	return mpLatestEntity;
}

//-----------------------------------------------------------------------

void cMapHandler::SaveToGlobal(cMapHandler_GlobalSave *apSave)
{
	cMapHandler_GlobalSave *pData = apSave;
	
	/////////////////
	//Variables
	kSaveData_SaveTo(mfGameTime);
	kSaveData_SaveTo(msCurrentMap);
	
	/////////////////
	//Containers

	//Loaded maps
	pData->mvLoadedMaps.Resize(mvLoadedMaps.size());
	for(size_t i=0; i < mvLoadedMaps.size(); ++i)
	{
		pData->mvLoadedMaps[i].mfTime = mvLoadedMaps[i].mfTime;
		pData->mvLoadedMaps[i].msName = mvLoadedMaps[i].msName;
	}
	
	//Timers
	tGameTimerListIt it = mlstTimers.begin();
	for(; it != mlstTimers.end(); ++it)
	{
		cGameTimer *pTimer = *it;
		cMapHandlerTimer_GlobalSave timerSave;
		
		if(pTimer->mbGlobal)
		{
			timerSave.mfTime = pTimer->mfTime;
			timerSave.msName = pTimer->msName;
			timerSave.msCallback = pTimer->msCallback;

			timerSave.mbGlobal = pTimer->mbGlobal;

			timerSave.mbDeleteMe = pTimer->mbDeleteMe;
			timerSave.mbPaused = pTimer->mbPaused;

			pData->mlstTimers.Add(timerSave);
		}
	}
}

void cMapHandler::LoadFromGlobal(cMapHandler_GlobalSave *apSave)
{
	cMapHandler_GlobalSave *pData = apSave;
	
	/////////////////
	//Variables
	kSaveData_LoadFrom(mfGameTime);
	//Should not be loadead so that current map is ""
	//kSaveData_LoadFrom(msCurrentMap);
	
	/////////////////
	//Containers
	
	//Loaded maps
	mvLoadedMaps.resize(pData->mvLoadedMaps.Size());
	for(size_t i=0; i < mvLoadedMaps.size(); ++i)
	{
		mvLoadedMaps[i].mfTime = pData->mvLoadedMaps[i].mfTime;
		mvLoadedMaps[i].msName = pData->mvLoadedMaps[i].msName;
	}
	
	//Timers
	cContainerListIterator<cMapHandlerTimer_GlobalSave> it = pData->mlstTimers.GetIterator();
	while(it.HasNext())
	{
		cGameTimer *pTimer = hplNew( cGameTimer, () );
		cMapHandlerTimer_GlobalSave &timerSave = it.Next();

		pTimer->mfTime = timerSave.mfTime;
		pTimer->msName = timerSave.msName;
		pTimer->msCallback = timerSave.msCallback;

		pTimer->mbGlobal = timerSave.mbGlobal;

		pTimer->mbDeleteMe = timerSave.mbDeleteMe;
		pTimer->mbPaused = timerSave.mbPaused;

		mlstTimers.push_back(pTimer);
	}
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
//////////////////////////////////////////////////////////////////////////

void cMapHandler::PrintSoundsPlaying()
{
	Log("Sounds: ");
	tSoundEntryList *pEntryList = mpInit->mpGame->GetSound()->GetSoundHandler()->GetWorldEntryList();
	for(tSoundEntryListIt it = pEntryList->begin(); it != pEntryList->end();++it)
	{
		iSoundChannel *pSound = it->mpSound;
		
        Log("'%s', ",pSound->GetData()->GetName().c_str());	
	}
	
    Log("\n");
}
//-----------------------------------------------------------------------

void cMapHandler::UpdateTimers(float afTimeStep)
{
	for(tGameTimerListIt it=mlstTimers.begin(); it != mlstTimers.end(); )
	{
		cGameTimer *pTimer = *it;

        if(pTimer->mbDeleteMe)
		{
			it = mlstTimers.erase(it);
			hplDelete( pTimer );
		}
		else
		{
			if(pTimer->mbPaused==false)
				pTimer->mfTime -= afTimeStep;

			if(pTimer->mfTime <= 0)
			{
				tString sCommand = pTimer->msCallback + "(\""+pTimer->msName+"\")";
				mpInit->RunScriptCommand(sCommand);	

				it = mlstTimers.erase(it);
				hplDelete( pTimer );
			}
			else
			{
				++it;
			}
		}
	}
}

void cMapHandler::SetLocalTimersPaused(bool abPaused)
{
	for(cGameTimer *pTimer : mlstTimers)
	{
		if(pTimer->mbGlobal==false) pTimer->mbPaused = abPaused;
	}
}

//-----------------------------------------------------------------------

void cMapHandler::RemoveLocalTimers()
{
	for(tGameTimerListIt it=mlstTimers.begin(); it != mlstTimers.end(); )
	{
		cGameTimer *pTimer = *it;

		if(pTimer->mbGlobal==false)
		{
			it = mlstTimers.erase(it);
			hplDelete( pTimer );
		}
		else
		{
			++it;
		}
	}
}

//-----------------------------------------------------------------------


void cMapHandler::PreUpdate(double afSecondsSinceVisit)
{
	cWorld3D* pWorld = mpScene->GetWorld3D();
	iPhysicsWorld *pPhysicsWorld = pWorld->GetPhysicsWorld();

	mbPreUpdating = true;

	unsigned long lStart = mpInit->mpGame->GetSystem()->GetLowLevel()->GetTime();

	//Enable all physic bodies
	cPhysicsBodyIterator bodyIt = pPhysicsWorld->GetBodyIterator();
	while(bodyIt.HasNext())
	{
		iPhysicsBody *pBody = bodyIt.Next();
		pBody->SetEnabled(true);
	}
	
	//////////////////////////////////////
	//Pre update game stuff
	if(afSecondsSinceVisit ==0)
	{
		pWorld->PreUpdate(2.0f, mpInit->mpGame->GetStepSize());
	}
	else
	{
		float fTime = (float)afSecondsSinceVisit;
		if(fTime > 3) fTime = 3;
		float fStepSize = mpInit->mpGame->GetStepSize();
        
		mpInit->mpGame->GetSound()->GetSoundHandler()->SetSilent(true);

		while(fTime>0)
		{
			//if(pWorld->GetPhysicsWorld()) pWorld->GetPhysicsWorld()->Update(fStepSize);
			//pWorld->UpdateParticles(fStepSize);
			pWorld->Update(fStepSize);

			tGameEnemyListIt it = mlstGameEnemies.begin();
			for(; it != mlstGameEnemies.end(); ++it)
			{
				iGameEnemy *pEnemy = *it;
				pEnemy->Update(fStepSize);
			}

			fTime -= fStepSize;
		}

		mpInit->mpGame->GetSound()->GetSoundHandler()->SetSilent(false);
	}

	unsigned long lTime = mpInit->mpGame->GetSystem()->GetLowLevel()->GetTime() - lStart;

	//Log("PREUPDATE time: %d\n",lTime);
	
	mbPreUpdating = false;
}

//-----------------------------------------------------------------------

double cMapHandler::AddLoadedMap(cWorld3D *apWorld)
{
	//Check if map is loaded
	for(size_t i=0; i< mvLoadedMaps.size();++i)
	{
		if(mvLoadedMaps[i].msName == apWorld->GetName())
		{
			double fTime = mvLoadedMaps[i].mfTime;
			mvLoadedMaps[i].mfTime = GetGameTime();
			return GetGameTime() - fTime;
		}
	}

	cLoadedMap LoadedMap;
	LoadedMap.msName = apWorld->GetName();
	LoadedMap.mfTime = GetGameTime();

	mvLoadedMaps.push_back(LoadedMap);
	return 0;
}

//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// SAVE DATA
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------



//-----------------------------------------------------------------------
