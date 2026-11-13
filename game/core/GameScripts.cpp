// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "core/StdAfx.h"
#include "core/GameScripts.h"

#include "core/Init.h"
#include "core/MapHandler.h"
#include "core/ButtonHandler.h"
#include "player/Player.h"
#include "player/PlayerHelper.h"
#include "entities/GameEntity.h"
#include "entities/GameSwingDoor.h"
#include "entities/GameItem.h"
#include "entities/GameObject.h"
#include "entities/GameArea.h"
#include "entities/GameStickArea.h"
#include "enemies/GameEnemy.h"
#include "enemies/GameEnemy_Infected.h"
#include "entities/GameLink.h"
#include "entities/GameLamp.h"
#include "entities/GameLever.h"
#include "entities/GameCamera.h"
#include "entities/GameWheel.h"
#include "entities/GameLadder.h"
#include "entities/GameDamageArea.h"
#include "entities/GameForceArea.h"
#include "entities/GameLiquidArea.h"
#include "entities/GameSaveArea.h"
#include "systems/GameMessageHandler.h"
#include "systems/RadioHandler.h"
#include "ui/Inventory.h"
#include "ui/Notebook.h"
#include "ui/GameGui.h"
#include "ui/Ending.h"
#include "systems/GameSoundEnvHandler.h"
#include "ui/NumericalPanel.h"
#include "systems/FadeHandler.h"
#include "systems/EffectHandler.h"
#include "systems/AttackHandler.h"
#include "core/SaveHandler.h"
#include "core/GraphicsHelper.h"
#include "ui/MainMenu.h"
#include "systems/GameMusicHandler.h"
#include "ui/Credits.h"
#include "ui/DemoEndText.h"

#include "core/GlobalInit.h"

// SCRIPT FUNCTIONS

// Maps' scripts name entities that may be missing or of another type: the
// function warns and does nothing. asWhat names the expected kind in the
// warning; eGameEntityType_LastEnum accepts any type.
template<class T = iGameEntity>
static T* FindScriptEntity(const tString &asName, const char *asWhat,
							eGameEntityType aType = eGameEntityType_LastEnum)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr || (aType != eGameEntityType_LastEnum && pEntity->GetType() != aType))
	{
		Warning("Couldn't find %s '%s'\n",asWhat,asName.c_str());
		return nullptr;
	}
	return static_cast<T*>(pEntity);
}

static cGameTimer* FindScriptTimer(const tString &asName)
{
	cGameTimer *pTimer = gpInit->mpMapHandler->GetTimer(asName);
	if(pTimer==nullptr) Warning("Couldn't find timer '%s'\n",asName.c_str());
	return pTimer;
}


tWString gsTempString = _W("");
// The key just added was in quotes or brackets (Overture's "LeftMouse" and
// (Tab)): the closing one, at the start of the next text added, is dropped too
wchar_t gwcTempStringKeyClose = 0;

static void AddToTempStringText(const tWString &asText)
{
	if(gwcTempStringKeyClose && asText.size() > 0 && asText[0] == gwcTempStringKeyClose)
		gsTempString += asText.substr(1);
	else
		gsTempString += asText;
	gwcTempStringKeyClose = 0;
}

static void AddToTempString(std::string asString)
{
    AddToTempStringText(cString::To16Char(asString));
}

static void AddToTempStringTrans(std::string asCat,std::string asEntry)
{
	cInit *mpInit = gpInit;
	AddToTempStringText(kTranslate(asCat,asEntry));
}

// The key is drawn as a key cap, which needs no quotes or brackets around it
static void AddToTempStringAction(std::string asAction)
{
	wchar_t lClose = 0;
	if(gsTempString.size() > 0)
	{
		wchar_t lOpen = gsTempString[gsTempString.size()-1];
		if(lOpen == _W('"')) lClose = _W('"');
		else if(lOpen == _W('(')) lClose = _W(')');
	}
	if(lClose) gsTempString.resize(gsTempString.size()-1);

	gsTempString += gpInit->mpButtonHandler->GetActionKeyCap(asAction);
	gwcTempStringKeyClose = lClose;
}

// GENERAL GAME

static void ResetGame()
{
	gpInit->mpGame->ResetLogicTimer();

	gpInit->mpMapHandler->SetCurrentMapName("");
	gpInit->mpMainMenu->SetActive(true);
}

static void StartCredits()
{
	gpInit->mpCredits->SetActive(true);
}

static void StartDemoEndText()
{
	gpInit->mpDemoEndText->SetActive(true);
}

static void ClearSavedMaps()
{
	gpInit->mpSaveHandler->ClearSavedMaps();
}

static std::string GetActionKeyString(std::string asAction)
{
	iAction *pAction = gpInit->mpGame->GetInput()->GetAction(asAction);
	if(pAction)
	{
		// The same short name as tips and the inventory
		return cString::To8Char(gpInit->mpButtonHandler->GetKeyLabel(pAction->GetInputName()));
	}
	
	return "ActionNotFound";
}

static void AddMessageTrans(std::string asTransCat,
									  std::string asTransName)
{
	cInit *mpInit = gpInit;
	gpInit->mpGameMessageHandler->Add(kTranslate(asTransCat,asTransName));
}

static void AddMessage(std::string asMessage)
{
	gpInit->mpGameMessageHandler->Add(cString::To16Char(asMessage));
}

static void AddMessageTempString()
{
	gpInit->mpGameMessageHandler->Add(gsTempString);
	gsTempString = _W("");
	gwcTempStringKeyClose = 0;
}

static void AddSubTitleTrans(std::string asTransCat,
									  std::string asTransName, float afTime)
{
	cInit *mpInit = gpInit;
	gpInit->mpEffectHandler->GetSubTitle()->Add(kTranslate(asTransCat,asTransName),afTime,false);
}

static void AddSubTitle(std::string asMessage, float afTime)
{
	gpInit->mpEffectHandler->GetSubTitle()->Add(cString::To16Char(asMessage),afTime,false);
}

static void AddSubTitleTempString(float afTime)
{
	gpInit->mpEffectHandler->GetSubTitle()->Add(gsTempString,afTime,false);
	gsTempString = _W("");
	gwcTempStringKeyClose = 0;
}

static void AddRadioMessage(	std::string asTransCat,std::string asTransName, 
												std::string asSound)
{
	cInit *mpInit = gpInit;
	gpInit->mpRadioHandler->Add(kTranslate(asTransCat,asTransName),asSound);
}

static void SetRadioOnEndCallback(std::string asFunc)
{
	gpInit->mpRadioHandler->SetOnEndCallback(asFunc);
}

static void SetInventoryMessage(std::string asMessage)
{
	gpInit->mpInventory->SetMessage(cString::To16Char(asMessage));
}

static void SetInventoryMessageTrans(std::string asTransCat,
											   std::string asTransName)
{
	cInit *mpInit = gpInit;
	gpInit->mpInventory->SetMessage(kTranslate(asTransCat,asTransName));
}

static void SetMessagesOverCallback(std::string asFunction)
{
	gpInit->mpGameMessageHandler->SetOnMessagesOverCallback(asFunction);
}

static void ChangeMap(std::string asMapFile, std::string asMapPos, 
								std::string asStartSound, std::string asStopSound,
								float afFadeOutTime, float afFadeInTime,
								std::string asLoadTextCat, std::string asLoadTextEntry)
{

	gpInit->mpMapHandler->ChangeMap(asMapFile,asMapPos, asStartSound,asStopSound,
									afFadeOutTime,afFadeInTime,
									asLoadTextCat,asLoadTextEntry);
}

// Black Plague: with the picture of the loading screen
static void ChangeMapLoadImage(std::string asMapFile, std::string asMapPos,
								std::string asStartSound, std::string asStopSound,
								float afFadeOutTime, float afFadeInTime,
								std::string asLoadTextCat, std::string asLoadTextEntry,
								std::string asLoadImage)
{
	gpInit->mpMapHandler->ChangeMap(asMapFile, asMapPos,asStartSound, asStopSound,
									afFadeOutTime, afFadeInTime, asLoadTextCat, asLoadTextEntry, asLoadImage);
}

static void SetMapGameName(std::string asName)
{
	gpInit->mpMapHandler->SetMapGameName(cString::To16Char(asName));
}

static void SetMapGameNameTrans(std::string asTransCat,std::string asTransEntry)
{
	cInit *mpInit = gpInit;
	gpInit->mpMapHandler->SetMapGameName(kTranslate(asTransCat,asTransEntry));
}

static void AddNotebookTaskText(std::string asName,std::string asText) 
{
	gpInit->mpNotebook->AddTask(asName, cString::To16Char(asText));
}

static void AddNotebookTask(std::string asName, std::string asTransCat,
									  std::string asTransEntry)
{
	cInit *mpInit = gpInit;
	gpInit->mpNotebook->AddTask(asName, kTranslate(asTransCat,asTransEntry));
}	

static void RemoveNotebookTask(std::string asName)
{
	gpInit->mpNotebook->RemoveTask(asName);
}

static void AddNotebookNote(std::string asNameCat, std::string asNameEntry,
											 std::string asTextCat,	std::string asTextEntry)
{
	cInit *mpInit = gpInit;
	gpInit->mpNotebook->AddNote(kTranslate(asNameCat,asNameEntry),asTextCat,asTextEntry);
}

static void StartNumericalPanel(std::string asName,int alCode1,int alCode2,int alCode3,int alCode4,
										  float afDifficulty, std::string asCallback)
{
	gpInit->mpNumericalPanel->SetActive(true);
	tIntVec vCode; vCode.resize(4);
	vCode[0] = alCode1; vCode[1] = alCode2; vCode[2] = alCode3; vCode[3] = alCode4;
	
	gpInit->mpNumericalPanel->SetUp(asName,asCallback);
	gpInit->mpNumericalPanel->SetCode(vCode);
}

static void SetInventoryActive(bool abActive)
{
	gpInit->mpInventory->SetActive(abActive);
}

static void FadeIn(float afTime)
{
	gpInit->mpFadeHandler->FadeIn(afTime);
}

static void FadeOut(float afTime)
{
	gpInit->mpFadeHandler->FadeOut(afTime);
}

static bool IsFading()
{
	return gpInit->mpFadeHandler->IsActive();
}

static void SetWideScreenActive(bool abActive)
{
	gpInit->mpFadeHandler->SetWideScreenActive(abActive);
}

static void AutoSave()
{
	if(gpInit->mpPlayer->GetHealth()<=0) return;

	//TODO: Some other effect here.
	gpInit->mpEffectHandler->GetSaveEffect()->AutoSave();
}

static void StartFlash(float afFadeIn, float afWhite, float afFadeOut)
{
	gpInit->mpEffectHandler->GetFlash()->Start(afFadeIn,afWhite,afFadeOut);
}

static void SetWaveGravityActive(bool abActive)
{
	gpInit->mpEffectHandler->GetWaveGravity()->SetActive(abActive);
}

static void SetupWaveGravity(float afMaxAngle, float afSwingLength, 
									   float afGravitySize, std::string asAxis)
{
	int lDir = cString::ToLowerCase(asAxis) == "x" ? 0 : 1;

	gpInit->mpEffectHandler->GetWaveGravity()->Setup(cMath::ToRad(afMaxAngle),afSwingLength,afGravitySize,lDir);
}

static void SetDepthOfFieldActive(bool abActive, float afFadeTime)
{
	gpInit->mpEffectHandler->GetDepthOfField()->SetActive(abActive,afFadeTime);
}

static void SetupDepthOfField(float afNearPlane, float afFocalPlane, float afFarPlane)
{
	gpInit->mpEffectHandler->GetDepthOfField()->SetUp(afNearPlane,afFocalPlane,afFarPlane);
	gpInit->mpEffectHandler->GetDepthOfField()->SetFocusBody(nullptr);
}

static void FocusOnEntity(std::string asEntity)
{
	iGameEntity *pEntity = FindScriptEntity(asEntity, "game entity");
	if(pEntity==nullptr) return;
	
	if(pEntity->GetBodyNum()==0) {
		Error("Entity %s had no bodies and can not be focus on.\n",pEntity->GetName().c_str());
		return;
	}

	gpInit->mpEffectHandler->GetDepthOfField()->FocusOnBody(pEntity->GetBody(0));
	gpInit->mpEffectHandler->GetDepthOfField()->SetFocusBody(nullptr);
}

static void SetConstantFocusOnEntity(std::string asEntity)
{
	if(asEntity=="")
	{
		gpInit->mpEffectHandler->GetDepthOfField()->SetFocusBody(nullptr);
		return;
	}

	iGameEntity *pEntity = FindScriptEntity(asEntity, "game entity");
	if(pEntity==nullptr) return;

	if(pEntity->GetBodyNum()==0) {
		Error("Entity %s had no bodies and can not be focus on.\n",pEntity->GetName().c_str());
			return;
	}

	gpInit->mpEffectHandler->GetDepthOfField()->SetFocusBody(pEntity->GetBody(0));
}

static void PlayGameMusic(	std::string asFile, float afVolume,float afFadeStep,
											bool abLoop, int alPrio)
{
	gpInit->mpMusicHandler->Play(asFile,abLoop,afVolume,afFadeStep,alPrio);
}

static void StopGameMusic(float afFadeStep, int alPrio)
{
	gpInit->mpMusicHandler->Stop(afFadeStep,alPrio);
}

static void StartScreenShake(float afAmount, float afTime,float afFadeInTime,float afFadeOutTime)
{
	gpInit->mpEffectHandler->GetShakeScreen()->Start(afAmount,afTime,afFadeInTime,afFadeOutTime);
}

static void CreateLightFlashAtArea(std::string asArea, float afRadius,
											 float afR,float afG, float afB, float afA,
											 float afAddTime, float afNegTime)
{
	cAreaEntity *pArea = gpInit->mpGame->GetScene()->GetWorld3D()->GetAreaEntity(asArea);
	if(pArea==nullptr)
	{
		Error("Could not find area '%s'\n",asArea.c_str());
		return;
	}

    gpInit->mpMapHandler->AddLightFlash(pArea->m_mtxTransform.GetTranslation(),afRadius,
										cColor(afR,afG,afB,afA),afAddTime,afNegTime);
}

// ATTACK

static void CreateSplashDamage(std::string asAreaName, float afRadius, 
										 float afMinDamage, float afMaxDamage,
										 float afMinForce, float afMaxForce,
										 float afMaxImpulse, int alStrength)
{	
	//Get area
	cGameArea *pArea = FindScriptEntity<cGameArea>(asAreaName, "area entity", eGameEntityType_Area);
	if(pArea==nullptr) return;

    gpInit->mpAttackHandler->CreateSplashDamage(pArea->GetBody(0)->GetWorldPosition(),
												afRadius,afMinDamage,afMaxDamage,
												afMinForce, afMaxForce, afMaxImpulse,
												eAttackTargetFlag_Bodies | 
												eAttackTargetFlag_Player |
												eAttackTargetFlag_Enemy,
												1, alStrength);
}

// GAME TIMER

static void CreateTimer(std::string asName, float afTime, std::string asCallback, bool abGlobal)
{
	gpInit->mpMapHandler->CreateTimer(asName,afTime,asCallback,abGlobal);
}

static void DestroyTimer(std::string asName)
{
	cGameTimer *pTimer = FindScriptTimer(asName);
	if(pTimer==nullptr) return;
	
	pTimer->mbDeleteMe = true;
}

static void SetTimerPaused(std::string asName, bool abPaused)
{
	cGameTimer *pTimer = FindScriptTimer(asName);
	if(pTimer==nullptr) return;

	pTimer->mbPaused = abPaused;
}

static void SetTimerTime(std::string asName, float afTime)
{
	cGameTimer *pTimer = FindScriptTimer(asName);
	if(pTimer==nullptr) return;

	pTimer->mfTime = afTime;
}

static void AddTimerTime(std::string asName, float afTime)
{
	cGameTimer *pTimer = FindScriptTimer(asName);
	if(pTimer==nullptr) return;

	pTimer->mfTime += afTime;
}

static float GetTimerTime(std::string asName)
{
	cGameTimer *pTimer = FindScriptTimer(asName);
	if(pTimer==nullptr) return 0.0f;

	return pTimer->mfTime;
}

// PLAYER

static void GivePlayerDamage(float afAmount, std::string asType)
{
	ePlayerDamageType type = ePlayerDamageType_BloodSplash;
	tString sLowType = cString::ToLowerCase(asType);

	if(sLowType == "bloodsplash") type = ePlayerDamageType_BloodSplash;
	else if(sLowType == "ice") type = ePlayerDamageType_Ice;
	else 
		Warning("Damage type %s does not exist!\n",asType.c_str());



	gpInit->mpPlayer->Damage(afAmount,type);
}

static void SetPlayerHealth(float afHealth)
{
	gpInit->mpPlayer->SetHealth(afHealth);
}

static float GetPlayerHealth()
{
	return gpInit->mpPlayer->GetHealth();
}

static void SetPlayerPose(std::string asPose,bool abChangeDirectly)
{
	tString sPose = cString::ToLowerCase(asPose);

	if(sPose == "stand")
	{
		gpInit->mpPlayer->ChangeMoveState(ePlayerMoveState_Walk,abChangeDirectly);
	}
	else if(sPose == "crouch")
	{
		gpInit->mpPlayer->ChangeMoveState(ePlayerMoveState_Crouch,abChangeDirectly);
	}
	else
	{
		Warning("Player pose mode '%s' does not exist\n",asPose.c_str());
	}
}

static void SetPlayerActive(bool abActive)
{
	gpInit->mpPlayer->SetActive(abActive);
	if(cInit::IsOverture()==false) gpInit->mpGameMessageHandler->ClearMessages();
}

static void StartPlayerLookAt(std::string asEntityName, float afSpeedMul, float afMaxSpeed)
{
	//Get entity
	iGameEntity *pEntity = FindScriptEntity(asEntityName, "entity");
	if(pEntity==nullptr) return;
	if(pEntity->GetBody(0)==nullptr)
	{
		Warning("Couldn't find a body in entity '%s'\n",asEntityName.c_str());
		return;
	}
	
	gpInit->mpPlayer->GetLookAt()->SetTarget(pEntity->GetBody(0)->GetWorldPosition(),afSpeedMul,afMaxSpeed);
	gpInit->mpPlayer->GetLookAt()->SetActive(true);
}

static void StopPlayerLookAt()
{
	gpInit->mpPlayer->GetLookAt()->SetActive(false);
}

static void StartPlayerFearFilter(float afStrength)
{
	gpInit->mpPlayer->GetFearFilter()->SetActive(true);
}

static void StopPlayerFearFilter()
{
	gpInit->mpPlayer->GetFearFilter()->SetActive(false);
}

static void SetFlashlightDisabled(bool abDisabled)
{
	gpInit->mpPlayer->GetFlashLight()->SetDisabled(abDisabled);
}

// INVENTORY

static void AddPickupCallback(std::string asItem, std::string asFunction)
{
	gpInit->mpInventory->AddPickupCallback(asItem,asFunction);
}

static void AddUseCallback(std::string asItem, std::string asEntity, std::string asFunction)
{
	gpInit->mpInventory->AddUseCallback(asItem,asEntity,asFunction);
}

static void AddCombineCallback(std::string asItem1,std::string asItem2, std::string asFunction)
{
	gpInit->mpInventory->AddCombineCallback(asItem1,asItem2,asFunction);
}

static void RemovePickupCallback(std::string asFunction)
{
	gpInit->mpInventory->RemovePickupCallback(asFunction);
}

static void RemoveUseCallback(std::string asFunction)
{
	gpInit->mpInventory->RemoveUseCallback(asFunction);
}

static void RemoveCombineCallback(std::string asFunction)
{
	gpInit->mpInventory->RemoveCombineCallback(asFunction);
}

static bool HasItem(std::string asName)
{

	return gpInit->mpInventory->GetItem(asName) != nullptr;
}

static void RemoveItem(std::string asName)
{
	cInventoryItem *pItem = gpInit->mpInventory->GetItem(asName);
	if(pItem)
	{
		gpInit->mpInventory->RemoveItem(pItem);
	}
	else
	{
		Warning("Cannot find item '%s' in inventory\n",asName.c_str());
	}
}

static void GiveItem(std::string asName,std::string asEntityFile,int alSlotIndex)
{
	gpInit->mpInventory->AddItemFromFile(asName,asEntityFile, alSlotIndex);
}

// GAME ENTITY PROPERTIES

static void ReplaceEntity(std::string asName, std::string asBodyName,
											   std::string asNewName, std::string asNewFile)
{
	iGameEntity *pEntity = FindScriptEntity(asName, "game entity");
	if(pEntity==nullptr) return;

	if(pEntity->GetBodyNum()==0){
        Error("Entity '%s' contains no bodies!\n",pEntity->GetName().c_str());
		return;
	}

	iPhysicsBody *pBody = nullptr;
	
	if(asBodyName != "" && pEntity->GetBodyNum()>1)
	{
		for(int i= 0; i < pEntity->GetBodyNum(); ++i)
		{
			tString sBodyName = cString::Sub(pEntity->GetBody(i)->GetName(), (int)asName.size() +1);
			if(sBodyName == asBodyName){
				pBody = pEntity->GetBody(i);
				break;
			}
		}

		if(pBody==nullptr)
		{
			Error("Body '%s' could not be found in entity '%s'!\n",asBodyName.c_str(),asName.c_str());
			return;
		}
	}
	else
	{
		//Overture's code declared a second pBody here and crashed; fixed in Black Plague
		pBody = pEntity->GetBody(0);
	}

	cMatrixf mtxTransform = pBody->GetWorldMatrix();

	gpInit->mpMapHandler->RemoveGameEntity(pEntity);

	cWorld3D *pWorld = gpInit->mpGame->GetScene()->GetWorld3D();
	pWorld->CreateEntity(asNewName,mtxTransform,asNewFile, true);
}

static void SetGameEntityActive(std::string asName, bool abActive)
{
	iGameEntity *pEntity = FindScriptEntity(asName, "game entity");
	if(pEntity==nullptr) return;

	pEntity->SetActive(abActive);
}

static bool GetGameEntityActive(std::string asName)
{
	iGameEntity *pEntity = FindScriptEntity(asName, "game entity");
	if(pEntity==nullptr) return false;

	return pEntity->IsActive();
}

static void CreateGameEntityVar(std::string asEntName, std::string asVarName, int alVal)
{
	iGameEntity *pEntity = FindScriptEntity(asEntName, "game entity");
	if(pEntity==nullptr) return;

	pEntity->CreateVar(asVarName,alVal);
}

static void SetGameEntityVar(std::string asEntName, std::string asVarName, int alVal)
{
	iGameEntity *pEntity = FindScriptEntity(asEntName, "game entity");
	if(pEntity==nullptr) return;

	pEntity->SetVar(asVarName,alVal);
}

static void AddGameEntityVar(std::string asEntName, std::string asVarName, int alVal)
{
	iGameEntity *pEntity = FindScriptEntity(asEntName, "game entity");
	if(pEntity==nullptr) return;

	pEntity->AddVar(asVarName,alVal);
}

static int GetGameEntityVar(std::string asEntName, std::string asVarName)
{
	iGameEntity *pEntity = FindScriptEntity(asEntName, "game entity");
	if(pEntity==nullptr) return 0;

	return pEntity->GetVar(asVarName);
}

static void SetGameEntityMaxExamineDist(std::string asName,float afDist)
{
	iGameEntity *pEntity = FindScriptEntity(asName, "game entity");
	if(pEntity==nullptr) return;
	
	pEntity->SetMaxExamineDist(afDist);
}

static void SetGameEntityMaxInteractDist(std::string asName,float afDist)
{
	iGameEntity *pEntity = FindScriptEntity(asName, "game entity");
	if(pEntity==nullptr) return;

	pEntity->SetMaxInteractDist(afDist);
}

static void SetGameEntityDescriptionTrans(std::string asName, 
													std::string asTransCat,
													std::string asTransName)
{
	cInit *mpInit = gpInit;
	iGameEntity *pEntity = FindScriptEntity(asName, "game entity");
	if(pEntity==nullptr) return;

	pEntity->SetDescription(kTranslate(asTransCat,asTransName));
	pEntity->SetShowDescritionOnce(false);
}

static void SetGameEntityDescriptionOnceTrans(std::string asName, 
														   std::string asTransCat,
														   std::string asTransName)
{
	cInit *mpInit = gpInit;
	iGameEntity *pEntity = FindScriptEntity(asName, "game entity");
	if(pEntity==nullptr) return;

	pEntity->SetDescription(kTranslate(asTransCat,asTransName));
	pEntity->SetShowDescritionOnce(true);
}

static void SetGameEntityDescription(std::string asName, std::string asMessage)
{
	iGameEntity *pEntity = FindScriptEntity(asName, "game entity");
	if(pEntity==nullptr) return;

	pEntity->SetDescription(cString::To16Char(asMessage));
	pEntity->SetShowDescritionOnce(false);
}

static void SetGameEntityGameNameTrans(std::string asName, 
												 std::string asTransCat,
												 std::string asTransName)
{
	cInit *mpInit = gpInit;
	iGameEntity *pEntity = FindScriptEntity(asName, "game entity");
	if(pEntity==nullptr) return;

	pEntity->SetGameName(kTranslate(asTransCat,asTransName));
}

static void ChangeEntityAnimation(std::string asName,
											std::string asAnimation,
											bool abLoop)
{
	iGameEntity *pEntity = FindScriptEntity(asName, "game entity");
	if(pEntity==nullptr) return;

 	pEntity->GetMeshEntity()->PlayName(asAnimation,abLoop, true);
}

static void SetEntityHealth(std::string asName, float afHealth)
{
	iGameEntity *pEntity = FindScriptEntity(asName, "game entity");
	if(pEntity==nullptr) return;

	pEntity->SetHealth(afHealth);
}

static void DamageEntity(std::string asName, float afDamage, int alStrength)
{
	iGameEntity *pEntity = FindScriptEntity(asName, "game entity");
	if(pEntity==nullptr) return;

	pEntity->Damage(afDamage,alStrength);
}

static void SetDoorState(std::string asName, 
											std::string asState)
{
}

static void SetObjectInteractMode(std::string asName,std::string asMode)
{
	cGameObject *pObject = FindScriptEntity<cGameObject>(asName, "object entity", eGameEntityType_Object);
	if(pObject==nullptr) return;

	pObject->SetInteractMode(cEntityLoader_GameObject::ToInteractMode(asMode.c_str()));
}

static void SetupLink(std::string asName,
								std::string asMapFile, std::string asMapPos, 
								std::string asStartSound, std::string asStopSound,
								float afFadeOutTime, float afFadeInTime)
{
	cGameLink *pLink = FindScriptEntity<cGameLink>(asName, "object entity", eGameEntityType_Link);
	if(pLink==nullptr) return;

	pLink->msMapFile = asMapFile;
	pLink->msMapPos = asMapPos;
	pLink->msStartSound = asStartSound;
	pLink->msStopSound = asStopSound;
	pLink->mfFadeInTime = afFadeInTime;
	pLink->mfFadeOutTime = afFadeOutTime;
	pLink->msLoadTextCat = "";
	pLink->msLoadTextEntry = "";
}

// Black Plague: with the picture of the loading screen
static void SetupLinkLoadImage(std::string asName,
								std::string asMapFile, std::string asMapPos,
								std::string asStartSound, std::string asStopSound,
								float afFadeOutTime, float afFadeInTime,
								std::string asLoadImage)
{
	SetupLink(asName, asMapFile, asMapPos, asStartSound, asStopSound, afFadeOutTime, afFadeInTime);

	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName, false);
	if(pEntity && pEntity->GetType() == eGameEntityType_Link)
		static_cast<cGameLink*>(pEntity)->msLoadImage = asLoadImage;
}

static void SetupLinkLoadText(std::string asName,
										std::string asMapFile, std::string asMapPos, 
										std::string asStartSound, std::string asStopSound,
										float afFadeOutTime, float afFadeInTime,
										std::string asTextCat, std::string asTextEntry)
{
	cGameLink *pLink = FindScriptEntity<cGameLink>(asName, "object entity", eGameEntityType_Link);
	if(pLink==nullptr) return;

	pLink->msMapFile = asMapFile;
	pLink->msMapPos = asMapPos;
	pLink->msStartSound = asStartSound;
	pLink->msStopSound = asStopSound;
	pLink->mfFadeInTime = afFadeInTime;
	pLink->mfFadeOutTime = afFadeOutTime;
	pLink->msLoadTextCat = asTextCat;
	pLink->msLoadTextEntry = asTextEntry;
}

static void SetupLinkLoadTextLoadImage(std::string asName,
										std::string asMapFile, std::string asMapPos,
										std::string asStartSound, std::string asStopSound,
										float afFadeOutTime, float afFadeInTime,
										std::string asTextCat, std::string asTextEntry,
										std::string asLoadImage)
{
	SetupLinkLoadText(asName, asMapFile, asMapPos, asStartSound, asStopSound, afFadeOutTime, afFadeInTime,
						asTextCat, asTextEntry);

	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName, false);
	if(pEntity && pEntity->GetType() == eGameEntityType_Link)
		static_cast<cGameLink*>(pEntity)->msLoadImage = asLoadImage;
}

/**
 * Black Plague: a locked link does not change the map
 */
static void SetLinkLocked(std::string asName, bool abLocked)
{
	cGameLink *pLink = FindScriptEntity<cGameLink>(asName, "object entity", eGameEntityType_Link);
	if(pLink) pLink->mbLocked = abLocked;
}

/**
 * Black Plague: the sound and the message of a locked link
 */
static void SetupLinkLockedProp(std::string asName, std::string asLockedSound,
											std::string asLockedDescCat, std::string asLockedDescEntry)
{
	cGameLink *pLink = FindScriptEntity<cGameLink>(asName, "object entity", eGameEntityType_Link);
	if(pLink==nullptr) return;

	pLink->msLockedSound = asLockedSound;
	if(asLockedDescCat != "")
		pLink->msLockedDesc = gpInit->mpGame->GetResources()->Translate(asLockedDescCat, asLockedDescEntry);
}

static void SetAreaCustomIcon(std::string asName, std::string asIcon)
{
	cGameArea *pArea = FindScriptEntity<cGameArea>(asName, "area entity", eGameEntityType_Area);
	if(pArea==nullptr) return;

	eCrossHairState aCrosshair = eCrossHairState_None;
	
	tString sIconName = cString::ToLowerCase(asIcon);

	if(sIconName == "active") aCrosshair = eCrossHairState_Active;
	else if(sIconName == "inactive") aCrosshair = eCrossHairState_Inactive;
	else if(sIconName == "invalid") aCrosshair = eCrossHairState_Invalid;
	else if(sIconName == "grab") aCrosshair = eCrossHairState_Grab;
	else if(sIconName == "examine") aCrosshair = eCrossHairState_Examine;
	else if(sIconName == "pointer") aCrosshair = eCrossHairState_Pointer;
	else if(sIconName == "item") aCrosshair = eCrossHairState_Item;
	else if(sIconName == "doorlink") aCrosshair = eCrossHairState_DoorLink;
	else if(sIconName == "pickup") aCrosshair = eCrossHairState_PickUp;
	else if(sIconName == "none") aCrosshair = eCrossHairState_None;
	else Warning("Icon type %s not found!\n",asIcon.c_str());

	pArea->SetCustomIcon(aCrosshair);

}

static void AddEnemyPatrolNode(std::string asEnemy,std::string asNode,float afTime,
														std::string asAnimation)
{
	iGameEnemy *pEnemy = FindScriptEntity<iGameEnemy>(asEnemy, "enemy entity", eGameEntityType_Enemy);
	if(pEnemy==nullptr) return;
    
    pEnemy->AddPatrolNode(asNode,afTime,asAnimation);
}

/**
* Clears all the patrol nodes for the enemy
* \param asEnemy The Name of the enemy.
*/
static void ClearEnemyPatrolNodes(std::string asEnemy)
{
	iGameEnemy *pEnemy = FindScriptEntity<iGameEnemy>(asEnemy, "enemy entity", eGameEntityType_Enemy);
	if(pEnemy==nullptr) return;
	
	pEnemy->ClearPatrolNodes();
}

static void SetEnemyDeathCallback(std::string asEnemy,std::string asFunction)
{
	iGameEnemy *pEnemy = FindScriptEntity<iGameEnemy>(asEnemy, "enemy entity", eGameEntityType_Enemy);
	if(pEnemy==nullptr) return;

	pEnemy->SetOnDeathCallback(asFunction);
}

static void SetEnemyAttackCallback(std::string asEnemy,std::string asFunction)
{
	iGameEnemy *pEnemy = FindScriptEntity<iGameEnemy>(asEnemy, "enemy entity", eGameEntityType_Enemy);
	if(pEnemy==nullptr) return;

	pEnemy->SetOnAttackCallback(asFunction);
}

static float GetEnemyHealth(std::string asEnemy)
{
	iGameEnemy *pEnemy = FindScriptEntity<iGameEnemy>(asEnemy, "enemy entity", eGameEntityType_Enemy);
	if(pEnemy==nullptr) return 0;

	return pEnemy->GetHealth();
}

static void SetEnemyUseTriggers(std::string asEnemy, bool abUseTriggers)
{
	iGameEnemy *pEnemy = FindScriptEntity<iGameEnemy>(asEnemy, "enemy entity", eGameEntityType_Enemy);
	if(pEnemy==nullptr) return;

	pEnemy->SetUsesTriggers(abUseTriggers);
}

// Black Plague's enemy functions

static cGameEnemy_Infected* GetScriptInfected(const std::string &asEnemy)
{
	iGameEnemy *pEnemy = FindScriptEntity<iGameEnemy>(asEnemy, "enemy entity", eGameEntityType_Enemy);
	if(pEnemy==nullptr) return nullptr;

	cGameEnemy_Infected *pInfected = dynamic_cast<cGameEnemy_Infected*>(pEnemy);
	if(pInfected==nullptr) Warning("Enemy '%s' is not an Infected\n",asEnemy.c_str());
	return pInfected;
}

/**
 * Plays an animation in place of the enemy's moving ones.
 **/
static void PlayEnemyAnim(std::string asEnemy, std::string asAnim, bool abLoop)
{
	iGameEnemy *pEnemy = FindScriptEntity<iGameEnemy>(asEnemy, "enemy entity", eGameEntityType_Enemy);
	if(pEnemy==nullptr) return;

	pEnemy->PlayAnim(asAnim,abLoop,0.5f);
}

/**
 * Called once, as asFunc(string asEnemy), when the animation being played is over.
 **/
static void SetEnemyAnimOverCallback(std::string asEnemy, std::string asFunc)
{
	iGameEnemy *pEnemy = FindScriptEntity<iGameEnemy>(asEnemy, "enemy entity", eGameEntityType_Enemy);
	if(pEnemy==nullptr) return;

	pEnemy->SetOnAnimOverCallback(asFunc);
}

/**
 * Called, as asFunc(string asEnemy), each time the enemy starts to hunt the player.
 **/
static void SetEnemyHuntCallback(std::string asEnemy, std::string asFunc)
{
	iGameEnemy *pEnemy = FindScriptEntity<iGameEnemy>(asEnemy, "enemy entity", eGameEntityType_Enemy);
	if(pEnemy==nullptr) return;

	pEnemy->SetOnHuntCallback(asFunc);
}

/**
 * Called once, as asFunc(string asEnemy), when the enemy has been at its last patrol node.
 **/
static void SetEnemyPatrolOverCallback(std::string asEnemy, std::string asFunc)
{
	iGameEnemy *pEnemy = FindScriptEntity<iGameEnemy>(asEnemy, "enemy entity", eGameEntityType_Enemy);
	if(pEnemy==nullptr) return;

	pEnemy->SetOnPatrolOverCallback(asFunc);
}

/**
 * The patrol is over at its last node instead of starting again.
 **/
static void SetEnemyStopAtLastPatrolNode(std::string asEnemy, bool abStop)
{
	iGameEnemy *pEnemy = FindScriptEntity<iGameEnemy>(asEnemy, "enemy entity", eGameEntityType_Enemy);
	if(pEnemy==nullptr) return;

	pEnemy->SetStopAtLastPatrolNode(abStop);
}

/**
 * Puts a weapon in an Infected's hand. The rotation is in degrees.
 **/
static void SetInfectedWeapon(std::string asEnemy, std::string asFile, float afDamage,
										float afRotX, float afRotY, float afRotZ,
										float afPosX, float afPosY, float afPosZ)
{
	cGameEnemy_Infected *pInfected = GetScriptInfected(asEnemy);
	if(pInfected==nullptr) return;

	cMatrixf mtxOffset = cMath::MatrixRotate(cVector3f(cMath::ToRad(afRotX),cMath::ToRad(afRotY),cMath::ToRad(afRotZ)),
											eEulerRotationOrder_XYZ);
	mtxOffset.SetTranslation(cVector3f(afPosX,afPosY,afPosZ));

	pInfected->SetWeapon(asFile,mtxOffset,afDamage,true);
}

/**
 * Gives an Infected a lit flashlight to carry.
 **/
static void AddInfectedFlashlight(std::string asEnemy)
{
	cGameEnemy_Infected *pInfected = GetScriptInfected(asEnemy);
	if(pInfected==nullptr) return;

	pInfected->AddFlashLight(true);
}

// Black Plague's security cameras

/**
 * How a security camera turns: the angles to each side in degrees, the pause
 * at each end in seconds and the speed. asSpottedCallback(string asCamera) is
 * called when the player is seen, asAlertOverCallback when the alert is over.
 **/
static void SetupSecurityCamera(std::string asName, float afMinAngle, float afMaxAngle,
											float afMinPause, float afMaxPause, float afSpeed,
											std::string asSpottedCallback, std::string asAlertOverCallback)
{
	cGameCamera *pCamera = FindScriptEntity<cGameCamera>(asName, "camera entity", eGameEntityType_Camera);
	if(pCamera==nullptr) return;

	pCamera->Setup(cMath::ToRad(afMinAngle),cMath::ToRad(afMaxAngle),afMinPause,afMaxPause,afSpeed,
					asSpottedCallback,asAlertOverCallback);
}

static void SetSecurityCameraAlerted(std::string asName, bool abAlerted)
{
	cGameCamera *pCamera = FindScriptEntity<cGameCamera>(asName, "camera entity", eGameEntityType_Camera);
	if(pCamera==nullptr) return;

	pCamera->SetAlerted(abAlerted);
}

static void ShowEnemyPlayer(std::string asEnemy)
{
	iGameEnemy *pEnemy = FindScriptEntity<iGameEnemy>(asEnemy, "enemy entity", eGameEntityType_Enemy);
	if(pEnemy==nullptr) return;


	pEnemy->SetLastPlayerPos(	gpInit->mpPlayer->GetCharacterBody()->GetFeetPosition() + 
								cVector3f(0,0.1f,0));
	pEnemy->ChangeState(STATE_HUNT);
}

static void SetDoorLocked(std::string asDoor, bool abLocked)
{
	cGameSwingDoor *pDoor = FindScriptEntity<cGameSwingDoor>(asDoor, "swing door entity", eGameEntityType_SwingDoor);
	if(pDoor==nullptr) return;

	pDoor->SetLocked(abLocked);

}

static void SetupStickArea(	std::string asArea, bool abCanDeatch,
											bool abMoveBody,bool abRotateBody,
											bool abCheckCenterInArea, float afPoseTime,
									 std::string asAttachSound, std::string asDetachSound,
									 std::string asAttachPS, std::string asDetachPS,
									 std::string asAttachFunc, std::string asDetachFunc)
{
	cGameStickArea *pArea = FindScriptEntity<cGameStickArea>(asArea, "stick area", eGameEntityType_StickArea);
	if(pArea==nullptr) return;

    pArea->SetCanDetach(abCanDeatch);

	pArea->SetRotateBody(abRotateBody);
	pArea->SetMoveBody(abMoveBody);

	pArea->SetCheckCenterInArea(abCheckCenterInArea);
	
	pArea->SetPoseTime(afPoseTime);
	
	pArea->SetAttachSound(asAttachSound);
	pArea->SetDetachSound(asDetachSound);

	pArea->SetAttachPS(asAttachPS);
	pArea->SetDetachPS(asDetachPS);

	pArea->SetAttachFunction(asAttachFunc);
	pArea->SetDetachFunction(asDetachFunc);
}

static void AllowAttachment()
{
	cGameStickArea::mbAllowAttachment = true;
}

static void SetLampLit(std::string asName,bool abLit, bool abFade)
{
	cGameLamp *pLamp = FindScriptEntity<cGameLamp>(asName, "lamp", eGameEntityType_Lamp);
	if(pLamp==nullptr) return;

    pLamp->SetLit(abLit,abFade);
}

static void SetLampFlicker(std::string asName,bool abFlicker)
{
	cGameLamp *pLamp = FindScriptEntity<cGameLamp>(asName, "lamp", eGameEntityType_Lamp);
	if(pLamp==nullptr) return;

	pLamp->SetFlicker(abFlicker);
}

static void SetLampLitChangeCallback(std::string asName,std::string asCallback)
{
	cGameLamp *pLamp = FindScriptEntity<cGameLamp>(asName, "lamp", eGameEntityType_Lamp);
	if(pLamp==nullptr) return;

	pLamp->SetLitChangeCallback(asCallback);
}

static void SetupLadder(	std::string asName,
											std::string asAttachSound, 
											std::string asClimbUpSound, 
											std::string asClimbDownSound)
{
	cGameLadder *pLadder = FindScriptEntity<cGameLadder>(asName, "ladder", eGameEntityType_Ladder);
	if(pLadder==nullptr) return;

	pLadder->SetAttachSound(asAttachSound);
	pLadder->SetClimbUpSound(asClimbUpSound);
	pLadder->SetClimbDownSound(asClimbDownSound);
}

static void SetupDamageArea(std::string asName, float afDamage,
													 float afUpdatesPerSec, int alStrength,
													 bool abDisableObjects, bool abDisableEnemies)
{
	cGameDamageArea *pDamage = FindScriptEntity<cGameDamageArea>(asName, "damage area", eGameEntityType_DamageArea);
	if(pDamage==nullptr) return;

	pDamage->SetDamage(afDamage);
	pDamage->SetUpdatesPerSec(afUpdatesPerSec);
	pDamage->SetStrength(alStrength);
	pDamage->SetDisableObjects(abDisableObjects);
	pDamage->SetDisableEnemies(abDisableEnemies);
	
}

/**
 * Black Plague: with how the player is shown to be hurt and what is heard then.
 * \param asDamageType "BloodSplash", "Ice" or "Poison"
 * \param asDamageSound played each time the player is hurt, "" = none
 */
static void SetupDamageAreaType(std::string asName, float afDamage,
													 float afUpdatesPerSec, int alStrength,
													 bool abDisableObjects, bool abDisableEnemies,
													 std::string asDamageType, std::string asDamageSound)
{
	cGameDamageArea *pDamage = FindScriptEntity<cGameDamageArea>(asName, "damage area", eGameEntityType_DamageArea);
	if(pDamage==nullptr) return;

	SetupDamageArea(asName, afDamage, afUpdatesPerSec, alStrength, abDisableObjects, abDisableEnemies);

	tString sType = cString::ToLowerCase(asDamageType);
	ePlayerDamageType type = ePlayerDamageType_BloodSplash;
	if(sType == "bloodsplash")	type = ePlayerDamageType_BloodSplash;
	else if(sType == "ice")		type = ePlayerDamageType_Ice;
	else if(sType == "poison")	type = ePlayerDamageType_Poison;
	else Warning("Damage type %s does not exist!\n",asDamageType.c_str());

	pDamage->SetDamageType(type);
	pDamage->SetDamageSound(asDamageSound);
}

static void SetupForceArea(std::string asName, 
									 float afMaxForce, float afConstant,
									 float afDestSpeed, float afMaxMass,
									 bool abMulWithMass, bool abForceAtPoint,
									 bool abAffectBodies, bool abAffectCharacters)
{
	cGameForceArea *pForce = FindScriptEntity<cGameForceArea>(asName, "force area", eGameEntityType_ForceArea);
	if(pForce==nullptr) return;

	pForce->SetMaxForce(afMaxForce);
	pForce->SetConstant(afConstant);
	pForce->SetDestSpeed(afDestSpeed);
	pForce->SetMaxMass(afMaxMass);
	pForce->SetMulWithMass(abMulWithMass);
	pForce->SetForceAtPoint(abForceAtPoint);
	pForce->SetAffectBodies(abAffectBodies);
	pForce->SetAffectCharacters(abAffectCharacters);
}

static void SetupLiquidArea(std::string asName,
								  float afDensity, float afLinearViscosity,
								  float afAngularViscosity,
								  std::string asPhysicsMaterial,
								  float fR, float fG, float fB,
								  bool abHasWaves)
{
	cGameLiquidArea *pLiquid = FindScriptEntity<cGameLiquidArea>(asName, "liquid area", eGameEntityType_LiquidArea);
	if(pLiquid==nullptr) return;

	pLiquid->SetDensity(afDensity);
	pLiquid->SetLinearViscosity(afLinearViscosity);
	pLiquid->SetAngularViscosity(afAngularViscosity);

	pLiquid->SetPhysicsMaterial(asPhysicsMaterial);

	pLiquid->SetColor(cColor(fR,fG,fB,1));

	pLiquid->SetHasWaves(abHasWaves);
}

static void SetupSaveArea(std::string asName,
											std::string asMessageCat,std::string asMessageEntry, 
											std::string asSound)
{
	cGameSaveArea *pSave = FindScriptEntity<cGameSaveArea>(asName, "save area", eGameEntityType_SaveArea);
	if(pSave==nullptr) return;

	pSave->SetMessageCat(asMessageCat);
	pSave->SetMessageEntry(asMessageEntry);
	pSave->SetSound(asSound);
}

// GAME ENTITY CALLBACKS

static eGameCollideScriptType GetGameCollideScriptType(const tString &asType)
{
	tString sName = cString::ToLowerCase(asType);

    if(sName == "enter") return eGameCollideScriptType_Enter;
	if(sName == "leave") return eGameCollideScriptType_Leave;
	if(sName == "during") return eGameCollideScriptType_During;

	Warning("Collide Type %s doesn't exist!\n",asType.c_str());

	return eGameCollideScriptType_LastEnum;
}

static void AddEntityCollideCallback(std::string asType,
											   std::string asDestName,
											   std::string asEntityName,
											   std::string asFuncName)
{
	if(cString::ToLowerCase(asDestName)=="player")
	{
		eGameCollideScriptType type = GetGameCollideScriptType(asType);
		
		if(type != eGameCollideScriptType_LastEnum)
			gpInit->mpPlayer->AddCollideScript(type,asFuncName,asEntityName);
	}
	else
	{
		iGameEntity *pEntity = FindScriptEntity(asDestName, "entity");
		if(pEntity==nullptr) return;

		eGameCollideScriptType type = GetGameCollideScriptType(asType);

		if(type != eGameCollideScriptType_LastEnum)
			pEntity->AddCollideScript(type,asFuncName,asEntityName);
	}
	
}

static void RemoveEntityCollideCallback(std::string asType,
											   std::string asDestName,
											   std::string asEntityName)
{
	if(cString::ToLowerCase(asDestName)=="player")
	{
		eGameCollideScriptType type = GetGameCollideScriptType(asType);
		
		if(type != eGameCollideScriptType_LastEnum)
			gpInit->mpPlayer->RemoveCollideScript(type,asEntityName);
	}
	else
	{
		iGameEntity *pEntity = FindScriptEntity(asDestName, "entity");
		if(pEntity==nullptr) return;

		eGameCollideScriptType type = GetGameCollideScriptType(asType);

		if(type != eGameCollideScriptType_LastEnum)
		{
			pEntity->RemoveCollideScript(type,asEntityName);
		}
	}
}

static eGameEntityScriptType GetGameScriptType(const tString &asType)
{
	tString sName = cString::ToLowerCase(asType);

	if(sName == "playerinteract") return eGameEntityScriptType_PlayerInteract;
	if(sName == "playerexamine") return eGameEntityScriptType_PlayerExamine;
	if(sName == "playerlook") return eGameEntityScriptType_PlayerPick;
	if(sName == "onupdate") return eGameEntityScriptType_OnUpdate;
	if(sName == "onbreak") return eGameEntityScriptType_OnBreak;

	Warning("Script type '%s' doesn't exist!\n",asType.c_str());

	return eGameEntityScriptType_LastEnum;
}

static void AddEntityCallback(std::string asType,
										std::string asDestName,
										std::string asFuncName)
{
	iGameEntity *pEntity = FindScriptEntity(asDestName, "entity");
	if(pEntity==nullptr) return;

	eGameEntityScriptType type = GetGameScriptType(asType);

	if(type!= eGameEntityScriptType_LastEnum)
	{
		pEntity->AddScript(type, asFuncName);
	}
}

static void RemoveEntityCallback(std::string asType,
										   std::string asDestName)
{
	iGameEntity *pEntity = FindScriptEntity(asDestName, "entity");
	if(pEntity==nullptr) return;

	eGameEntityScriptType type = GetGameScriptType(asType);

    if(type!= eGameEntityScriptType_LastEnum)
	{
		pEntity->RemoveScript(type);
	}
}

// GAME SOUND

static void CreateSoundEntityAt(std::string asType,std::string asDestName, 
													 std::string asSoundName,  std::string asSoundFile)
{
	cWorld3D *pWorld = gpInit->mpGame->GetScene()->GetWorld3D();
	iPhysicsWorld *pPhysicsWorld = gpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();

	int lType=0;
	tString sTypeLow = cString::ToLowerCase(asType);

	if(sTypeLow=="body")lType = 1;
	else if(sTypeLow=="joint")lType = 2;
	else if(sTypeLow=="entity")lType = 3;

	if(lType == 0)
	{
		Warning("Cannot find type '%s' for sound entity position.\n",asType.c_str());
		return;
	}

	cVector3f vPos;
	
	// Body
	if(lType == 1)
	{
		iPhysicsBody *pBody = pPhysicsWorld->GetBody(asDestName);
		if(pBody==nullptr)	{
			Warning("Body '%s' coudln't be found!\n",asDestName.c_str()); return;
		}

		vPos = pBody->GetLocalPosition();
	}
	// Joint
	else if(lType == 2)
	{
		iPhysicsJoint *pJoint = pPhysicsWorld->GetJoint(asDestName);
		if(pJoint==nullptr)	{
			Warning("Body '%s' coudln't be found!\n",asDestName.c_str()); return;
		}

		vPos = pJoint->GetPivotPoint();
	}
	// Entity
	{
		iGameEntity *pEntity = FindScriptEntity(asDestName, "entity");
		if(pEntity==nullptr) return;
		
		if(pEntity->GetMeshEntity())
			vPos = pEntity->GetMeshEntity()->GetWorldPosition();
		else
			vPos = pEntity->GetBody(0)->GetLocalPosition();
	}
	
	// Create sound.
	cSoundEntity *pSound = pWorld->CreateSoundEntity(asSoundName,asSoundFile,true);
	if(pSound)
	{
		pSound->SetPosition(vPos);
	}
}

// PUBLIC FUNCTIONS

// LEVERS AND WHEELS (BLACK PLAGUE)

/**
 * The function is called as Func(string asName, int alAction, int alState):
 * the action 0 = enter, 1 = leave, 2 = during; the state -1 = min, 0 = middle, 1 = max.
 * \param asType "Enter", "Leave" or "During"
 */
static void SetLeverCallback(std::string asName, std::string asType, std::string asFunc)
{
	cGameLever *pLever = FindScriptEntity<cGameLever>(asName, "lever", eGameEntityType_Lever);
	if(pLever==nullptr) return;

	tString sType = cString::ToLowerCase(asType);

	if(sType == "enter")		pLever->SetCallback(eGameLeverAction_Enter, asFunc);
	else if(sType == "leave")	pLever->SetCallback(eGameLeverAction_Leave, asFunc);
	else if(sType == "during")	pLever->SetCallback(eGameLeverAction_During, asFunc);
}

/**
 * \param alState -1 = stuck at min, 1 = stuck at max, 0 = not stuck
 */
static void SetLeverStuck(std::string asName, int alState)
{
	cGameLever *pLever = FindScriptEntity<cGameLever>(asName, "lever", eGameEntityType_Lever);
	if(pLever==nullptr) return;

	if(alState == 0)		pLever->SetIsStuck(false);
	else if(alState == 1)	pLever->SetStuck(eGameLeverState_Max);
	else if(alState == -1)	pLever->SetStuck(eGameLeverState_Min);
}

/**
 * \return -1 = min, 0 = middle, 1 = max
 */
static int GetLeverState(std::string asName)
{
	cGameLever *pLever = FindScriptEntity<cGameLever>(asName, "lever", eGameEntityType_Lever);
	if(pLever==nullptr) return 0;

	switch(pLever->GetState())
	{
	case eGameLeverState_Min: return -1;
	case eGameLeverState_Max: return 1;
	default: return 0;
	}
}

static void SetWheelCallback(std::string asName, std::string asType, std::string asFunc)
{
	cGameWheel *pWheel = FindScriptEntity<cGameWheel>(asName, "wheel", eGameEntityType_Wheel);
	if(pWheel==nullptr) return;

	tString sType = cString::ToLowerCase(asType);

	if(sType == "enter")		pWheel->SetCallback(eGameWheelAction_Enter, asFunc);
	else if(sType == "leave")	pWheel->SetCallback(eGameWheelAction_Leave, asFunc);
	else if(sType == "during")	pWheel->SetCallback(eGameWheelAction_During, asFunc);
}

static void SetWheelStuck(std::string asName, int alState)
{
	cGameWheel *pWheel = FindScriptEntity<cGameWheel>(asName, "wheel", eGameEntityType_Wheel);
	if(pWheel==nullptr) return;

	if(alState == 0)		pWheel->SetIsStuck(false);
	else if(alState == 1)	pWheel->SetStuck(eGameWheelState_Max);
	else if(alState == -1)	pWheel->SetStuck(eGameWheelState_Min);
}

static int GetWheelState(std::string asName)
{
	cGameWheel *pWheel = FindScriptEntity<cGameWheel>(asName, "wheel", eGameEntityType_Wheel);
	if(pWheel==nullptr) return 0;

	switch(pWheel->GetState())
	{
	case eGameWheelState_Min: return -1;
	case eGameWheelState_Max: return 1;
	default: return 0;
	}
}

/**
 * The angle the wheel has been turned in all, in degrees
 */
static float GetWheelAngle(std::string asName)
{
	cGameWheel *pWheel = FindScriptEntity<cGameWheel>(asName, "wheel", eGameEntityType_Wheel);
	if(pWheel==nullptr) return 0;

	return cMath::ToDeg(pWheel->GetAngle());
}

static float GetWheelSpeed(std::string asName)
{
	cGameWheel *pWheel = FindScriptEntity<cGameWheel>(asName, "wheel", eGameEntityType_Wheel);
	if(pWheel==nullptr) return 0;

	return pWheel->GetSpeed();
}

/**
 * The limits of a joint follow the wheel: afMin at the wheel's least angle,
 * afMax at its greatest, afRange between the two limits.
 */
static void ConnectWheelToJoint(std::string asName, std::string asJoint,
											float afMin, float afMax, float afRange)
{
	cGameWheel *pWheel = FindScriptEntity<cGameWheel>(asName, "wheel", eGameEntityType_Wheel);
	if(pWheel==nullptr) return;

	iPhysicsWorld *pPhysicsWorld = gpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();
	iPhysicsJoint *pJoint = pPhysicsWorld->GetJoint(asJoint);
	if(pJoint==nullptr)
	{
		Warning("Couldn't find joint '%s'\n",asJoint.c_str());
		return;
	}

	pWheel->ConnectToJointLimit(pJoint, afMin, afMax, afRange);
}

/**
 * The same for the value a controller on one of the entity's joints aims at
 */
static void ConnectWheelToController(std::string asName, std::string asEntity, std::string asController,
											float afMin, float afMax)
{
	cGameWheel *pWheel = FindScriptEntity<cGameWheel>(asName, "wheel", eGameEntityType_Wheel);
	if(pWheel==nullptr) return;

	iGameEntity *pEntity = FindScriptEntity(asEntity, "controller entity");
	if(pEntity==nullptr) return;

	pWheel->ConnectToController(pEntity, asController, afMin, afMax);
}

// MOVING THINGS TO AREAS (BLACK PLAGUE)

static iPhysicsBody* GetBody(const std::string &asName)
{
	iPhysicsWorld *pPhysicsWorld = gpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();
	iPhysicsBody *pBody = pPhysicsWorld->GetBody(asName);
	if(pBody==nullptr) Warning("Couldn't find body '%s'\n",asName.c_str());
	return pBody;
}

static void TeleportBody(iPhysicsBody *apBody, cGameArea *apArea, bool abResetVelocity, bool abSetRotation,
						const cVector3f &avRotation)
{
	apBody->SetPosition(apArea->GetBody(0)->GetWorldPosition());

	if(abResetVelocity)
	{
		apBody->SetLinearVelocity(0);
		apBody->SetAngularVelocity(0);
	}

	if(abSetRotation)
	{
		cMatrixf mtxTransform = cMath::MatrixRotate(avRotation, eEulerRotationOrder_XYZ);
		mtxTransform.SetTranslation(apBody->GetWorldPosition());
		apBody->SetMatrix(mtxTransform);
	}

	apBody->SetEnabled(true);
}

/**
 * Moves an entity in a straight line to the centre of an area.
 * \param afMaxSpeed in m/s
 * \param afAcceleration in m/s^2
 * \param afSlowDownDist the distance from the area inside which it slows down
 * \param asCallback called on arrival: Func(string asEntity). "" = none.
 */
static void MoveEntityToArea(std::string asName, std::string asArea, float afMaxSpeed,
										float afAcceleration, float afSlowDownDist, std::string asCallback)
{
	iGameEntity *pEntity = FindScriptEntity(asName, "game entity");
	if(pEntity==nullptr) return;

	cGameArea *pArea = FindScriptEntity<cGameArea>(asArea, "area entity", eGameEntityType_Area);
	if(pArea==nullptr) return;

	pEntity->MoveToPosition(pArea->GetBody(0)->GetWorldPosition(), afMaxSpeed, afAcceleration,
							afSlowDownDist, asCallback);
}

/**
 * Puts a body at the centre of an area.
 * \param abResetVelocity the body is left without speed
 * \param abSetRotation the body gets the rotation given (radians), else it keeps its own
 */
static void TeleportBodyToArea(std::string asBody, std::string asArea, bool abResetVelocity,
										bool abSetRotation, float afRotX, float afRotY, float afRotZ)
{
	iPhysicsBody *pBody = GetBody(asBody);
	if(pBody==nullptr) return;

	cGameArea *pArea = FindScriptEntity<cGameArea>(asArea, "area entity", eGameEntityType_Area);
	if(pArea==nullptr) return;

	TeleportBody(pBody, pArea, abResetVelocity, abSetRotation, cVector3f(afRotX, afRotY, afRotZ));
}

/**
 * The same for the first body of an entity
 */
static void TeleportEntityToArea(std::string asName, std::string asArea, bool abResetVelocity,
										bool abSetRotation, float afRotX, float afRotY, float afRotZ)
{
	iGameEntity *pEntity = FindScriptEntity(asName, "game entity");
	if(pEntity==nullptr) return;

	cGameArea *pArea = FindScriptEntity<cGameArea>(asArea, "area entity", eGameEntityType_Area);
	if(pArea==nullptr) return;

	if(pEntity->GetBodyNum()==0)
	{
		Warning("Entity %s has no bodies! Could not be teleported!\n",asName.c_str());
		return;
	}

	TeleportBody(pEntity->GetBody(0), pArea, abResetVelocity, abSetRotation, cVector3f(afRotX, afRotY, afRotZ));
}

/**
 * The player stands at the centre of an area
 */
static void TeleportPlayerToArea(std::string asArea)
{
	cGameArea *pArea = FindScriptEntity<cGameArea>(asArea, "area entity", eGameEntityType_Area);
	if(pArea==nullptr) return;

	gpInit->mpPlayer->GetCharacterBody()->SetFeetPosition(pArea->GetBody(0)->GetWorldPosition(), false);
}

/**
 * Requiem: an entity is made from a file, placed and turned as the area is
 */
static void CreateEntityAtArea(std::string asName, std::string asFile, std::string asArea)
{
	cWorld3D *pWorld = gpInit->mpGame->GetScene()->GetWorld3D();

	cGameArea *pArea = FindScriptEntity<cGameArea>(asArea, "area entity", eGameEntityType_Area);
	if(pArea==nullptr) return;

	pWorld->CreateEntity(asName, pArea->GetBody(0)->GetWorldMatrix(), asFile, true);
}

static bool BodiesCollide(iPhysicsBody *apBodyA, iPhysicsBody *apBodyB)
{
	if(cMath::CheckCollisionBV(*apBodyA->GetBV(), *apBodyB->GetBV())==false) return false;

	iPhysicsWorld *pPhysicsWorld = gpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();

	cCollideData collideData;
	collideData.SetMaxSize(1);

	return pPhysicsWorld->CheckShapeCollision(apBodyA->GetShape(), apBodyA->GetLocalMatrix(),
											apBodyB->GetShape(), apBodyB->GetLocalMatrix(),
											collideData, 1);
}

/**
 * \return true if two bodies touch
 */
static bool CheckBodyCollision(std::string asBodyA, std::string asBodyB)
{
	iPhysicsWorld *pPhysicsWorld = gpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();

	iPhysicsBody *pBodyA = pPhysicsWorld->GetBody(asBodyA);
	iPhysicsBody *pBodyB = pPhysicsWorld->GetBody(asBodyB);
	if(pBodyA==nullptr || pBodyB==nullptr) return false;

	return BodiesCollide(pBodyA, pBodyB);
}

/**
 * \return true if any body of an entity touches a body
 */
static bool CheckEntityBodyCollision(std::string asName, std::string asBody)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr) return false;

	iPhysicsWorld *pPhysicsWorld = gpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();
	iPhysicsBody *pBody = pPhysicsWorld->GetBody(asBody);
	if(pBody==nullptr) return false;

	for(int i=0; i<pEntity->GetBodyNum(); ++i)
	{
		if(BodiesCollide(pEntity->GetBody(i), pBody)) return true;
	}
	return false;
}

/**
 * \return true if any body of one entity touches a body of the other.
 * (The game's own binary pairs the bodies by their number only, which is the
 * same thing for the entities of one body that its scripts ask about.)
 */
static bool CheckGameEntityCollision(std::string asNameA, std::string asNameB)
{
	iGameEntity *pEntityA = gpInit->mpMapHandler->GetGameEntity(asNameA);
	iGameEntity *pEntityB = gpInit->mpMapHandler->GetGameEntity(asNameB);
	if(pEntityA==nullptr || pEntityB==nullptr) return false;

	for(int i=0; i<pEntityA->GetBodyNum(); ++i)
	for(int j=0; j<pEntityB->GetBodyNum(); ++j)
	{
		if(BodiesCollide(pEntityA->GetBody(i), pEntityB->GetBody(j))) return true;
	}
	return false;
}

/**
 * A stick area takes hold of a body, wherever the body is
 */
static void AttachBodyToStickArea(std::string asBody, std::string asArea)
{
	cGameStickArea *pStickArea = FindScriptEntity<cGameStickArea>(asArea, "stick area", eGameEntityType_StickArea);
	if(pStickArea==nullptr) return;

	iPhysicsBody *pBody = GetBody(asBody);
	if(pBody==nullptr) return;

	pStickArea->AttachBody(pBody);
}

static void DetachBodyFromStickArea(std::string asArea)
{
	cGameStickArea *pStickArea = FindScriptEntity<cGameStickArea>(asArea, "stick area", eGameEntityType_StickArea);
	if(pStickArea==nullptr) return;

	pStickArea->DetachBody();
}

// DOORS, PLAYER AND NOTEBOOK (BLACK PLAGUE)

/**
 * What is said and heard when the player tries a locked swing door.
 * \param abX not used by the game
 */
static void SetDoorLockedProperties(std::string asDoor, std::string asDescCat, std::string asDescEntry,
												std::string asSound, bool abX)
{
	cGameSwingDoor *pDoor = FindScriptEntity<cGameSwingDoor>(asDoor, "swing door entity", eGameEntityType_SwingDoor);
	if(pDoor==nullptr) return;

	pDoor->SetLockedDesc(gpInit->mpGame->GetResources()->Translate(asDescCat, asDescEntry));
	pDoor->SetLockedSound(asSound);
}

static void SetNoFallDamage(bool abNoFallDamage)
{
	gpInit->mpPlayer->SetNoFallDamage(abNoFallDamage);
}

static void TurnOffGlowStick()
{
	if(gpInit->mpPlayer->GetGlowStick()->IsActive())
		gpInit->mpPlayer->GetGlowStick()->SetActive(false);
}

/**
 * Requiem: a lit flare is put out
 */
static void DropFlare()
{
	if(gpInit->mpPlayer->GetFlare()->IsActive())
		gpInit->mpPlayer->GetFlare()->SetActive(false);
}

/**
 * The function is called, once, when the flashlight's power falls below the value: Func()
 */
static void SetLowPowerCallback(std::string asFunc, float afVal)
{
	gpInit->mpPlayer->SetLowPowerCallback(asFunc, afVal);
}

/**
 * A radio message heard from an entity (an area as a rule): at full volume inside
 * afMinDist, fading to nothing at afMaxDist, where its text is not shown either.
 */
static void AddRadioMessageAtPos(std::string asTransCat, std::string asTransName, std::string asSound,
											std::string asEntity, float afMinDist, float afMaxDist)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asEntity);
	if(pEntity==nullptr || pEntity->GetBodyNum()==0) return;

	gpInit->mpRadioHandler->Add(gpInit->mpGame->GetResources()->Translate(asTransCat, asTransName),
								asSound, pEntity, afMinDist, afMaxDist);
}

/**
 * The waiting radio messages are dropped and the one playing fades out.
 * \param afFadeSpeed volume lost per second
 */
static void StopAllRadioMessages(float afFadeSpeed)
{
	gpInit->mpRadioHandler->StopAll(afFadeSpeed);
}

// COMPUTER TERMINALS (Black Plague)
//
// A screen from interfaces/*.gui, see ui/GameGui. The widget functions act
// on the screen that is open and are called from its own script.

static cGameGui* GetScreenGui(const char *apFunc)
{
	cGameGui *pGui = gpInit->mpGameGuiHandler->GetScreenGui();
	if(pGui == nullptr) Warning("%s: no computer screen is open\n", apFunc);
	return pGui;
}

static iWidget* GetScreenWidget(const char *apFunc, const tString &asName)
{
	cGameGui *pGui = GetScreenGui(apFunc);
	if(pGui == nullptr) return nullptr;

	return pGui->GetSet()->GetWidgetFromName(asName);
}

static void OpenScreenGui(std::string asFile)
{
	gpInit->mpGameGuiHandler->OpenScreenGui(asFile);
}

/**
 * The screen is taken down at the next update, so its own script may call this.
 */
static void CloseScreenGui()
{
	gpInit->mpGameGuiHandler->CloseScreenGui(true);
}

/**
 * Properties: Enabled, Visible
 */
static void SetWidgetBool(std::string asName, std::string asProp, bool abVal)
{
	iWidget *pWidget = GetScreenWidget("SetWidgetBool", asName);
	if(pWidget == nullptr) return;

	tString sProp = cString::ToLowerCase(asProp);

	if(sProp == "enabled")		pWidget->SetEnabled(abVal);
	else if(sProp == "visible")	pWidget->SetVisible(abVal);
	else Warning("Bool property '%s' does not exist", asProp.c_str());
}

static bool GetWidgetBool(std::string asName, std::string asProp)
{
	iWidget *pWidget = GetScreenWidget("GetWidgetBool", asName);
	if(pWidget == nullptr) return false;

	tString sProp = cString::ToLowerCase(asProp);

	if(sProp == "enabled") return pWidget->IsEnabled();
	if(sProp == "visible") return pWidget->IsVisible();

	Warning("Bool property '%s' does not exist", asProp.c_str());
	return false;
}

/**
 * Properties: SelectedItem (list and combo box), MaxTextLength (text box, label),
 * MaxShownItems (combo box), MaxValue, Value, BarValueSize, ButtonValueAdd (slider)
 */
static void SetWidgetInt(std::string asName, std::string asProp, int alVal)
{
	iWidget *pWidget = GetScreenWidget("SetWidgetInt", asName);
	if(pWidget == nullptr) return;

	tString sProp = cString::ToLowerCase(asProp);
	eWidgetType type = pWidget->GetType();

	if(sProp == "selecteditem")
	{
		if(type == eWidgetType_ComboBox)		static_cast<cWidgetComboBox*>(pWidget)->SetSelectedItem(alVal, false);
		else if(type == eWidgetType_ListBox)	static_cast<cWidgetListBox*>(pWidget)->SetSelectedItem(alVal, false);
		return;
	}
	if(sProp == "maxtextlength")
	{
		if(type == eWidgetType_TextBox)		static_cast<cWidgetTextBox*>(pWidget)->SetMaxTextLength(alVal);
		else if(type == eWidgetType_Label)	static_cast<cWidgetLabel*>(pWidget)->SetMaxTextLength(alVal);
		return;
	}
	if(type == eWidgetType_ComboBox && sProp == "maxshownitems")
	{
		static_cast<cWidgetComboBox*>(pWidget)->SetMaxShownItems(alVal);
		return;
	}
	if(type == eWidgetType_Slider)
	{
		cWidgetSlider *pSlider = static_cast<cWidgetSlider*>(pWidget);

		if(sProp == "maxvalue")			{ pSlider->SetMaxValue(alVal); return; }
		if(sProp == "value")			{ pSlider->SetValue(alVal); return; }
		if(sProp == "barvaluesize")		{ pSlider->SetBarValueSize(alVal); return; }
		if(sProp == "buttonvalueadd")	{ pSlider->SetButtonValueAdd(alVal); return; }
	}

	Warning("Int property '%s' does not exist", asProp.c_str());
}

static int GetWidgetInt(std::string asName, std::string asProp)
{
	iWidget *pWidget = GetScreenWidget("GetWidgetInt", asName);
	if(pWidget == nullptr) return 0;

	tString sProp = cString::ToLowerCase(asProp);
	eWidgetType type = pWidget->GetType();

	if(sProp == "selecteditem")
	{
		if(type == eWidgetType_ComboBox)	return static_cast<cWidgetComboBox*>(pWidget)->GetSelectedItem();
		if(type == eWidgetType_ListBox)		return static_cast<cWidgetListBox*>(pWidget)->GetSelectedItem();
		return 0;
	}
	if(type == eWidgetType_ComboBox && sProp == "maxshownitems")
	{
		return static_cast<cWidgetComboBox*>(pWidget)->GetMaxShownItems();
	}
	if(type == eWidgetType_Slider)
	{
		cWidgetSlider *pSlider = static_cast<cWidgetSlider*>(pWidget);

		if(sProp == "maxvalue")			return pSlider->GetMaxValue();
		if(sProp == "value")			return pSlider->GetValue();
		if(sProp == "barvaluesize")		return pSlider->GetBarValueSize();
		if(sProp == "buttonvalueadd")	return pSlider->GetButtonValueAdd();
	}

	Warning("Int property '%s' does not exist", asProp.c_str());
	return 0;
}

/**
 * Properties: BackgroundZ (frame, list box)
 */
static void SetWidgetFloat(std::string asName, std::string asProp, float afVal)
{
	iWidget *pWidget = GetScreenWidget("SetWidgetFloat", asName);
	if(pWidget == nullptr) return;

	tString sProp = cString::ToLowerCase(asProp);

	if(sProp == "backgroundz")
	{
		if(pWidget->GetType() == eWidgetType_ListBox)		static_cast<cWidgetListBox*>(pWidget)->SetBackgroundZ(afVal);
		else if(pWidget->GetType() == eWidgetType_Frame)	static_cast<cWidgetFrame*>(pWidget)->SetBackgroundZ(afVal);
		return;
	}

	Warning("Float property '%s' does not exist", asProp.c_str());
}

/**
 * Properties: BackgroundColor (frame)
 */
static void SetWidgetColor(std::string asName, std::string asProp, float afR, float afG, float afB, float afA)
{
	iWidget *pWidget = GetScreenWidget("SetWidgetColor", asName);
	if(pWidget == nullptr) return;

	tString sProp = cString::ToLowerCase(asProp);

	if(sProp == "backgroundcolor")
	{
		if(pWidget->GetType() == eWidgetType_Frame)
			static_cast<cWidgetFrame*>(pWidget)->SetBackGroundColor(cColor(afR, afG, afB, afA));
		return;
	}

	Warning("Color property '%s' does not exist", asProp.c_str());
}

/**
 * Properties: Size
 */
static void SetWidgetVec2(std::string asName, std::string asProp, float afX, float afY)
{
	iWidget *pWidget = GetScreenWidget("SetWidgetVec2", asName);
	if(pWidget == nullptr) return;

	tString sProp = cString::ToLowerCase(asProp);

	if(sProp == "size")	pWidget->SetSize(cVector2f(afX, afY));
	else Warning("Vec2 property '%s' does not exist", asProp.c_str());
}

/**
 * Properties: Position
 */
static void SetWidgetVec3(std::string asName, std::string asProp, float afX, float afY, float afZ)
{
	iWidget *pWidget = GetScreenWidget("SetWidgetVec3", asName);
	if(pWidget == nullptr) return;

	tString sProp = cString::ToLowerCase(asProp);

	if(sProp == "position")	pWidget->SetPosition(cVector3f(afX, afY, afZ));
	else Warning("Vec3 property '%s' does not exist", asProp.c_str());
}

/**
 * Properties: Text, Font (a font of the screen's file), Image (image and button; an image of the screen's file)
 */
static void SetWidgetString(std::string asName, std::string asProp, std::string asVal)
{
	cGameGui *pGui = GetScreenGui("SetWidgetString");
	if(pGui == nullptr) return;
	iWidget *pWidget = pGui->GetSet()->GetWidgetFromName(asName);
	if(pWidget == nullptr) return;

	tString sProp = cString::ToLowerCase(asProp);

	if(sProp == "text")
	{
		pWidget->SetText(cString::To16Char(asVal));
	}
	else if(sProp == "font")
	{
		if(pGui->SetWidgetFont(pWidget, asVal)==false)
			Error("Could not find gui font %s!\n", asVal.c_str());
	}
	else if(sProp == "image")
	{
		// The screen keeps its images: the button must not destroy the one it gets
		cGuiGfxElement *pImage = pGui->GetImage(asVal);

		if(pWidget->GetType() == eWidgetType_Image)			static_cast<cWidgetImage*>(pWidget)->SetImage(pImage);
		else if(pWidget->GetType() == eWidgetType_Button)	static_cast<cWidgetButton*>(pWidget)->SetImage(pImage, false);
	}
	else
	{
		Warning("String property '%s' does not exist", asProp.c_str());
	}
}

/**
 * Properties: Text
 */
static void SetWidgetStringTrans(std::string asName, std::string asProp, std::string asTransCat, std::string asTransName)
{
	iWidget *pWidget = GetScreenWidget("SetWidgetStringTrans", asName);
	if(pWidget == nullptr) return;

	tString sProp = cString::ToLowerCase(asProp);

	if(sProp == "text")
		pWidget->SetText(gpInit->mpGame->GetResources()->Translate(asTransCat, asTransName));
	else
		Warning("StringTrans property '%s' does not exist", asProp.c_str());
}

/**
 * Properties: Text
 */
static std::string GetWidgetString(std::string asName, std::string asProp)
{
	iWidget *pWidget = GetScreenWidget("GetWidgetString", asName);
	if(pWidget == nullptr) return "";

	tString sProp = cString::ToLowerCase(asProp);

	if(sProp == "text") return cString::To8Char(pWidget->GetText());

	Warning("String property '%s' does not exist", asProp.c_str());
	return "";
}

static iWidgetItemContainer* GetScreenItemContainer(const char *apFunc, const tString &asName)
{
	iWidget *pWidget = GetScreenWidget(apFunc, asName);
	if(pWidget == nullptr) return nullptr;

	if(pWidget->GetType() == eWidgetType_ComboBox)	return static_cast<cWidgetComboBox*>(pWidget);
	if(pWidget->GetType() == eWidgetType_ListBox)	return static_cast<cWidgetListBox*>(pWidget);

	Warning("Item '%s' does not have any items!\n", asName.c_str());
	return nullptr;
}

/**
 * A line more in a list box or combo box
 */
static void AddWidgetItem(std::string asName, std::string asItem)
{
	iWidgetItemContainer *pContainer = GetScreenItemContainer("AddWidgetItem", asName);
	if(pContainer) pContainer->AddItem(cString::To16Char(asItem));
}

static void AddWidgetItemTrans(std::string asName, std::string asTransCat, std::string asTransName)
{
	iWidgetItemContainer *pContainer = GetScreenItemContainer("AddWidgetItemTrans", asName);
	if(pContainer) pContainer->AddItem(gpInit->mpGame->GetResources()->Translate(asTransCat, asTransName));
}

static void RemoveWidgetItem(std::string asName, int alIdx)
{
	iWidgetItemContainer *pContainer = GetScreenItemContainer("RemoveWidgetItem", asName);
	if(pContainer == nullptr) return;

	// The engine's container does not check the number
	if(alIdx < 0 || alIdx >= pContainer->GetItemNum()) return;

	pContainer->RemoveItem(alIdx);
}

/**
 * Only this widget and those in it take the mouse; "" gives it back to all.
 */
static void SetAttentionWidget(std::string asName)
{
	cGameGui *pGui = GetScreenGui("SetAttentionWidget");
	if(pGui == nullptr) return;

	if(asName == "")
		pGui->GetSet()->SetAttentionWidget(nullptr);
	else
		pGui->GetSet()->SetAttentionWidget(pGui->GetSet()->GetWidgetFromName(asName));
}

/**
 * The function is called when the widget gets the message: Func(string asName, string asMessage, int alVal)
 * Messages: MouseDown, MouseUp, MouseDoubleClick, MouseEnter, MouseLeave, ButtonPressed,
 * TextChange, CheckChange, KeyPress, SliderMove, SelectionChange
 */
static void SetWidgetCallback(std::string asName, std::string asMessage, std::string asFunc)
{
	cGameGui *pGui = GetScreenGui("SetWidgetCallback");
	if(pGui == nullptr) return;

	pGui->AddCallback(asName, cGameGui::StringToGuiMessage(asMessage), asFunc);
}

/**
 * A message box with one or two buttons ("" for none). The function is called with the button pressed: Func(int alButton)
 */
static void CreateGuiPopUp(std::string asLabel, std::string asText, std::string asButton1,
									std::string asButton2, std::string asCallback)
{
	cGameGui *pGui = GetScreenGui("CreateGuiPopUp");
	if(pGui == nullptr) return;

	pGui->CreatePopUp(cString::To16Char(asLabel), cString::To16Char(asText),
					cString::To16Char(asButton1), cString::To16Char(asButton2), asCallback);
}

static void CreateGuiPopUpTrans(std::string asLabelCat, std::string asLabelName,
										std::string asTextCat, std::string asTextName,
										std::string asButton1Cat, std::string asButton1Name,
										std::string asButton2Cat, std::string asButton2Name,
										std::string asCallback)
{
	cGameGui *pGui = GetScreenGui("CreateGuiPopUpTrans");
	if(pGui == nullptr) return;

	cResources *pResources = gpInit->mpGame->GetResources();

	pGui->CreatePopUp(pResources->Translate(asLabelCat, asLabelName),
					pResources->Translate(asTextCat, asTextName),
					pResources->Translate(asButton1Cat, asButton1Name),
					pResources->Translate(asButton2Cat, asButton2Name), asCallback);
}

/**
 * A text to read on the screen. It is put in the notebook, the first time it is opened.
 * The function is called when the text is closed: Func()
 */
static void CreateGuiTextView(std::string asLabelCat, std::string asLabelName,
										std::string asTextCat, std::string asTextName,
										std::string asCallback)
{
	cGameGui *pGui = GetScreenGui("CreateGuiTextView");
	if(pGui == nullptr) return;

	tWString sLabel = gpInit->mpGame->GetResources()->Translate(asLabelCat, asLabelName);

	bool bNewNote = false;
	if(gpInit->mpNotebook->NoteExists(asTextCat, asTextName)==false)
	{
		gpInit->mpNotebook->AddNote(sLabel, asTextCat, asTextName);
		bNewNote = true;
	}

	pGui->CreateTextViewer(sLabel, asTextCat, asTextName, asCallback, bNewNote);
}

/**
 * As SetRadioOnEndCallback, for a function in the script of the open computer screen.
 */
static void SetRadioOnEndCallbackGui(std::string asFunc)
{
	gpInit->mpRadioHandler->SetOnEndCallback(asFunc, true);
}

// EFFECTS OF BLACK PLAGUE

/**
 * The view is laid over itself, a little larger, swelling and shrinking
 * afAlpha: how strong; afSpeed: how fast it comes, per second (0 = at once)
 */
static void StartDreamFilter(float afAlpha, float afSpeed)
{
	gpInit->mpEffectHandler->GetDreamFilter()->Start(afAlpha, afSpeed);
}

static void StopDreamFilter(float afSpeed)
{
	gpInit->mpEffectHandler->GetDreamFilter()->Stop(afSpeed);
}

/**
 * A picture fades in over the view, stays and fades out
 * abStartFlash, abEndFlash: with a white flash as it comes and before it goes
 */
static void StartImageFlash(float afFadeIn, float afStay, float afFadeOut, std::string asFile,
									bool abStartFlash, bool abEndFlash)
{
	gpInit->mpEffectHandler->GetImageFlash()->Start(afFadeIn, afStay, afFadeOut, asFile, abStartFlash, abEndFlash);
}

/**
 * Messages in black, for a bright view
 */
static void SetMessageBlackText(bool abBlackText)
{
	gpInit->mpGameMessageHandler->SetBlackText(abBlackText);
}

/**
 * Subtitles on black strips, for a bright view
 */
static void SetSubTitleBlackText(bool abBlackText)
{
	gpInit->mpEffectHandler->GetSubTitle()->SetBlackText(abBlackText);
}

/**
 * True while the gas mask is on the player's face
 */
static bool GetPlayerHasGasMask()
{
	return gpInit->mpEffectHandler->GetGasMask()->IsActive();
}

/**
 * Requiem: the effect of a save at the area (the look, the flash), without saving
 */
static void SaveEffectOnArea(std::string asArea)
{
	cAreaEntity *pArea = gpInit->mpGame->GetScene()->GetWorld3D()->GetAreaEntity(asArea);
	if(pArea == nullptr)
	{
		Error("Could not find area '%s'\n", asArea.c_str());
		return;
	}

	gpInit->mpEffectHandler->GetSaveEffect()->NormalSave(pArea->m_mtxTransform.GetTranslation(), nullptr, false);
}

/**
 * Black Plague's endings: mails typed on a black screen. 1 goes back to the
 * map afterwards, 2 goes on to the credits.
 */
static void StartEnding1()
{
	gpInit->mpEnding->SetActive(true, 0);
}

static void StartEnding2()
{
	gpInit->mpEnding->SetActive(true, 3);
}

// SOUND ENVIRONMENTS (Black Plague)

static iSoundEnvironment* GetScriptSoundEnv(const tString &asFile)
{
	if(gpInit->mpGame->GetSound()->GetLowLevel()->IsEnvAudioAvailable()==false) return nullptr;

	iSoundEnvironment *pEnv = gpInit->mpSoundEnvHandler->GetSoundEnv(asFile);
	if(pEnv == nullptr) Error("SoundEnv '%s' does not exist!\n", asFile.c_str());
	return pEnv;
}

/**
 * The reverb heard in a sector: "soundenvs/x.snv"
 * afFadeTime: seconds to fade into it; alPrio: where sectors overlap the highest wins
 */
static void LinkSoundEnvToSector(std::string asSector, std::string asSoundEnv, float afFadeTime, int alPrio)
{
	cSector *pSector = gpInit->mpGame->GetScene()->GetWorld3D()->GetPortalContainer()->GetSector(asSector);
	if(pSector == nullptr)
	{
		Error("Sector '%s' does not exist!\n", asSector.c_str());
		return;
	}

	iSoundEnvironment *pEnv = GetScriptSoundEnv(asSoundEnv);
	if(pEnv == nullptr) return;

	gpInit->mpSoundEnvHandler->AddSectorSoundEnv(pSector, pEnv, afFadeTime, alPrio);
}

static void LinkSoundEnvToAllSectors(std::string asSoundEnv, float afFadeTime)
{
	iSoundEnvironment *pEnv = GetScriptSoundEnv(asSoundEnv);
	if(pEnv == nullptr) return;

	tSectorMap *pSectors = gpInit->mpGame->GetScene()->GetWorld3D()->GetPortalContainer()->GetSectorMap();
	for(tSectorMapIt it = pSectors->begin(); it != pSectors->end(); ++it)
	{
		gpInit->mpSoundEnvHandler->AddSectorSoundEnv(it->second, pEnv, afFadeTime, 0);
	}
}

/**
 * The reverb heard outside the sectors that have one
 */
static void SetDefaultSoundEnv(std::string asSoundEnv, float afFadeTime)
{
	iSoundEnvironment *pEnv = GetScriptSoundEnv(asSoundEnv);
	if(pEnv == nullptr) return;

	gpInit->mpSoundEnvHandler->SetDefaultSoundEnv(pEnv, afFadeTime);
}

/**
 * The function is called, once, when the notebook is closed: Func()
 */
static void SetNotebookCloseCallback(std::string asFunc)
{
	gpInit->mpNotebook->SetCloseCallback(asFunc);
}

void cGameScripts::Init()
{
	iLowLevelSystem *pLowLevelSystem = gpInit->mpGame->GetSystem()->GetLowLevel();

	//Game helper
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddToTempString));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddToTempStringTrans));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddToTempStringAction));
	
	//Game general
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(ResetGame));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(StartCredits));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(StartDemoEndText));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(ClearSavedMaps));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(GetActionKeyString));
	
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddMessageTrans));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddMessage));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddMessageTempString));
	
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddSubTitleTrans));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddSubTitle));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddSubTitleTempString));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddRadioMessage));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetRadioOnEndCallback));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetMessagesOverCallback));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(ChangeMap));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetMapGameName));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetMapGameNameTrans));
	
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddNotebookTaskText));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddNotebookTask));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(RemoveNotebookTask));
	
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddNotebookNote));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(StartNumericalPanel));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetInventoryActive));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(FadeIn));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(FadeOut));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(IsFading));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetWideScreenActive));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AutoSave));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(StartFlash));
	
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetWaveGravityActive));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetupWaveGravity));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetDepthOfFieldActive));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetupDepthOfField));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(FocusOnEntity));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetConstantFocusOnEntity));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(PlayGameMusic));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(StopGameMusic));
	
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(StartScreenShake));
	
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(CreateLightFlashAtArea));
	
	//Attack
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(CreateSplashDamage));

	//Game timer
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(CreateTimer));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(DestroyTimer));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetTimerPaused));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetTimerTime));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddTimerTime));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(GetTimerTime));

	//Player
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(GivePlayerDamage));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetPlayerHealth));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(GetPlayerHealth));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetPlayerPose));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetPlayerActive));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(StartPlayerLookAt));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(StopPlayerLookAt));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(StartPlayerFearFilter));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(StopPlayerFearFilter));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetFlashlightDisabled));


	//Inventory
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddPickupCallback));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddUseCallback));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddCombineCallback));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(RemovePickupCallback));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(RemoveUseCallback));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(RemoveCombineCallback));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(HasItem));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(RemoveItem));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(GiveItem));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetInventoryMessage));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetInventoryMessageTrans));

	//Game entity properties
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(ReplaceEntity));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetGameEntityActive));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(GetGameEntityActive));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetGameEntityMaxExamineDist));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetGameEntityMaxInteractDist));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(CreateGameEntityVar));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetGameEntityVar));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddGameEntityVar));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(GetGameEntityVar));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetGameEntityDescriptionTrans));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetGameEntityDescriptionOnceTrans));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetGameEntityDescription));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetGameEntityGameNameTrans));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(ChangeEntityAnimation));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetEntityHealth));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(DamageEntity));

	
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetDoorState));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetObjectInteractMode));
	
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetupLink));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetupLinkLoadText));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetAreaCustomIcon));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddEnemyPatrolNode));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(ClearEnemyPatrolNodes));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetEnemyDeathCallback));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetEnemyAttackCallback));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(GetEnemyHealth));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetEnemyUseTriggers));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(PlayEnemyAnim));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetEnemyAnimOverCallback));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetEnemyHuntCallback));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetEnemyPatrolOverCallback));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetEnemyStopAtLastPatrolNode));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetInfectedWeapon));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddInfectedFlashlight));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetupSecurityCamera));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetSecurityCameraAlerted));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(ShowEnemyPlayer));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetDoorLocked));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetupStickArea));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AllowAttachment));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetLampLit));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetLampLitChangeCallback));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetLampFlicker));
	
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetupLadder));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetupDamageArea));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetupForceArea));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetupLiquidArea));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetupSaveArea));
	
	//Game entity callbacks
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddEntityCollideCallback));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(RemoveEntityCollideCallback));

	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddEntityCallback));
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(RemoveEntityCallback));
	
	//Game sound
	pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(CreateSoundEntityAt));

	//Black Plague and Requiem: more arguments to functions above, and
	//the functions that are not ported
	if(cInit::IsOverture()==false)
	{
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC_AS(ChangeMapLoadImage, "ChangeMap"));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC_AS(SetupLinkLoadImage, "SetupLink"));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC_AS(SetupLinkLoadTextLoadImage, "SetupLinkLoadText"));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC_AS(SetupDamageAreaType, "SetupDamageArea"));

		//Links
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetLinkLocked));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetupLinkLockedProp));

		//Levers and wheels
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetLeverCallback));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetLeverStuck));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(GetLeverState));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetWheelCallback));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetWheelStuck));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(GetWheelState));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(GetWheelAngle));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(GetWheelSpeed));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(ConnectWheelToJoint));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(ConnectWheelToController));

		//Moving things to areas, collision checks, stick areas
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(MoveEntityToArea));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(TeleportBodyToArea));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(TeleportEntityToArea));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(TeleportPlayerToArea));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(CreateEntityAtArea));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(CheckBodyCollision));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(CheckEntityBodyCollision));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(CheckGameEntityCollision));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AttachBodyToStickArea));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(DetachBodyFromStickArea));

		//Doors, player and notebook
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetDoorLockedProperties));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetNoFallDamage));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(TurnOffGlowStick));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(DropFlare));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetLowPowerCallback));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetNotebookCloseCallback));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddRadioMessageAtPos));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(StopAllRadioMessages));

		// Computer terminals
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(OpenScreenGui));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(CloseScreenGui));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetWidgetBool));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(GetWidgetBool));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetWidgetInt));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(GetWidgetInt));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetWidgetFloat));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetWidgetColor));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetWidgetVec2));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetWidgetVec3));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetWidgetString));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetWidgetStringTrans));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(GetWidgetString));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddWidgetItem));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(AddWidgetItemTrans));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(RemoveWidgetItem));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetAttentionWidget));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetWidgetCallback));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(CreateGuiPopUp));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(CreateGuiPopUpTrans));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(CreateGuiTextView));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetRadioOnEndCallbackGui));

		// Effects
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(StartDreamFilter));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(StopDreamFilter));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(StartImageFlash));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetMessageBlackText));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetSubTitleBlackText));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(GetPlayerHasGasMask));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SaveEffectOnArea));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(StartEnding1));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(StartEnding2));

		// Sound environments
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(LinkSoundEnvToSector));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(LinkSoundEnvToAllSectors));
		pLowLevelSystem->AddScriptFunc(SCRIPT_REGISTER_FUNC(SetDefaultSoundEnv));

		InitNotPorted(pLowLevelSystem);
	}

}
