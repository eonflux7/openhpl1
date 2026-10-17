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
#include "core/StdAfx.h"
#include "core/GameScripts.h"

#include "core/Init.h"
#include "core/MapHandler.h"
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

//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// SCRIPT FUNCTIONS
//////////////////////////////////////////////////////////////////////////

///////// HELPERS ////////////////////////////////////

//-----------------------------------------------------------------------

#define GAME_ENTITY_BEGIN(asName) iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName); \
	if(pEntity==NULL){ \
	Warning("Couldn't find game entity '%s'\n",asName.c_str()); \
	return; } \

//-----------------------------------------------------------------------

tWString gsTempString = _W("");

//-----------------------------------------------------------------------

static void __stdcall AddToTempString(std::string asString)
{
    gsTempString += cString::To16Char(asString);
}
SCRIPT_DEFINE_FUNC_1(void, AddToTempString, string)

static void __stdcall AddToTempStringTrans(std::string asCat,std::string asEntry)
{
	cInit *mpInit = gpInit;
	gsTempString += kTranslate(asCat,asEntry);
}
SCRIPT_DEFINE_FUNC_2(void, AddToTempStringTrans, string,string)

static void __stdcall AddToTempStringAction(std::string asAction)
{
	cInit *mpInit = gpInit;
	iAction *pAction = gpInit->mpGame->GetInput()->GetAction(asAction);
	if(pAction)
	{
		tWString sString = kTranslate("ButtonNames",pAction->GetInputName());
		if(sString != _W(""))
			gsTempString += sString;
		else 
			gsTempString += cString::To16Char(pAction->GetInputName());
	}
	else
	{
		gsTempString += kTranslate("ButtonNames", "None");
	}
}
SCRIPT_DEFINE_FUNC_1(void, AddToTempStringAction,string)


//-----------------------------------------------------------------------


///////// GENERAL GAME ////////////////////////////////////

static void __stdcall ResetGame()
{
	gpInit->mpGame->ResetLogicTimer();

	gpInit->mpMapHandler->SetCurrentMapName("");
	gpInit->mpMainMenu->SetActive(true);
}
SCRIPT_DEFINE_FUNC(void, ResetGame)

//-----------------------------------------------------------------------

static void __stdcall StartCredits()
{
	gpInit->mpCredits->SetActive(true);
}
SCRIPT_DEFINE_FUNC(void, StartCredits)

static void __stdcall StartDemoEndText()
{
	gpInit->mpDemoEndText->SetActive(true);
}
SCRIPT_DEFINE_FUNC(void, StartDemoEndText)

//-----------------------------------------------------------------------

static void __stdcall ClearSavedMaps()
{
	gpInit->mpSaveHandler->ClearSavedMaps();
}
SCRIPT_DEFINE_FUNC(void, ClearSavedMaps)

//-----------------------------------------------------------------------

static std::string __stdcall GetActionKeyString(std::string asAction)
{
	iAction *pAction = gpInit->mpGame->GetInput()->GetAction(asAction);
	if(pAction)
	{
		return pAction->GetInputName();
	}
	
	return "ActionNotFound";
}
SCRIPT_DEFINE_FUNC_1(string, GetActionKeyString, string)

//-----------------------------------------------------------------------

static void __stdcall AddMessageTrans(std::string asTransCat,
									  std::string asTransName)
{
	cInit *mpInit = gpInit;
	gpInit->mpGameMessageHandler->Add(kTranslate(asTransCat,asTransName));
}
SCRIPT_DEFINE_FUNC_2(void, AddMessageTrans, string, string)

//-----------------------------------------------------------------------

static void __stdcall AddMessage(std::string asMessage)
{
	gpInit->mpGameMessageHandler->Add(cString::To16Char(asMessage));
}
SCRIPT_DEFINE_FUNC_1(void, AddMessage, string)

//-----------------------------------------------------------------------

static void __stdcall AddMessageTempString()
{
	gpInit->mpGameMessageHandler->Add(gsTempString);
	gsTempString = _W("");
}
SCRIPT_DEFINE_FUNC(void, AddMessageTempString)

//-----------------------------------------------------------------------

static void __stdcall AddSubTitleTrans(std::string asTransCat,
									  std::string asTransName, float afTime)
{
	cInit *mpInit = gpInit;
	gpInit->mpEffectHandler->GetSubTitle()->Add(kTranslate(asTransCat,asTransName),afTime,false);
}
SCRIPT_DEFINE_FUNC_3(void, AddSubTitleTrans, string, string, float)

//-----------------------------------------------------------------------

static void __stdcall AddSubTitle(std::string asMessage, float afTime)
{
	gpInit->mpEffectHandler->GetSubTitle()->Add(cString::To16Char(asMessage),afTime,false);
}
SCRIPT_DEFINE_FUNC_2(void, AddSubTitle, string, float)

//-----------------------------------------------------------------------

static void __stdcall AddSubTitleTempString(float afTime)
{
	gpInit->mpEffectHandler->GetSubTitle()->Add(gsTempString,afTime,false);
	gsTempString = _W("");
}
SCRIPT_DEFINE_FUNC_1(void, AddSubTitleTempString, float)


//-----------------------------------------------------------------------

static void __stdcall AddRadioMessage(	std::string asTransCat,std::string asTransName, 
												std::string asSound)
{
	cInit *mpInit = gpInit;
	gpInit->mpRadioHandler->Add(kTranslate(asTransCat,asTransName),asSound);
}
SCRIPT_DEFINE_FUNC_3(void, AddRadioMessage, string, string, string)

//-----------------------------------------------------------------------

static void __stdcall SetRadioOnEndCallback(std::string asFunc)
{
	gpInit->mpRadioHandler->SetOnEndCallback(asFunc);
}
SCRIPT_DEFINE_FUNC_1(void, SetRadioOnEndCallback, string)

//-----------------------------------------------------------------------

static void __stdcall SetInventoryMessage(std::string asMessage)
{
	gpInit->mpInventory->SetMessage(cString::To16Char(asMessage));
}
SCRIPT_DEFINE_FUNC_1(void, SetInventoryMessage, string)

static void __stdcall SetInventoryMessageTrans(std::string asTransCat,
											   std::string asTransName)
{
	cInit *mpInit = gpInit;
	gpInit->mpInventory->SetMessage(kTranslate(asTransCat,asTransName));
}
SCRIPT_DEFINE_FUNC_2(void, SetInventoryMessageTrans, string, string)							

//-----------------------------------------------------------------------

static void __stdcall SetMessagesOverCallback(std::string asFunction)
{
	gpInit->mpGameMessageHandler->SetOnMessagesOverCallback(asFunction);
}
SCRIPT_DEFINE_FUNC_1(void, SetMessagesOverCallback, string)

//-----------------------------------------------------------------------

static void __stdcall ChangeMap(std::string asMapFile, std::string asMapPos, 
								std::string asStartSound, std::string asStopSound,
								float afFadeOutTime, float afFadeInTime,
								std::string asLoadTextCat, std::string asLoadTextEntry)
{
	//if(gpInit->mpRadioHandler->IsActive()) return;

	gpInit->mpMapHandler->ChangeMap(asMapFile,asMapPos, asStartSound,asStopSound,
									afFadeOutTime,afFadeInTime,
									asLoadTextCat,asLoadTextEntry);
}
SCRIPT_DEFINE_FUNC_8(void, ChangeMap, string, string, string, string, float, float, string, string)

// Black Plague: with the picture of the loading screen
static void __stdcall ChangeMapLoadImage(std::string asMapFile, std::string asMapPos,
								std::string asStartSound, std::string asStopSound,
								float afFadeOutTime, float afFadeInTime,
								std::string asLoadTextCat, std::string asLoadTextEntry,
								std::string asLoadImage)
{
	gpInit->mpMapHandler->ChangeMap(asMapFile, asMapPos,asStartSound, asStopSound,
									afFadeOutTime, afFadeInTime, asLoadTextCat, asLoadTextEntry, asLoadImage);
}
SCRIPT_DEFINE_FUNC_9(void, ChangeMapLoadImage, string, string, string, string, float, float, string, string, string)

//-----------------------------------------------------------------------

static void __stdcall SetMapGameName(std::string asName)
{
	gpInit->mpMapHandler->SetMapGameName(cString::To16Char(asName));
}
SCRIPT_DEFINE_FUNC_1(void, SetMapGameName, string)

static void __stdcall SetMapGameNameTrans(std::string asTransCat,std::string asTransEntry)
{
	cInit *mpInit = gpInit;
	gpInit->mpMapHandler->SetMapGameName(kTranslate(asTransCat,asTransEntry));
}
SCRIPT_DEFINE_FUNC_2(void, SetMapGameNameTrans, string, string)

//-----------------------------------------------------------------------

static void __stdcall AddNotebookTaskText(std::string asName,std::string asText) 
{
	gpInit->mpNotebook->AddTask(asName, cString::To16Char(asText));
}
SCRIPT_DEFINE_FUNC_2(void, AddNotebookTaskText, string, string)

//-----------------------------------------------------------------------

static void __stdcall AddNotebookTask(std::string asName, std::string asTransCat,
									  std::string asTransEntry)
{
	cInit *mpInit = gpInit;
	gpInit->mpNotebook->AddTask(asName, kTranslate(asTransCat,asTransEntry));
}	
SCRIPT_DEFINE_FUNC_3(void, AddNotebookTask, string, string, string)

//-----------------------------------------------------------------------

static void __stdcall RemoveNotebookTask(std::string asName)
{
	gpInit->mpNotebook->RemoveTask(asName);
}
SCRIPT_DEFINE_FUNC_1(void, RemoveNotebookTask, string)

//-----------------------------------------------------------------------

static void __stdcall AddNotebookNote(std::string asNameCat, std::string asNameEntry,
											 std::string asTextCat,	std::string asTextEntry)
{
	cInit *mpInit = gpInit;
	gpInit->mpNotebook->AddNote(kTranslate(asNameCat,asNameEntry),asTextCat,asTextEntry);
}
SCRIPT_DEFINE_FUNC_4(void, AddNotebookNote, string, string, string, string)

//-----------------------------------------------------------------------

static void __stdcall StartNumericalPanel(std::string asName,int alCode1,int alCode2,int alCode3,int alCode4,
										  float afDifficulty, std::string asCallback)
{
	gpInit->mpNumericalPanel->SetActive(true);
	tIntVec vCode; vCode.resize(4);
	vCode[0] = alCode1; vCode[1] = alCode2; vCode[2] = alCode3; vCode[3] = alCode4;
	
	gpInit->mpNumericalPanel->SetUp(asName,asCallback);
	gpInit->mpNumericalPanel->SetCode(vCode);
}
SCRIPT_DEFINE_FUNC_7(void, StartNumericalPanel, string, int, int, int, int, float, string)

//-----------------------------------------------------------------------

static void __stdcall SetInventoryActive(bool abX)
{
	gpInit->mpInventory->SetActive(abX);
}
SCRIPT_DEFINE_FUNC_1(void,SetInventoryActive,bool)

//-----------------------------------------------------------------------

static void __stdcall FadeIn(float afTime)
{
	gpInit->mpFadeHandler->FadeIn(afTime);
}
SCRIPT_DEFINE_FUNC_1(void,FadeIn,float)

static void __stdcall FadeOut(float afTime)
{
	gpInit->mpFadeHandler->FadeOut(afTime);
}
SCRIPT_DEFINE_FUNC_1(void,FadeOut,float)

static bool __stdcall IsFading()
{
	return gpInit->mpFadeHandler->IsActive();
}
SCRIPT_DEFINE_FUNC(bool,IsFading)

//-----------------------------------------------------------------------

static void __stdcall SetWideScreenActive(bool abActive)
{
	gpInit->mpFadeHandler->SetWideScreenActive(abActive);
}
SCRIPT_DEFINE_FUNC_1(void,SetWideScreenActive,bool)

//-----------------------------------------------------------------------

static void __stdcall AutoSave()
{
	if(gpInit->mpPlayer->GetHealth()<=0) return;

	//TODO: Some other effect here.
	//gpInit->mpGraphicsHelper->DrawLoadingScreen("other_saving.jpg");
	//gpInit->mpSaveHandler->SaveGameToFile("save/auto.sav");
	//gpInit->mpSaveHandler->AutoSave("Auto",5);
	gpInit->mpEffectHandler->GetSaveEffect()->AutoSave();
}
SCRIPT_DEFINE_FUNC(void,AutoSave)

//-----------------------------------------------------------------------

static void __stdcall StartFlash(float afFadeIn, float afWhite, float afFadeOut)
{
	gpInit->mpEffectHandler->GetFlash()->Start(afFadeIn,afWhite,afFadeOut);
}
SCRIPT_DEFINE_FUNC_3(void,StartFlash,float,float,float)

//-----------------------------------------------------------------------

static void __stdcall SetWaveGravityActive(bool abX)
{
	gpInit->mpEffectHandler->GetWaveGravity()->SetActive(abX);
}
SCRIPT_DEFINE_FUNC_1(void,SetWaveGravityActive,bool)

static void __stdcall SetupWaveGravity(float afMaxAngle, float afSwingLength, 
									   float afGravitySize, std::string asAxis)
{
	int lDir = cString::ToLowerCase(asAxis) == "x" ? 0 : 1;

	gpInit->mpEffectHandler->GetWaveGravity()->Setup(cMath::ToRad(afMaxAngle),afSwingLength,afGravitySize,lDir);
}
SCRIPT_DEFINE_FUNC_4(void, SetupWaveGravity, float, float, float, string)

//-----------------------------------------------------------------------

static void __stdcall SetDepthOfFieldActive(bool abX, float afFadeTime)
{
	gpInit->mpEffectHandler->GetDepthOfField()->SetActive(abX,afFadeTime);
}
SCRIPT_DEFINE_FUNC_2(void,SetDepthOfFieldActive,bool,float)

//-----------------------------------------------------------------------

static void __stdcall SetupDepthOfField(float afNearPlane, float afFocalPlane, float afFarPlane)
{
	gpInit->mpEffectHandler->GetDepthOfField()->SetUp(afNearPlane,afFocalPlane,afFarPlane);
	gpInit->mpEffectHandler->GetDepthOfField()->SetFocusBody(nullptr);
}
SCRIPT_DEFINE_FUNC_3(void,SetupDepthOfField,float,float,float)

//-----------------------------------------------------------------------

static void __stdcall FocusOnEntity(std::string asEntity)
{
	GAME_ENTITY_BEGIN(asEntity);
	
	if(pEntity->GetBodyNum()==0) {
		Error("Entity %s had no bodies and can not be focus on.\n",pEntity->GetName().c_str());
		return;
	}

	gpInit->mpEffectHandler->GetDepthOfField()->FocusOnBody(pEntity->GetBody(0));
	gpInit->mpEffectHandler->GetDepthOfField()->SetFocusBody(nullptr);
}
SCRIPT_DEFINE_FUNC_1(void,FocusOnEntity,string)

//-----------------------------------------------------------------------

static void __stdcall SetConstantFocusOnEntity(std::string asEntity)
{
	if(asEntity=="")
	{
		gpInit->mpEffectHandler->GetDepthOfField()->SetFocusBody(nullptr);
		return;
	}

	GAME_ENTITY_BEGIN(asEntity);

	if(pEntity->GetBodyNum()==0) {
		Error("Entity %s had no bodies and can not be focus on.\n",pEntity->GetName().c_str());
			return;
	}

	gpInit->mpEffectHandler->GetDepthOfField()->SetFocusBody(pEntity->GetBody(0));
}
SCRIPT_DEFINE_FUNC_1(void,SetConstantFocusOnEntity,string)

//-----------------------------------------------------------------------

static void __stdcall PlayGameMusic(	std::string asFile, float afVolume,float afFadeStep,
											bool abLoop, int alPrio)
{
	gpInit->mpMusicHandler->Play(asFile,abLoop,afVolume,afFadeStep,alPrio);
}
SCRIPT_DEFINE_FUNC_5(void, PlayGameMusic, string, float, float, bool, int)

//-----------------------------------------------------------------------

static void __stdcall StopGameMusic(float afFadeStep, int alPrio)
{
	gpInit->mpMusicHandler->Stop(afFadeStep,alPrio);
}
SCRIPT_DEFINE_FUNC_2(void,StopGameMusic,float,int)

//-----------------------------------------------------------------------

static void __stdcall StartScreenShake(float afAmount, float afTime,float afFadeInTime,float afFadeOutTime)
{
	gpInit->mpEffectHandler->GetShakeScreen()->Start(afAmount,afTime,afFadeInTime,afFadeOutTime);
}
SCRIPT_DEFINE_FUNC_4(void,StartScreenShake,float,float,float,float)

//-----------------------------------------------------------------------

static void __stdcall CreateLightFlashAtArea(std::string asArea, float afRadius,
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
SCRIPT_DEFINE_FUNC_8(void, CreateLightFlashAtArea, string, float, float, float, float, float, float, float)

//-----------------------------------------------------------------------


///////// ATTACK ////////////////////////////////////

//-----------------------------------------------------------------------

static void __stdcall CreateSplashDamage(std::string asAreaName, float afRadius, 
										 float afMinDamage, float afMaxDamge,
										 float afMinForce, float afMaxForce,
										 float afMaxImpulse, int alStrength)
{	
	///////////////////
	//Get area
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asAreaName);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Area)
	{
		Warning("Couldn't find area entity '%s'\n",asAreaName.c_str());
		return;
	}
	cGameArea *pArea = static_cast<cGameArea*>(pEntity);

    gpInit->mpAttackHandler->CreateSplashDamage(pArea->GetBody(0)->GetWorldPosition(),
												afRadius,afMinDamage,afMaxDamge,
												afMinForce, afMaxForce, afMaxImpulse,
												eAttackTargetFlag_Bodies | 
												eAttackTargetFlag_Player |
												eAttackTargetFlag_Enemy,
												1, alStrength);
}
SCRIPT_DEFINE_FUNC_8(void, CreateSplashDamage, string, float, float, float, float, float, float, int)
//-----------------------------------------------------------------------

///////// GAME TIMER ////////////////////////////////////

//-----------------------------------------------------------------------

static void __stdcall  CreateTimer(std::string asName, float afTime, std::string asCallback, bool abGlobal)
{
	gpInit->mpMapHandler->CreateTimer(asName,afTime,asCallback,abGlobal);
}
SCRIPT_DEFINE_FUNC_4(void,CreateTimer,string,float,string,bool)

static void __stdcall  DestroyTimer(std::string asName)
{
	cGameTimer *pTimer = gpInit->mpMapHandler->GetTimer(asName);
	if(pTimer==nullptr){
		Warning("Couldn't find timer '%s'\n",asName.c_str());
		return;
	}
	
	pTimer->mbDeleteMe = true;
}
SCRIPT_DEFINE_FUNC_1(void,DestroyTimer,string)

static void __stdcall  SetTimerPaused(std::string asName, bool abPaused)
{
	cGameTimer *pTimer = gpInit->mpMapHandler->GetTimer(asName);
	if(pTimer==nullptr){
		Warning("Couldn't find timer '%s'\n",asName.c_str());
		return;
	}

	pTimer->mbPaused = abPaused;
}
SCRIPT_DEFINE_FUNC_2(void,SetTimerPaused,string,bool)

static void __stdcall SetTimerTime(std::string asName, float afTime)
{
	cGameTimer *pTimer = gpInit->mpMapHandler->GetTimer(asName);
	if(pTimer==nullptr){
		Warning("Couldn't find timer '%s'\n",asName.c_str());
		return;
	}

	pTimer->mfTime = afTime;
}
SCRIPT_DEFINE_FUNC_2(void,SetTimerTime,string,float)

static void __stdcall  AddTimerTime(std::string asName, float afTime)
{
	cGameTimer *pTimer = gpInit->mpMapHandler->GetTimer(asName);
	if(pTimer==nullptr){
		Warning("Couldn't find timer '%s'\n",asName.c_str());
		return;
	}

	pTimer->mfTime += afTime;
}
SCRIPT_DEFINE_FUNC_2(void,AddTimerTime,string,float)

static float __stdcall   GetTimerTime(std::string asName)
{
	cGameTimer *pTimer = gpInit->mpMapHandler->GetTimer(asName);
	if(pTimer==nullptr){
		Warning("Couldn't find timer '%s'\n",asName.c_str());
		return 0.0f;
	}

	return pTimer->mfTime;
}
SCRIPT_DEFINE_FUNC_1(float,GetTimerTime,string)

//-----------------------------------------------------------------------

///////// PLAYER ////////////////////////////////////

//-----------------------------------------------------------------------


static void __stdcall GivePlayerDamage(float afAmount, std::string asType)
{
	ePlayerDamageType type = ePlayerDamageType_BloodSplash;
	tString sLowType = cString::ToLowerCase(asType);

	if(sLowType == "bloodsplash") type = ePlayerDamageType_BloodSplash;
	else if(sLowType == "ice") type = ePlayerDamageType_Ice;
	else 
		Warning("Damage type %s does not exist!\n",asType.c_str());



	gpInit->mpPlayer->Damage(afAmount,type);
}
SCRIPT_DEFINE_FUNC_2(void,GivePlayerDamage,float,string)

//-----------------------------------------------------------------------

static void __stdcall SetPlayerHealth(float afHealth)
{
	gpInit->mpPlayer->SetHealth(afHealth);
}
SCRIPT_DEFINE_FUNC_1(void,SetPlayerHealth,float)

static float __stdcall GetPlayerHealth()
{
	return gpInit->mpPlayer->GetHealth();
}
SCRIPT_DEFINE_FUNC(float,GetPlayerHealth)

//-----------------------------------------------------------------------

static void __stdcall SetPlayerPose(std::string asPose,bool abChangeDirectly)
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
SCRIPT_DEFINE_FUNC_2(void,SetPlayerPose,string,bool)

static void __stdcall SetPlayerActive(bool abActive)
{
	gpInit->mpPlayer->SetActive(abActive);
}
SCRIPT_DEFINE_FUNC_1(void,SetPlayerActive,bool)

//-----------------------------------------------------------------------

static void __stdcall StartPlayerLookAt(std::string asEntityName, float afSpeedMul, float afMaxSpeed)
{
	///////////////////
	//Get entity
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asEntityName);
	if(pEntity==nullptr)
	{
		Warning("Couldn't find entity '%s'\n",asEntityName.c_str());
		return;
	}
	if(pEntity->GetBody(0)==nullptr)
	{
		Warning("Couldn't find a body in entity '%s'\n",asEntityName.c_str());
		return;
	}
	
	gpInit->mpPlayer->GetLookAt()->SetTarget(pEntity->GetBody(0)->GetWorldPosition(),afSpeedMul,afMaxSpeed);
	gpInit->mpPlayer->GetLookAt()->SetActive(true);
}
SCRIPT_DEFINE_FUNC_3(void,StartPlayerLookAt,string,float,float)

static void __stdcall StopPlayerLookAt()
{
	gpInit->mpPlayer->GetLookAt()->SetActive(false);
}
SCRIPT_DEFINE_FUNC(void,StopPlayerLookAt)

//-----------------------------------------------------------------------

static void __stdcall StartPlayerFearFilter(float afStrength)
{
	gpInit->mpPlayer->GetFearFilter()->SetActive(true);
}
SCRIPT_DEFINE_FUNC_1(void,StartPlayerFearFilter,float)

static void __stdcall StopPlayerFearFilter()
{
	gpInit->mpPlayer->GetFearFilter()->SetActive(false);
}
SCRIPT_DEFINE_FUNC(void,StopPlayerFearFilter)

//-----------------------------------------------------------------------

static void __stdcall SetFlashlightDisabled(bool abDisabled)
{
	gpInit->mpPlayer->GetFlashLight()->SetDisabled(abDisabled);
}
SCRIPT_DEFINE_FUNC_1(void,SetFlashlightDisabled,bool)

//-----------------------------------------------------------------------

///////// INVENTORY ////////////////////////////////////

//-----------------------------------------------------------------------


static void __stdcall AddPickupCallback(std::string asItem, std::string asFunction)
{
	gpInit->mpInventory->AddPickupCallback(asItem,asFunction);
}
SCRIPT_DEFINE_FUNC_2(void,AddPickupCallback,string,string)

static void __stdcall AddUseCallback(std::string asItem, std::string asEntity, std::string asFunction)
{
	gpInit->mpInventory->AddUseCallback(asItem,asEntity,asFunction);
}
SCRIPT_DEFINE_FUNC_3(void,AddUseCallback,string,string,string)

static void __stdcall AddCombineCallback(std::string asItem1,std::string asItem2, std::string asFunction)
{
	gpInit->mpInventory->AddCombineCallback(asItem1,asItem2,asFunction);
}
SCRIPT_DEFINE_FUNC_3(void,AddCombineCallback,string,string,string)

//-----------------------------------------------------------------------

static void __stdcall RemovePickupCallback(std::string asFunction)
{
	gpInit->mpInventory->RemovePickupCallback(asFunction);
}
SCRIPT_DEFINE_FUNC_1(void,RemovePickupCallback,string)

static void __stdcall RemoveUseCallback(std::string asFunction)
{
	gpInit->mpInventory->RemoveUseCallback(asFunction);
}
SCRIPT_DEFINE_FUNC_1(void,RemoveUseCallback,string)

static void __stdcall RemoveCombineCallback(std::string asFunction)
{
	gpInit->mpInventory->RemoveCombineCallback(asFunction);
}
SCRIPT_DEFINE_FUNC_1(void,RemoveCombineCallback,string)

//-----------------------------------------------------------------------

static bool __stdcall HasItem(std::string asName)
{
	//if(cString::ToLowerCase(asName)=="notebook") return gpInit->mpInventory->GetNoteBookActive();

	return gpInit->mpInventory->GetItem(asName) != nullptr;
}
SCRIPT_DEFINE_FUNC_1(bool,HasItem,string)

static void __stdcall RemoveItem(std::string asName)
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
SCRIPT_DEFINE_FUNC_1(void,RemoveItem,string)

static void __stdcall GiveItem(std::string asName,std::string asEntityFile,int alSlotIndex)
{
	gpInit->mpInventory->AddItemFromFile(asName,asEntityFile, alSlotIndex);
}
SCRIPT_DEFINE_FUNC_3(void,GiveItem,string,string,int)

//-----------------------------------------------------------------------

///////// GAME ENTITY PROPERTIES //////////////////////////////////


//-----------------------------------------------------------------------

static void __stdcall ReplaceEntity(std::string asName, std::string asBodyName,
											   std::string asNewName, std::string asNewFile)
{
	GAME_ENTITY_BEGIN(asName)

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
SCRIPT_DEFINE_FUNC_4(void, ReplaceEntity, string, string, string, string)

//-----------------------------------------------------------------------

static void __stdcall SetGameEntityActive(std::string asName, bool abX)
{
	GAME_ENTITY_BEGIN(asName)

	pEntity->SetActive(abX);
}
SCRIPT_DEFINE_FUNC_2(void,SetGameEntityActive,string,bool)

static bool __stdcall GetGameEntityActive(std::string asName)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName); 
	if(pEntity==nullptr){ 
		Warning("Couldn't find game entity '%s'\n",asName.c_str()); 
		return false; 
	}

	return pEntity->IsActive();
}
SCRIPT_DEFINE_FUNC_1(bool,GetGameEntityActive,string)

//-----------------------------------------------------------------------

static void __stdcall CreateGameEntityVar(std::string asEntName, std::string asVarName, int alVal)
{
	GAME_ENTITY_BEGIN(asEntName);

	pEntity->CreateVar(asVarName,alVal);
}
SCRIPT_DEFINE_FUNC_3(void,CreateGameEntityVar,string,string,int)

static void __stdcall SetGameEntityVar(std::string asEntName, std::string asVarName, int alVal)
{
	GAME_ENTITY_BEGIN(asEntName);

	pEntity->SetVar(asVarName,alVal);
}
SCRIPT_DEFINE_FUNC_3(void,SetGameEntityVar,string,string,int)

static void __stdcall AddGameEntityVar(std::string asEntName, std::string asVarName, int alVal)
{
	GAME_ENTITY_BEGIN(asEntName);

	pEntity->AddVar(asVarName,alVal);
}
SCRIPT_DEFINE_FUNC_3(void,AddGameEntityVar,string,string,int)

static int __stdcall GetGameEntityVar(std::string asEntName, std::string asVarName)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asEntName); 
	if(pEntity==nullptr){ 
		Warning("Couldn't find game entity '%s'\n",asEntName.c_str()); 
		return 0; 
	}

	return pEntity->GetVar(asVarName);
}
SCRIPT_DEFINE_FUNC_2(int,GetGameEntityVar,string,string)

//-----------------------------------------------------------------------

static void __stdcall SetGameEntityMaxExamineDist(std::string asName,float afDist)
{
	GAME_ENTITY_BEGIN(asName)
	
	pEntity->SetMaxExamineDist(afDist);
}
SCRIPT_DEFINE_FUNC_2(void,SetGameEntityMaxExamineDist,string,float)

static void __stdcall SetGameEntityMaxInteractDist(std::string asName,float afDist)
{
	GAME_ENTITY_BEGIN(asName)

	pEntity->SetMaxInteractDist(afDist);
}
SCRIPT_DEFINE_FUNC_2(void,SetGameEntityMaxInteractDist,string,float)

//-----------------------------------------------------------------------

static void __stdcall SetGameEntityDescriptionTrans(std::string asName, 
													std::string asTransCat,
													std::string asTransName)
{
	cInit *mpInit = gpInit;
	GAME_ENTITY_BEGIN(asName)

	pEntity->SetDescription(kTranslate(asTransCat,asTransName));
	pEntity->SetShowDescritionOnce(false);
}
SCRIPT_DEFINE_FUNC_3(void, SetGameEntityDescriptionTrans, string, string, string)

//-----------------------------------------------------------------------

static void __stdcall SetGameEntityDescriptionOnceTrans(std::string asName, 
														   std::string asTransCat,
														   std::string asTransName)
{
	cInit *mpInit = gpInit;
	GAME_ENTITY_BEGIN(asName)

	pEntity->SetDescription(kTranslate(asTransCat,asTransName));
	pEntity->SetShowDescritionOnce(true);
}
SCRIPT_DEFINE_FUNC_3(void, SetGameEntityDescriptionOnceTrans, string, string, string)

//-----------------------------------------------------------------------

static void __stdcall SetGameEntityDescription(std::string asName, std::string asMessage)
{
	GAME_ENTITY_BEGIN(asName)

	pEntity->SetDescription(cString::To16Char(asMessage));
	pEntity->SetShowDescritionOnce(false);
}
SCRIPT_DEFINE_FUNC_2(void,SetGameEntityDescription,string,string)

static void __stdcall SetGameEntityGameNameTrans(std::string asName, 
												 std::string asTransCat,
												 std::string asTransName)
{
	cInit *mpInit = gpInit;
	GAME_ENTITY_BEGIN(asName)

	pEntity->SetGameName(kTranslate(asTransCat,asTransName));
}
SCRIPT_DEFINE_FUNC_3(void, SetGameEntityGameNameTrans, string, string, string)

//-----------------------------------------------------------------------

static void __stdcall ChangeEntityAnimation(std::string asName,
											std::string asAnimation,
											bool abLoop)
{
	GAME_ENTITY_BEGIN(asName)

 	pEntity->GetMeshEntity()->PlayName(asAnimation,abLoop, true);
}
SCRIPT_DEFINE_FUNC_3(void, ChangeEntityAnimation, string, string, bool)

//-----------------------------------------------------------------------

static void __stdcall SetEntityHealth(std::string asName, float afHealth)
{
	GAME_ENTITY_BEGIN(asName)

	pEntity->SetHealth(afHealth);
}
SCRIPT_DEFINE_FUNC_2(void,SetEntityHealth,string,float)

static void __stdcall DamageEntity(std::string asName, float afDamage, int alStrength)
{
	GAME_ENTITY_BEGIN(asName)

	pEntity->Damage(afDamage,alStrength);
}
SCRIPT_DEFINE_FUNC_3(void,DamageEntity,string,float,int)

//-----------------------------------------------------------------------

static void __stdcall SetDoorState(std::string asName, 
											std::string asState)
{
	/*iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==NULL || pEntity->GetType() != eGameEntityType_Door)
	{
		Warning("Couldn't find door entity '%s'\n",asName.c_str());
		return;
	}

	cGameDoor *pDoor = static_cast<cGameDoor*>(pEntity);

	asState = cString::ToLowerCase(asState);
	eGameDoorState DoorState;

    if(asState == "open") DoorState = eGameDoorState_Open;
	else if(asState == "closed") DoorState = eGameDoorState_Closed;
	else if(asState == "opening") DoorState = eGameDoorState_Opening;
	else if(asState == "closing") DoorState = eGameDoorState_Closing;
	else{
		Warning("Invalid door state '%s'\n",asState.c_str());
		return;
	}

	pDoor->ChangeDoorState(DoorState);*/
}
SCRIPT_DEFINE_FUNC_2(void, SetDoorState, string, string)

//-----------------------------------------------------------------------

static void __stdcall SetObjectInteractMode(std::string asName,std::string asMode)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Object)
	{
		Warning("Couldn't find object entity '%s'\n",asName.c_str());
		return;
	}

	cGameObject *pObject = static_cast<cGameObject*>(pEntity);

	pObject->SetInteractMode(cEntityLoader_GameObject::ToInteractMode(asMode.c_str()));
}
SCRIPT_DEFINE_FUNC_2(void,SetObjectInteractMode,string,string)

//-----------------------------------------------------------------------

static void __stdcall SetupLink(std::string asName,
								std::string asMapFile, std::string asMapPos, 
								std::string asStartSound, std::string asStopSound,
								float afFadeOutTime, float afFadeInTime)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Link)
	{
		Warning("Couldn't find object entity '%s'\n",asName.c_str());
		return;
	}

	cGameLink *pLink = static_cast<cGameLink*>(pEntity);

	pLink->msMapFile = asMapFile;
	pLink->msMapPos = asMapPos;
	pLink->msStartSound = asStartSound;
	pLink->msStopSound = asStopSound;
	pLink->mfFadeInTime = afFadeInTime;
	pLink->mfFadeOutTime = afFadeOutTime;
	pLink->msLoadTextCat = "";
	pLink->msLoadTextEntry = "";
}
SCRIPT_DEFINE_FUNC_7(void, SetupLink, string, string, string, string, string, float, float)

// Black Plague: with the picture of the loading screen
static void __stdcall SetupLinkLoadImage(std::string asName,
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
SCRIPT_DEFINE_FUNC_8(void, SetupLinkLoadImage, string, string, string, string, string, float, float, string)

//-----------------------------------------------------------------------

static void __stdcall SetupLinkLoadText(std::string asName,
										std::string asMapFile, std::string asMapPos, 
										std::string asStartSound, std::string asStopSound,
										float afFadeOutTime, float afFadeInTime,
										std::string asTextCat, std::string asTextEntry)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Link)
	{
		Warning("Couldn't find object entity '%s'\n",asName.c_str());
		return;
	}

	cGameLink *pLink = static_cast<cGameLink*>(pEntity);

	pLink->msMapFile = asMapFile;
	pLink->msMapPos = asMapPos;
	pLink->msStartSound = asStartSound;
	pLink->msStopSound = asStopSound;
	pLink->mfFadeInTime = afFadeInTime;
	pLink->mfFadeOutTime = afFadeOutTime;
	pLink->msLoadTextCat = asTextCat;
	pLink->msLoadTextEntry = asTextEntry;
}
SCRIPT_DEFINE_FUNC_9(void, SetupLinkLoadText, string, string, string, string, string, float, float, string, string)

static void __stdcall SetupLinkLoadTextLoadImage(std::string asName,
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

//-----------------------------------------------------------------------

/**
 * Black Plague: a locked link does not change the map
 */
static void __stdcall SetLinkLocked(std::string asName, bool abLocked)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Link)
	{
		Warning("Couldn't find object entity '%s'\n",asName.c_str());
		return;
	}

	static_cast<cGameLink*>(pEntity)->mbLocked = abLocked;
}
SCRIPT_DEFINE_FUNC_2(void, SetLinkLocked, string, bool)

/**
 * Black Plague: the sound and the message of a locked link
 */
static void __stdcall SetupLinkLockedProp(std::string asName, std::string asLockedSound,
											std::string asLockedDescCat, std::string asLockedDescEntry)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Link)
	{
		Warning("Couldn't find object entity '%s'\n",asName.c_str());
		return;
	}

	cGameLink *pLink = static_cast<cGameLink*>(pEntity);

	pLink->msLockedSound = asLockedSound;
	if(asLockedDescCat != "")
		pLink->msLockedDesc = gpInit->mpGame->GetResources()->Translate(asLockedDescCat, asLockedDescEntry);
}
SCRIPT_DEFINE_FUNC_4(void, SetupLinkLockedProp, string, string, string, string)
SCRIPT_DEFINE_FUNC_10(void, SetupLinkLoadTextLoadImage, string, string, string, string, string, float, float, string, string, string)

//-----------------------------------------------------------------------

static void __stdcall SetAreaCustomIcon(std::string asName, std::string asIcon)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Area)
	{
		Warning("Couldn't find area entity '%s'\n",asName.c_str());
		return;
	}
	cGameArea *pArea = static_cast<cGameArea*>(pEntity);

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
SCRIPT_DEFINE_FUNC_2(void,SetAreaCustomIcon,string,string)

//-----------------------------------------------------------------------

static void __stdcall AddEnemyPatrolNode(std::string asEnemy,std::string asNode,float afTime,
														std::string asAnimation)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asEnemy);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Enemy)
	{
		Warning("Couldn't find enemy entity '%s'\n",asEnemy.c_str());
		return;
	}
	iGameEnemy *pEnemy = static_cast<iGameEnemy*>(pEntity);
    
    pEnemy->AddPatrolNode(asNode,afTime,asAnimation);
}
SCRIPT_DEFINE_FUNC_4(void, AddEnemyPatrolNode,string, string, float, string)

/**
* Clears all the patrol nodes for the enemy
* \param asEnemy The Name of the enemy.
*/
static void __stdcall ClearEnemyPatrolNodes(std::string asEnemy)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asEnemy);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Enemy)
	{
		Warning("Couldn't find enemy entity '%s'\n",asEnemy.c_str());
		return;
	}
	iGameEnemy *pEnemy = static_cast<iGameEnemy*>(pEntity);
	
	pEnemy->ClearPatrolNodes();
}
SCRIPT_DEFINE_FUNC_1(void,ClearEnemyPatrolNodes,string)

//-----------------------------------------------------------------------

static void __stdcall SetEnemyDeathCallback(std::string asEnemy,std::string asFunction)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asEnemy);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Enemy)
	{
		Warning("Couldn't find enemy entity '%s'\n",asEnemy.c_str());
		return;
	}
	iGameEnemy *pEnemy = static_cast<iGameEnemy*>(pEntity);

	pEnemy->SetOnDeathCallback(asFunction);
}
SCRIPT_DEFINE_FUNC_2(void,SetEnemyDeathCallback,string,string)

//-----------------------------------------------------------------------

static void __stdcall SetEnemyAttackCallback(std::string asEnemy,std::string asFunction)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asEnemy);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Enemy)
	{
		Warning("Couldn't find enemy entity '%s'\n",asEnemy.c_str());
		return;
	}
	iGameEnemy *pEnemy = static_cast<iGameEnemy*>(pEntity);

	pEnemy->SetOnAttackCallback(asFunction);
}
SCRIPT_DEFINE_FUNC_2(void,SetEnemyAttackCallback,string,string)

//-----------------------------------------------------------------------

static float __stdcall GetEnemyHealth(std::string asEnemy)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asEnemy);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Enemy)
	{
		Warning("Couldn't find enemy entity '%s'\n",asEnemy.c_str());
		return 0;
	}
	iGameEnemy *pEnemy = static_cast<iGameEnemy*>(pEntity);

	return pEnemy->GetHealth();
}
SCRIPT_DEFINE_FUNC_1(float,GetEnemyHealth,string)

//-----------------------------------------------------------------------

static void __stdcall SetEnemyUseTriggers(std::string asEnemy, bool abUseTriggers)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asEnemy);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Enemy)
	{
		Warning("Couldn't find enemy entity '%s'\n",asEnemy.c_str());
		return;
	}
	iGameEnemy *pEnemy = static_cast<iGameEnemy*>(pEntity);

	pEnemy->SetUsesTriggers(abUseTriggers);
}
SCRIPT_DEFINE_FUNC_2(void,SetEnemyUseTriggers,string, bool)

//-----------------------------------------------------------------------

// Black Plague's enemy functions

static iGameEnemy* GetScriptEnemy(const std::string &asEnemy)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asEnemy);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Enemy)
	{
		Warning("Couldn't find enemy entity '%s'\n",asEnemy.c_str());
		return nullptr;
	}
	return static_cast<iGameEnemy*>(pEntity);
}

static cGameEnemy_Infected* GetScriptInfected(const std::string &asEnemy)
{
	iGameEnemy *pEnemy = GetScriptEnemy(asEnemy);
	if(pEnemy==nullptr) return nullptr;

	cGameEnemy_Infected *pInfected = dynamic_cast<cGameEnemy_Infected*>(pEnemy);
	if(pInfected==nullptr) Warning("Enemy '%s' is not an Infected\n",asEnemy.c_str());
	return pInfected;
}

/**
 * Plays an animation in place of the enemy's moving ones.
 **/
static void __stdcall PlayEnemyAnim(std::string asEnemy, std::string asAnim, bool abLoop)
{
	iGameEnemy *pEnemy = GetScriptEnemy(asEnemy);
	if(pEnemy==nullptr) return;

	pEnemy->PlayAnim(asAnim,abLoop,0.5f);
}
SCRIPT_DEFINE_FUNC_3(void, PlayEnemyAnim, string, string, bool)

/**
 * Called once, as asFunc(string asEnemy), when the animation being played is over.
 **/
static void __stdcall SetEnemyAnimOverCallback(std::string asEnemy, std::string asFunc)
{
	iGameEnemy *pEnemy = GetScriptEnemy(asEnemy);
	if(pEnemy==nullptr) return;

	pEnemy->SetOnAnimOverCallback(asFunc);
}
SCRIPT_DEFINE_FUNC_2(void, SetEnemyAnimOverCallback, string, string)

/**
 * Called, as asFunc(string asEnemy), each time the enemy starts to hunt the player.
 **/
static void __stdcall SetEnemyHuntCallback(std::string asEnemy, std::string asFunc)
{
	iGameEnemy *pEnemy = GetScriptEnemy(asEnemy);
	if(pEnemy==nullptr) return;

	pEnemy->SetOnHuntCallback(asFunc);
}
SCRIPT_DEFINE_FUNC_2(void, SetEnemyHuntCallback, string, string)

/**
 * Called once, as asFunc(string asEnemy), when the enemy has been at its last patrol node.
 **/
static void __stdcall SetEnemyPatrolOverCallback(std::string asEnemy, std::string asFunc)
{
	iGameEnemy *pEnemy = GetScriptEnemy(asEnemy);
	if(pEnemy==nullptr) return;

	pEnemy->SetOnPatrolOverCallback(asFunc);
}
SCRIPT_DEFINE_FUNC_2(void, SetEnemyPatrolOverCallback, string, string)

/**
 * The patrol is over at its last node instead of starting again.
 **/
static void __stdcall SetEnemyStopAtLastPatrolNode(std::string asEnemy, bool abX)
{
	iGameEnemy *pEnemy = GetScriptEnemy(asEnemy);
	if(pEnemy==nullptr) return;

	pEnemy->SetStopAtLastPatrolNode(abX);
}
SCRIPT_DEFINE_FUNC_2(void, SetEnemyStopAtLastPatrolNode, string, bool)

/**
 * Puts a weapon in an Infected's hand. The rotation is in degrees.
 **/
static void __stdcall SetInfectedWeapon(std::string asEnemy, std::string asFile, float afDamage,
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
SCRIPT_DEFINE_FUNC_9(void, SetInfectedWeapon, string, string, float, float, float, float, float, float, float)

/**
 * Gives an Infected a lit flashlight to carry.
 **/
static void __stdcall AddInfectedFlashlight(std::string asEnemy)
{
	cGameEnemy_Infected *pInfected = GetScriptInfected(asEnemy);
	if(pInfected==nullptr) return;

	pInfected->AddFlashLight(true);
}
SCRIPT_DEFINE_FUNC_1(void, AddInfectedFlashlight, string)

//-----------------------------------------------------------------------

// Black Plague's security cameras

static cGameCamera* GetScriptCamera(const std::string &asName)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Camera)
	{
		Warning("Couldn't find camera entity '%s'\n",asName.c_str());
		return nullptr;
	}
	return static_cast<cGameCamera*>(pEntity);
}

/**
 * How a security camera turns: the angles to each side in degrees, the pause
 * at each end in seconds and the speed. asSpottedCallback(string asCamera) is
 * called when the player is seen, asAlertOverCallback when the alert is over.
 **/
static void __stdcall SetupSecurityCamera(std::string asName, float afMinAngle, float afMaxAngle,
											float afMinPause, float afMaxPause, float afSpeed,
											std::string asSpottedCallback, std::string asAlertOverCallback)
{
	cGameCamera *pCamera = GetScriptCamera(asName);
	if(pCamera==nullptr) return;

	pCamera->Setup(cMath::ToRad(afMinAngle),cMath::ToRad(afMaxAngle),afMinPause,afMaxPause,afSpeed,
					asSpottedCallback,asAlertOverCallback);
}
SCRIPT_DEFINE_FUNC_8(void, SetupSecurityCamera, string, float, float, float, float, float, string, string)

static void __stdcall SetSecurityCameraAlerted(std::string asName, bool abX)
{
	cGameCamera *pCamera = GetScriptCamera(asName);
	if(pCamera==nullptr) return;

	pCamera->SetAlerted(abX);
}
SCRIPT_DEFINE_FUNC_2(void, SetSecurityCameraAlerted, string, bool)

//-----------------------------------------------------------------------

static void __stdcall ShowEnemyPlayer(std::string asEnemy)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asEnemy);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Enemy)
	{
		Warning("Couldn't find enemy entity '%s'\n",asEnemy.c_str());
		return;
	}
	iGameEnemy *pEnemy = static_cast<iGameEnemy*>(pEntity);


	pEnemy->SetLastPlayerPos(	gpInit->mpPlayer->GetCharacterBody()->GetFeetPosition() + 
								cVector3f(0,0.1f,0));
	pEnemy->ChangeState(STATE_HUNT);
}
SCRIPT_DEFINE_FUNC_1(void,ShowEnemyPlayer,string)

//-----------------------------------------------------------------------

static void __stdcall SetDoorLocked(std::string asDoor, bool abLocked)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asDoor);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_SwingDoor)
	{
		Warning("Couldn't find swing door entity '%s'\n",asDoor.c_str());
		return;
	}
	cGameSwingDoor *pDoor = static_cast<cGameSwingDoor*>(pEntity);

	pDoor->SetLocked(abLocked);

}
SCRIPT_DEFINE_FUNC_2(void,SetDoorLocked,string,bool)

//-----------------------------------------------------------------------

static void __stdcall SetupStickArea(	std::string asArea, bool abCanDeatch,
											bool abMoveBody,bool abRotateBody,
											bool abCheckCenterInArea, float afPoseTime,
									 std::string asAttachSound, std::string asDetachSound,
									 std::string asAttachPS, std::string asDetachPS,
									 std::string asAttachFunc, std::string asDetachFunc)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asArea);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_StickArea)
	{
		Warning("Couldn't find stick area '%s'\n",asArea.c_str());
		return;
	}
	cGameStickArea *pArea = static_cast<cGameStickArea*>(pEntity);

    pArea->SetCanDeatch(abCanDeatch);

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
SCRIPT_DEFINE_FUNC_12(void, SetupStickArea, string, bool, bool, bool, bool, float, 
			string, string, string, string, string, string)

//-----------------------------------------------------------------------

static void __stdcall AllowAttachment()
{
	cGameStickArea::mbAllowAttachment = true;
}
SCRIPT_DEFINE_FUNC(void,AllowAttachment)

//-----------------------------------------------------------------------

static void __stdcall SetLampLit(std::string asName,bool abLit, bool abFade)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Lamp)
	{
		Warning("Couldn't find lamp '%s'\n",asName.c_str());
		return;
	}
	cGameLamp *pLamp = static_cast<cGameLamp*>(pEntity);

    pLamp->SetLit(abLit,abFade);
}
SCRIPT_DEFINE_FUNC_3(void,SetLampLit,string,bool,bool)

//-----------------------------------------------------------------------

static void __stdcall SetLampFlicker(std::string asName,bool abFlicker)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Lamp)
	{
		Warning("Couldn't find lamp '%s'\n",asName.c_str());
		return;
	}
	cGameLamp *pLamp = static_cast<cGameLamp*>(pEntity);

	pLamp->SetFlicker(abFlicker);
}
SCRIPT_DEFINE_FUNC_2(void,SetLampFlicker,string,bool)

//-----------------------------------------------------------------------

static void __stdcall SetLampLitChangeCallback(std::string asName,std::string asCallback)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Lamp)
	{
		Warning("Couldn't find lamp '%s'\n",asName.c_str());
		return;
	}
	cGameLamp *pLamp = static_cast<cGameLamp*>(pEntity);

	pLamp->SetLitChangeCallback(asCallback);
}
SCRIPT_DEFINE_FUNC_2(void,SetLampLitChangeCallback,string,string)

//-----------------------------------------------------------------------

static void __stdcall SetupLadder(	std::string asName,
											std::string asAttachSound, 
											std::string asClimbUpSound, 
											std::string asClimbDownSound)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Ladder)
	{
		Warning("Couldn't find ladder '%s'\n",asName.c_str());
		return;
	}
	cGameLadder *pLadder = static_cast<cGameLadder*>(pEntity);

	pLadder->SetAttachSound(asAttachSound);
	pLadder->SetClimbUpSound(asClimbUpSound);
	pLadder->SetClimbDownSound(asClimbDownSound);
}
SCRIPT_DEFINE_FUNC_4(void, SetupLadder, string, string, string, string)

//-----------------------------------------------------------------------

static void __stdcall SetupDamageArea(std::string asName, float afDamage,
													 float afUpdatesPerSec, int alStrength,
													 bool abDisableObjects, bool abDisableEnemies)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_DamageArea)
	{
		Warning("Couldn't find damage area '%s'\n",asName.c_str());
		return;
	}
	cGameDamageArea *pDamage = static_cast<cGameDamageArea*>(pEntity);

	pDamage->SetDamage(afDamage);
	pDamage->SetUpdatesPerSec(afUpdatesPerSec);
	pDamage->SetStrength(alStrength);
	pDamage->SetDisableObjects(abDisableObjects);
	pDamage->SetDisableEnemies(abDisableEnemies);
	
}
SCRIPT_DEFINE_FUNC_6(void, SetupDamageArea, string, float, float, int, bool, bool)

/**
 * Black Plague: with how the player is shown to be hurt and what is heard then.
 * \param asDamageType "BloodSplash", "Ice" or "Poison"
 * \param asDamageSound played each time the player is hurt, "" = none
 */
static void __stdcall SetupDamageAreaType(std::string asName, float afDamage,
													 float afUpdatesPerSec, int alStrength,
													 bool abDisableObjects, bool abDisableEnemies,
													 std::string asDamageType, std::string asDamageSound)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_DamageArea)
	{
		Warning("Couldn't find damage area '%s'\n",asName.c_str());
		return;
	}

	SetupDamageArea(asName, afDamage, afUpdatesPerSec, alStrength, abDisableObjects, abDisableEnemies);

	tString sType = cString::ToLowerCase(asDamageType);
	ePlayerDamageType type = ePlayerDamageType_BloodSplash;
	if(sType == "bloodsplash")	type = ePlayerDamageType_BloodSplash;
	else if(sType == "ice")		type = ePlayerDamageType_Ice;
	else if(sType == "poison")	type = ePlayerDamageType_Poison;
	else Warning("Damage type %s does not exist!\n",asDamageType.c_str());

	cGameDamageArea *pDamage = static_cast<cGameDamageArea*>(pEntity);
	pDamage->SetDamageType(type);
	pDamage->SetDamageSound(asDamageSound);
}
SCRIPT_DEFINE_FUNC_8(void, SetupDamageAreaType, string, float, float, int, bool, bool, string, string)

//-----------------------------------------------------------------------

static void __stdcall SetupForceArea(std::string asName, 
									 float afMaxForce, float afConstant,
									 float afDestSpeed, float afMaxMass,
									 bool abMulWithMass, bool abForceAtPoint,
									 bool abAffectBodies, bool abAffectCharacters)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_ForceArea)
	{
		Warning("Couldn't find force area '%s'\n",asName.c_str());
		return;
	}
	cGameForceArea *pForce = static_cast<cGameForceArea*>(pEntity);

	pForce->SetMaxForce(afMaxForce);
	pForce->SetConstant(afConstant);
	pForce->SetDestSpeed(afDestSpeed);
	pForce->SetMaxMass(afMaxMass);
	pForce->SetMulWithMass(abMulWithMass);
	pForce->SetForceAtPoint(abForceAtPoint);
	pForce->SetAffectBodies(abAffectBodies);
	pForce->SetAffectCharacters(abAffectCharacters);
}
SCRIPT_DEFINE_FUNC_9(void, SetupForceArea, string, float, float, float, float, bool, bool, bool, bool)

//-----------------------------------------------------------------------


static void __stdcall SetupLiquidArea(std::string asName,
								  float afDensity, float afLinearViscosity,
								  float afAngularViscosity,
								  std::string asPhysicsMaterial,
								  float fR, float fG, float fB,
								  bool abHasWaves)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_LiquidArea)
	{
		Warning("Couldn't find liquid area '%s'\n",asName.c_str());
		return;
	}
	cGameLiquidArea *pLiquid = static_cast<cGameLiquidArea*>(pEntity);

	pLiquid->SetDensity(afDensity);
	pLiquid->SetLinearViscosity(afLinearViscosity);
	pLiquid->SetAngularViscosity(afAngularViscosity);

	pLiquid->SetPhysicsMaterial(asPhysicsMaterial);

	pLiquid->SetColor(cColor(fR,fG,fB,1));

	pLiquid->SetHasWaves(abHasWaves);
}
SCRIPT_DEFINE_FUNC_9(void, SetupLiquidArea, string, float, float, float, string, float, float, float, bool)

//-----------------------------------------------------------------------

static void __stdcall SetupSaveArea(std::string asName,
											std::string asMessageCat,std::string asMessageEntry, 
											std::string asSound)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_SaveArea)
	{
		Warning("Couldn't find save area '%s'\n",asName.c_str());
		return;
	}
	cGameSaveArea *pSave = static_cast<cGameSaveArea*>(pEntity);

	pSave->SetMessageCat(asMessageCat);
	pSave->SetMessageEntry(asMessageEntry);
	pSave->SetSound(asSound);
}
SCRIPT_DEFINE_FUNC_4(void, SetupSaveArea, string, string, string, string)

//-----------------------------------------------------------------------


///////// GAME ENTITY CALLBACKS //////////////////////////////////

//-----------------------------------------------------------------------

static eGameCollideScriptType GetGameCollideScriptType(const tString &asType)
{
	tString sName = cString::ToLowerCase(asType);

    if(sName == "enter") return eGameCollideScriptType_Enter;
	if(sName == "leave") return eGameCollideScriptType_Leave;
	if(sName == "during") return eGameCollideScriptType_During;

	Warning("Collide Type %s doesn't exist!\n",asType.c_str());

	return eGameCollideScriptType_LastEnum;
}

//////////////////////////////

static void __stdcall AddEntityCollideCallback(std::string asType,
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
		iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asDestName);
		if(pEntity==nullptr)
		{
			Warning("Couldn't find entity '%s'\n",asDestName.c_str());
			return;
		}

		eGameCollideScriptType type = GetGameCollideScriptType(asType);

		if(type != eGameCollideScriptType_LastEnum)
			pEntity->AddCollideScript(type,asFuncName,asEntityName);
	}
	
}
SCRIPT_DEFINE_FUNC_4(void, AddEntityCollideCallback, string, string, string, string)

//////////////////////////////

static void __stdcall RemoveEntityCollideCallback(std::string asType,
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
		iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asDestName);
		if(pEntity==nullptr)
		{
			Warning("Couldn't find entity '%s'\n",asDestName.c_str());
			return;
		}

		eGameCollideScriptType type = GetGameCollideScriptType(asType);

		if(type != eGameCollideScriptType_LastEnum)
		{
			pEntity->RemoveCollideScript(type,asEntityName);
		}
	}
}
SCRIPT_DEFINE_FUNC_3(void, RemoveEntityCollideCallback, string, string, string)

//-----------------------------------------------------------------------

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

/////////////////////

static void __stdcall AddEntityCallback(std::string asType,
										std::string asDestName,
										std::string asFuncName)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asDestName);
	if(pEntity==nullptr)
	{
		Warning("Couldn't find entity '%s'\n",asDestName.c_str());
		return;
	}

	eGameEntityScriptType type = GetGameScriptType(asType);

	if(type!= eGameEntityScriptType_LastEnum)
	{
		pEntity->AddScript(type, asFuncName);
	}
}
SCRIPT_DEFINE_FUNC_3(void, AddEntityCallback, string, string, string)

/////////////////////

static void __stdcall RemoveEntityCallback(std::string asType,
										   std::string asDestName)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asDestName);
	if(pEntity==nullptr)
	{
		Warning("Couldn't find entity '%s'\n",asDestName.c_str());
		return;
	}

	eGameEntityScriptType type = GetGameScriptType(asType);

    if(type!= eGameEntityScriptType_LastEnum)
	{
		pEntity->RemoveScript(type);
	}
}
SCRIPT_DEFINE_FUNC_2(void, RemoveEntityCallback, string, string)

//-----------------------------------------------------------------------

///////// GAME SOUND //////////////////////////////////

//-----------------------------------------------------------------------

static void __stdcall CreateSoundEntityAt(std::string asType,std::string asDestName, 
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
	
	////////////////////////
	// Body
	if(lType == 1)
	{
		iPhysicsBody *pBody = pPhysicsWorld->GetBody(asDestName);
		if(pBody==nullptr)	{
			Warning("Body '%s' coudln't be found!\n",asDestName.c_str()); return;
		}

		vPos = pBody->GetLocalPosition();
	}
	////////////////////////
	// Joint
	else if(lType == 2)
	{
		iPhysicsJoint *pJoint = pPhysicsWorld->GetJoint(asDestName);
		if(pJoint==nullptr)	{
			Warning("Body '%s' coudln't be found!\n",asDestName.c_str()); return;
		}

		vPos = pJoint->GetPivotPoint();
	}
	////////////////////////
	// Entity
	{
		iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asDestName);
		if(pEntity==nullptr)
		{
			Warning("Couldn't find entity '%s'\n",asDestName.c_str());
			return;
		}
		
		if(pEntity->GetMeshEntity())
			vPos = pEntity->GetMeshEntity()->GetWorldPosition();
		else
			vPos = pEntity->GetBody(0)->GetLocalPosition();
	}
	
	////////////////////////
	// Create sound.
	cSoundEntity *pSound = pWorld->CreateSoundEntity(asSoundName,asSoundFile,true);
	if(pSound)
	{
		pSound->SetPosition(vPos);
	}
}
SCRIPT_DEFINE_FUNC_4(void, CreateSoundEntityAt, string, string, string, string)

//-----------------------------------------------------------------------


//////////////////////////////////////////////////////////////////////////
// PUBLIC FUNCTIONS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////
/////// LEVERS AND WHEELS (BLACK PLAGUE) ///////////////////////////////
/////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

static cGameLever* GetLever(const std::string &asName)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Lever)
	{
		Warning("Couldn't find lever '%s'\n",asName.c_str());
		return nullptr;
	}
	return static_cast<cGameLever*>(pEntity);
}

static cGameWheel* GetWheel(const std::string &asName)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Wheel)
	{
		Warning("Couldn't find wheel '%s'\n",asName.c_str());
		return nullptr;
	}
	return static_cast<cGameWheel*>(pEntity);
}

//-----------------------------------------------------------------------

/**
 * The function is called as Func(string asName, int alAction, int alState):
 * the action 0 = enter, 1 = leave, 2 = during; the state -1 = min, 0 = middle, 1 = max.
 * \param asType "Enter", "Leave" or "During"
 */
static void __stdcall SetLeverCallback(std::string asName, std::string asType, std::string asFunc)
{
	cGameLever *pLever = GetLever(asName);
	if(pLever==nullptr) return;

	tString sType = cString::ToLowerCase(asType);

	if(sType == "enter")		pLever->SetCallback(eGameLeverAction_Enter, asFunc);
	else if(sType == "leave")	pLever->SetCallback(eGameLeverAction_Leave, asFunc);
	else if(sType == "during")	pLever->SetCallback(eGameLeverAction_During, asFunc);
}
SCRIPT_DEFINE_FUNC_3(void, SetLeverCallback, string, string, string)

//-----------------------------------------------------------------------

/**
 * \param alState -1 = stuck at min, 1 = stuck at max, 0 = not stuck
 */
static void __stdcall SetLeverStuck(std::string asName, int alState)
{
	cGameLever *pLever = GetLever(asName);
	if(pLever==nullptr) return;

	if(alState == 0)		pLever->SetIsStuck(false);
	else if(alState == 1)	pLever->SetStuck(eGameLeverState_Max);
	else if(alState == -1)	pLever->SetStuck(eGameLeverState_Min);
}
SCRIPT_DEFINE_FUNC_2(void, SetLeverStuck, string, int)

//-----------------------------------------------------------------------

/**
 * \return -1 = min, 0 = middle, 1 = max
 */
static int __stdcall GetLeverState(std::string asName)
{
	cGameLever *pLever = GetLever(asName);
	if(pLever==nullptr) return 0;

	switch(pLever->GetState())
	{
	case eGameLeverState_Min: return -1;
	case eGameLeverState_Max: return 1;
	default: return 0;
	}
}
SCRIPT_DEFINE_FUNC_1(int, GetLeverState, string)

//-----------------------------------------------------------------------

static void __stdcall SetWheelCallback(std::string asName, std::string asType, std::string asFunc)
{
	cGameWheel *pWheel = GetWheel(asName);
	if(pWheel==nullptr) return;

	tString sType = cString::ToLowerCase(asType);

	if(sType == "enter")		pWheel->SetCallback(eGameWheelAction_Enter, asFunc);
	else if(sType == "leave")	pWheel->SetCallback(eGameWheelAction_Leave, asFunc);
	else if(sType == "during")	pWheel->SetCallback(eGameWheelAction_During, asFunc);
}
SCRIPT_DEFINE_FUNC_3(void, SetWheelCallback, string, string, string)

//-----------------------------------------------------------------------

static void __stdcall SetWheelStuck(std::string asName, int alState)
{
	cGameWheel *pWheel = GetWheel(asName);
	if(pWheel==nullptr) return;

	if(alState == 0)		pWheel->SetIsStuck(false);
	else if(alState == 1)	pWheel->SetStuck(eGameWheelState_Max);
	else if(alState == -1)	pWheel->SetStuck(eGameWheelState_Min);
}
SCRIPT_DEFINE_FUNC_2(void, SetWheelStuck, string, int)

//-----------------------------------------------------------------------

static int __stdcall GetWheelState(std::string asName)
{
	cGameWheel *pWheel = GetWheel(asName);
	if(pWheel==nullptr) return 0;

	switch(pWheel->GetState())
	{
	case eGameWheelState_Min: return -1;
	case eGameWheelState_Max: return 1;
	default: return 0;
	}
}
SCRIPT_DEFINE_FUNC_1(int, GetWheelState, string)

//-----------------------------------------------------------------------

/**
 * The angle the wheel has been turned in all, in degrees
 */
static float __stdcall GetWheelAngle(std::string asName)
{
	cGameWheel *pWheel = GetWheel(asName);
	if(pWheel==nullptr) return 0;

	return cMath::ToDeg(pWheel->GetAngle());
}
SCRIPT_DEFINE_FUNC_1(float, GetWheelAngle, string)

static float __stdcall GetWheelSpeed(std::string asName)
{
	cGameWheel *pWheel = GetWheel(asName);
	if(pWheel==nullptr) return 0;

	return pWheel->GetSpeed();
}
SCRIPT_DEFINE_FUNC_1(float, GetWheelSpeed, string)

//-----------------------------------------------------------------------

/**
 * The limits of a joint follow the wheel: afMin at the wheel's least angle,
 * afMax at its greatest, afRange between the two limits.
 */
static void __stdcall ConnectWheelToJoint(std::string asName, std::string asJoint,
											float afMin, float afMax, float afRange)
{
	cGameWheel *pWheel = GetWheel(asName);
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
SCRIPT_DEFINE_FUNC_5(void, ConnectWheelToJoint, string, string, float, float, float)

/**
 * The same for the value a controller on one of the entity's joints aims at
 */
static void __stdcall ConnectWheelToController(std::string asName, std::string asEntity, std::string asController,
											float afMin, float afMax)
{
	cGameWheel *pWheel = GetWheel(asName);
	if(pWheel==nullptr) return;

	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asEntity);
	if(pEntity==nullptr)
	{
		Warning("Couldn't find controller entity '%s'\n",asEntity.c_str());
		return;
	}

	pWheel->ConnectToController(pEntity, asController, afMin, afMax);
}
SCRIPT_DEFINE_FUNC_5(void, ConnectWheelToController, string, string, string, float, float)

//-----------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////
/////// MOVING THINGS TO AREAS (BLACK PLAGUE) //////////////////////////
/////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

static cGameArea* GetArea(const std::string &asName)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_Area)
	{
		Warning("Couldn't find area entity '%s'\n",asName.c_str());
		return nullptr;
	}
	return static_cast<cGameArea*>(pEntity);
}

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

//-----------------------------------------------------------------------

/**
 * Moves an entity in a straight line to the centre of an area.
 * \param afMaxSpeed in m/s
 * \param afAcceleration in m/s^2
 * \param afSlowDownDist the distance from the area inside which it slows down
 * \param asCallback called on arrival: Func(string asEntity). "" = none.
 */
static void __stdcall MoveEntityToArea(std::string asName, std::string asArea, float afMaxSpeed,
										float afAcceleration, float afSlowDownDist, std::string asCallback)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr)
	{
		Warning("Couldn't find game entity '%s'\n",asName.c_str());
		return;
	}

	cGameArea *pArea = GetArea(asArea);
	if(pArea==nullptr) return;

	pEntity->MoveToPosition(pArea->GetBody(0)->GetWorldPosition(), afMaxSpeed, afAcceleration,
							afSlowDownDist, asCallback);
}
SCRIPT_DEFINE_FUNC_6(void, MoveEntityToArea, string, string, float, float, float, string)

//-----------------------------------------------------------------------

/**
 * Puts a body at the centre of an area.
 * \param abResetVelocity the body is left without speed
 * \param abSetRotation the body gets the rotation given (radians), else it keeps its own
 */
static void __stdcall TeleportBodyToArea(std::string asBody, std::string asArea, bool abResetVelocity,
										bool abSetRotation, float afRotX, float afRotY, float afRotZ)
{
	iPhysicsBody *pBody = GetBody(asBody);
	if(pBody==nullptr) return;

	cGameArea *pArea = GetArea(asArea);
	if(pArea==nullptr) return;

	TeleportBody(pBody, pArea, abResetVelocity, abSetRotation, cVector3f(afRotX, afRotY, afRotZ));
}
SCRIPT_DEFINE_FUNC_7(void, TeleportBodyToArea, string, string, bool, bool, float, float, float)

/**
 * The same for the first body of an entity
 */
static void __stdcall TeleportEntityToArea(std::string asName, std::string asArea, bool abResetVelocity,
										bool abSetRotation, float afRotX, float afRotY, float afRotZ)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr)
	{
		Warning("Couldn't find game entity '%s'\n",asName.c_str());
		return;
	}

	cGameArea *pArea = GetArea(asArea);
	if(pArea==nullptr) return;

	if(pEntity->GetBodyNum()==0)
	{
		Warning("Entity %s has no bodies! Could not be teleported!\n",asName.c_str());
		return;
	}

	TeleportBody(pEntity->GetBody(0), pArea, abResetVelocity, abSetRotation, cVector3f(afRotX, afRotY, afRotZ));
}
SCRIPT_DEFINE_FUNC_7(void, TeleportEntityToArea, string, string, bool, bool, float, float, float)

//-----------------------------------------------------------------------

/**
 * The player stands at the centre of an area
 */
static void __stdcall TeleportPlayerToArea(std::string asArea)
{
	cGameArea *pArea = GetArea(asArea);
	if(pArea==nullptr) return;

	gpInit->mpPlayer->GetCharacterBody()->SetFeetPosition(pArea->GetBody(0)->GetWorldPosition(), false);
}
SCRIPT_DEFINE_FUNC_1(void, TeleportPlayerToArea, string)

//-----------------------------------------------------------------------

/**
 * Requiem: an entity is made from a file, placed and turned as the area is
 */
static void __stdcall CreateEntityAtArea(std::string asName, std::string asFile, std::string asArea)
{
	cWorld3D *pWorld = gpInit->mpGame->GetScene()->GetWorld3D();

	cGameArea *pArea = GetArea(asArea);
	if(pArea==nullptr) return;

	pWorld->CreateEntity(asName, pArea->GetBody(0)->GetWorldMatrix(), asFile, true);
}
SCRIPT_DEFINE_FUNC_3(void, CreateEntityAtArea, string, string, string)

//-----------------------------------------------------------------------

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
static bool __stdcall CheckBodyCollision(std::string asBodyA, std::string asBodyB)
{
	iPhysicsWorld *pPhysicsWorld = gpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld();

	iPhysicsBody *pBodyA = pPhysicsWorld->GetBody(asBodyA);
	iPhysicsBody *pBodyB = pPhysicsWorld->GetBody(asBodyB);
	if(pBodyA==nullptr || pBodyB==nullptr) return false;

	return BodiesCollide(pBodyA, pBodyB);
}
SCRIPT_DEFINE_FUNC_2(bool, CheckBodyCollision, string, string)

/**
 * \return true if any body of an entity touches a body
 */
static bool __stdcall CheckEntityBodyCollision(std::string asName, std::string asBody)
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
SCRIPT_DEFINE_FUNC_2(bool, CheckEntityBodyCollision, string, string)

/**
 * \return true if any body of one entity touches a body of the other.
 * (The game's own binary pairs the bodies by their number only, which is the
 * same thing for the entities of one body that its scripts ask about.)
 */
static bool __stdcall CheckGameEntityCollision(std::string asNameA, std::string asNameB)
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
SCRIPT_DEFINE_FUNC_2(bool, CheckGameEntityCollision, string, string)

//-----------------------------------------------------------------------

static cGameStickArea* GetStickArea(const std::string &asName)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asName);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_StickArea)
	{
		Warning("Couldn't find stick area '%s'\n",asName.c_str());
		return nullptr;
	}
	return static_cast<cGameStickArea*>(pEntity);
}

/**
 * A stick area takes hold of a body, wherever the body is
 */
static void __stdcall AttachBodyToStickArea(std::string asBody, std::string asArea)
{
	cGameStickArea *pStickArea = GetStickArea(asArea);
	if(pStickArea==nullptr) return;

	iPhysicsBody *pBody = GetBody(asBody);
	if(pBody==nullptr) return;

	pStickArea->AttachBody(pBody);
}
SCRIPT_DEFINE_FUNC_2(void, AttachBodyToStickArea, string, string)

static void __stdcall DetachBodyFromStickArea(std::string asArea)
{
	cGameStickArea *pStickArea = GetStickArea(asArea);
	if(pStickArea==nullptr) return;

	pStickArea->DetachBody();
}
SCRIPT_DEFINE_FUNC_1(void, DetachBodyFromStickArea, string)

//-----------------------------------------------------------------------

/////////////////////////////////////////////////////////////////////////
/////// DOORS, PLAYER AND NOTEBOOK (BLACK PLAGUE) //////////////////////
/////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

/**
 * What is said and heard when the player tries a locked swing door.
 * \param abX not used by the game
 */
static void __stdcall SetDoorLockedProperties(std::string asDoor, std::string asDescCat, std::string asDescEntry,
												std::string asSound, bool abX)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asDoor);
	if(pEntity==nullptr || pEntity->GetType() != eGameEntityType_SwingDoor)
	{
		Warning("Couldn't find swing door entity '%s'\n",asDoor.c_str());
		return;
	}
	cGameSwingDoor *pDoor = static_cast<cGameSwingDoor*>(pEntity);

	pDoor->SetLockedDesc(gpInit->mpGame->GetResources()->Translate(asDescCat, asDescEntry));
	pDoor->SetLockedSound(asSound);
}
SCRIPT_DEFINE_FUNC_5(void, SetDoorLockedProperties, string, string, string, string, bool)

//-----------------------------------------------------------------------

static void __stdcall SetNoFallDamage(bool abX)
{
	gpInit->mpPlayer->SetNoFallDamage(abX);
}
SCRIPT_DEFINE_FUNC_1(void, SetNoFallDamage, bool)

static void __stdcall TurnOffGlowStick()
{
	if(gpInit->mpPlayer->GetGlowStick()->IsActive())
		gpInit->mpPlayer->GetGlowStick()->SetActive(false);
}
SCRIPT_DEFINE_FUNC(void, TurnOffGlowStick)

/**
 * Requiem: a lit flare is put out
 */
static void __stdcall DropFlare()
{
	if(gpInit->mpPlayer->GetFlare()->IsActive())
		gpInit->mpPlayer->GetFlare()->SetActive(false);
}
SCRIPT_DEFINE_FUNC(void, DropFlare)

/**
 * The function is called, once, when the flashlight's power falls below the value: Func()
 */
static void __stdcall SetLowPowerCallback(std::string asFunc, float afVal)
{
	gpInit->mpPlayer->SetLowPowerCallback(asFunc, afVal);
}
SCRIPT_DEFINE_FUNC_2(void, SetLowPowerCallback, string, float)

/**
 * A radio message heard from an entity (an area as a rule): at full volume inside
 * afMinDist, fading to nothing at afMaxDist, where its text is not shown either.
 */
static void __stdcall AddRadioMessageAtPos(std::string asTransCat, std::string asTransName, std::string asSound,
											std::string asEntity, float afMinDist, float afMaxDist)
{
	iGameEntity *pEntity = gpInit->mpMapHandler->GetGameEntity(asEntity);
	if(pEntity==nullptr || pEntity->GetBodyNum()==0) return;

	gpInit->mpRadioHandler->Add(gpInit->mpGame->GetResources()->Translate(asTransCat, asTransName),
								asSound, pEntity, afMinDist, afMaxDist);
}
SCRIPT_DEFINE_FUNC_6(void, AddRadioMessageAtPos, string, string, string, string, float, float)

/**
 * The waiting radio messages are dropped and the one playing fades out.
 * \param afFadeSpeed volume lost per second
 */
static void __stdcall StopAllRadioMessages(float afFadeSpeed)
{
	gpInit->mpRadioHandler->StopAll(afFadeSpeed);
}
SCRIPT_DEFINE_FUNC_1(void, StopAllRadioMessages, float)

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// COMPUTER TERMINALS (Black Plague)
//
// A screen from interfaces/*.gui, see ui/GameGui. The widget functions act
// on the screen that is open and are called from its own script.
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

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

//-----------------------------------------------------------------------

static void __stdcall OpenScreenGui(std::string asFile)
{
	gpInit->mpGameGuiHandler->OpenScreenGui(asFile);
}
SCRIPT_DEFINE_FUNC_1(void, OpenScreenGui, string)

/**
 * The screen is taken down at the next update, so its own script may call this.
 */
static void __stdcall CloseScreenGui()
{
	gpInit->mpGameGuiHandler->CloseScreenGui(true);
}
SCRIPT_DEFINE_FUNC(void, CloseScreenGui)

//-----------------------------------------------------------------------

/**
 * Properties: Enabled, Visible
 */
static void __stdcall SetWidgetBool(std::string asName, std::string asProp, bool abVal)
{
	iWidget *pWidget = GetScreenWidget("SetWidgetBool", asName);
	if(pWidget == nullptr) return;

	tString sProp = cString::ToLowerCase(asProp);

	if(sProp == "enabled")		pWidget->SetEnabled(abVal);
	else if(sProp == "visible")	pWidget->SetVisible(abVal);
	else Warning("Bool property '%s' does not exist", asProp.c_str());
}
SCRIPT_DEFINE_FUNC_3(void, SetWidgetBool, string, string, bool)

static bool __stdcall GetWidgetBool(std::string asName, std::string asProp)
{
	iWidget *pWidget = GetScreenWidget("GetWidgetBool", asName);
	if(pWidget == nullptr) return false;

	tString sProp = cString::ToLowerCase(asProp);

	if(sProp == "enabled") return pWidget->IsEnabled();
	if(sProp == "visible") return pWidget->IsVisible();

	Warning("Bool property '%s' does not exist", asProp.c_str());
	return false;
}
SCRIPT_DEFINE_FUNC_2(bool, GetWidgetBool, string, string)

//-----------------------------------------------------------------------

/**
 * Properties: SelectedItem (list and combo box), MaxTextLength (text box, label),
 * MaxShownItems (combo box), MaxValue, Value, BarValueSize, ButtonValueAdd (slider)
 */
static void __stdcall SetWidgetInt(std::string asName, std::string asProp, int alVal)
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
SCRIPT_DEFINE_FUNC_3(void, SetWidgetInt, string, string, int)

static int __stdcall GetWidgetInt(std::string asName, std::string asProp)
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
SCRIPT_DEFINE_FUNC_2(int, GetWidgetInt, string, string)

//-----------------------------------------------------------------------

/**
 * Properties: BackgroundZ (frame, list box)
 */
static void __stdcall SetWidgetFloat(std::string asName, std::string asProp, float afVal)
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
SCRIPT_DEFINE_FUNC_3(void, SetWidgetFloat, string, string, float)

/**
 * Properties: BackgroundColor (frame)
 */
static void __stdcall SetWidgetColor(std::string asName, std::string asProp, float afR, float afG, float afB, float afA)
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
SCRIPT_DEFINE_FUNC_6(void, SetWidgetColor, string, string, float, float, float, float)

/**
 * Properties: Size
 */
static void __stdcall SetWidgetVec2(std::string asName, std::string asProp, float afX, float afY)
{
	iWidget *pWidget = GetScreenWidget("SetWidgetVec2", asName);
	if(pWidget == nullptr) return;

	tString sProp = cString::ToLowerCase(asProp);

	if(sProp == "size")	pWidget->SetSize(cVector2f(afX, afY));
	else Warning("Vec2 property '%s' does not exist", asProp.c_str());
}
SCRIPT_DEFINE_FUNC_4(void, SetWidgetVec2, string, string, float, float)

/**
 * Properties: Position
 */
static void __stdcall SetWidgetVec3(std::string asName, std::string asProp, float afX, float afY, float afZ)
{
	iWidget *pWidget = GetScreenWidget("SetWidgetVec3", asName);
	if(pWidget == nullptr) return;

	tString sProp = cString::ToLowerCase(asProp);

	if(sProp == "position")	pWidget->SetPosition(cVector3f(afX, afY, afZ));
	else Warning("Vec3 property '%s' does not exist", asProp.c_str());
}
SCRIPT_DEFINE_FUNC_5(void, SetWidgetVec3, string, string, float, float, float)

//-----------------------------------------------------------------------

/**
 * Properties: Text, Font (a font of the screen's file), Image (image and button; an image of the screen's file)
 */
static void __stdcall SetWidgetString(std::string asName, std::string asProp, std::string asVal)
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
SCRIPT_DEFINE_FUNC_3(void, SetWidgetString, string, string, string)

/**
 * Properties: Text
 */
static void __stdcall SetWidgetStringTrans(std::string asName, std::string asProp, std::string asTransCat, std::string asTransName)
{
	iWidget *pWidget = GetScreenWidget("SetWidgetStringTrans", asName);
	if(pWidget == nullptr) return;

	tString sProp = cString::ToLowerCase(asProp);

	if(sProp == "text")
		pWidget->SetText(gpInit->mpGame->GetResources()->Translate(asTransCat, asTransName));
	else
		Warning("StringTrans property '%s' does not exist", asProp.c_str());
}
SCRIPT_DEFINE_FUNC_4(void, SetWidgetStringTrans, string, string, string, string)

/**
 * Properties: Text
 */
static std::string __stdcall GetWidgetString(std::string asName, std::string asProp)
{
	iWidget *pWidget = GetScreenWidget("GetWidgetString", asName);
	if(pWidget == nullptr) return "";

	tString sProp = cString::ToLowerCase(asProp);

	if(sProp == "text") return cString::To8Char(pWidget->GetText());

	Warning("String property '%s' does not exist", asProp.c_str());
	return "";
}
SCRIPT_DEFINE_FUNC_2(string, GetWidgetString, string, string)

//-----------------------------------------------------------------------

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
static void __stdcall AddWidgetItem(std::string asName, std::string asItem)
{
	iWidgetItemContainer *pContainer = GetScreenItemContainer("AddWidgetItem", asName);
	if(pContainer) pContainer->AddItem(cString::To16Char(asItem));
}
SCRIPT_DEFINE_FUNC_2(void, AddWidgetItem, string, string)

static void __stdcall AddWidgetItemTrans(std::string asName, std::string asTransCat, std::string asTransName)
{
	iWidgetItemContainer *pContainer = GetScreenItemContainer("AddWidgetItemTrans", asName);
	if(pContainer) pContainer->AddItem(gpInit->mpGame->GetResources()->Translate(asTransCat, asTransName));
}
SCRIPT_DEFINE_FUNC_3(void, AddWidgetItemTrans, string, string, string)

static void __stdcall RemoveWidgetItem(std::string asName, int alIdx)
{
	iWidgetItemContainer *pContainer = GetScreenItemContainer("RemoveWidgetItem", asName);
	if(pContainer == nullptr) return;

	// The engine's container does not check the number
	if(alIdx < 0 || alIdx >= pContainer->GetItemNum()) return;

	pContainer->RemoveItem(alIdx);
}
SCRIPT_DEFINE_FUNC_2(void, RemoveWidgetItem, string, int)

//-----------------------------------------------------------------------

/**
 * Only this widget and those in it take the mouse; "" gives it back to all.
 */
static void __stdcall SetAttentionWidget(std::string asName)
{
	cGameGui *pGui = GetScreenGui("SetAttentionWidget");
	if(pGui == nullptr) return;

	if(asName == "")
		pGui->GetSet()->SetAttentionWidget(nullptr);
	else
		pGui->GetSet()->SetAttentionWidget(pGui->GetSet()->GetWidgetFromName(asName));
}
SCRIPT_DEFINE_FUNC_1(void, SetAttentionWidget, string)

/**
 * The function is called when the widget gets the message: Func(string asName, string asMessage, int alVal)
 * Messages: MouseDown, MouseUp, MouseDoubleClick, MouseEnter, MouseLeave, ButtonPressed,
 * TextChange, CheckChange, KeyPress, SliderMove, SelectionChange
 */
static void __stdcall SetWidgetCallback(std::string asName, std::string asMessage, std::string asFunc)
{
	cGameGui *pGui = GetScreenGui("SetWidgetCallback");
	if(pGui == nullptr) return;

	pGui->AddCallback(asName, cGameGui::StringToGuiMessage(asMessage), asFunc);
}
SCRIPT_DEFINE_FUNC_3(void, SetWidgetCallback, string, string, string)

//-----------------------------------------------------------------------

/**
 * A message box with one or two buttons ("" for none). The function is called with the button pressed: Func(int alButton)
 */
static void __stdcall CreateGuiPopUp(std::string asLabel, std::string asText, std::string asButton1,
									std::string asButton2, std::string asCallback)
{
	cGameGui *pGui = GetScreenGui("CreateGuiPopUp");
	if(pGui == nullptr) return;

	pGui->CreatePopUp(cString::To16Char(asLabel), cString::To16Char(asText),
					cString::To16Char(asButton1), cString::To16Char(asButton2), asCallback);
}
SCRIPT_DEFINE_FUNC_5(void, CreateGuiPopUp, string, string, string, string, string)

static void __stdcall CreateGuiPopUpTrans(std::string asLabelCat, std::string asLabelName,
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
SCRIPT_DEFINE_FUNC_9(void, CreateGuiPopUpTrans, string, string, string, string, string, string, string, string, string)

/**
 * A text to read on the screen. It is put in the notebook, the first time it is opened.
 * The function is called when the text is closed: Func()
 */
static void __stdcall CreateGuiTextView(std::string asLabelCat, std::string asLabelName,
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
SCRIPT_DEFINE_FUNC_5(void, CreateGuiTextView, string, string, string, string, string)

//-----------------------------------------------------------------------

/**
 * As SetRadioOnEndCallback, for a function in the script of the open computer screen.
 */
static void __stdcall SetRadioOnEndCallbackGui(std::string asFunc)
{
	gpInit->mpRadioHandler->SetOnEndCallback(asFunc, true);
}
SCRIPT_DEFINE_FUNC_1(void, SetRadioOnEndCallbackGui, string)

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// EFFECTS OF BLACK PLAGUE
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

/**
 * The view is laid over itself, a little larger, swelling and shrinking
 * afAlpha: how strong; afSpeed: how fast it comes, per second (0 = at once)
 */
static void __stdcall StartDreamFilter(float afAlpha, float afSpeed)
{
	gpInit->mpEffectHandler->GetDreamFilter()->Start(afAlpha, afSpeed);
}
SCRIPT_DEFINE_FUNC_2(void, StartDreamFilter, float, float)

static void __stdcall StopDreamFilter(float afSpeed)
{
	gpInit->mpEffectHandler->GetDreamFilter()->Stop(afSpeed);
}
SCRIPT_DEFINE_FUNC_1(void, StopDreamFilter, float)

/**
 * A picture fades in over the view, stays and fades out
 * abStartFlash, abEndFlash: with a white flash as it comes and before it goes
 */
static void __stdcall StartImageFlash(float afFadeIn, float afStay, float afFadeOut, std::string asFile,
									bool abStartFlash, bool abEndFlash)
{
	gpInit->mpEffectHandler->GetImageFlash()->Start(afFadeIn, afStay, afFadeOut, asFile, abStartFlash, abEndFlash);
}
SCRIPT_DEFINE_FUNC_6(void, StartImageFlash, float, float, float, string, bool, bool)

/**
 * Messages in black, for a bright view
 */
static void __stdcall SetMessageBlackText(bool abX)
{
	gpInit->mpGameMessageHandler->SetBlackText(abX);
}
SCRIPT_DEFINE_FUNC_1(void, SetMessageBlackText, bool)

/**
 * Subtitles on black strips, for a bright view
 */
static void __stdcall SetSubTitleBlackText(bool abX)
{
	gpInit->mpEffectHandler->GetSubTitle()->SetBlackText(abX);
}
SCRIPT_DEFINE_FUNC_1(void, SetSubTitleBlackText, bool)

/**
 * True while the gas mask is on the player's face
 */
static bool __stdcall GetPlayerHasGasMask()
{
	return gpInit->mpEffectHandler->GetGasMask()->IsActive();
}
SCRIPT_DEFINE_FUNC(bool, GetPlayerHasGasMask)

/**
 * Requiem: the effect of a save at the area (the look, the flash), without saving
 */
static void __stdcall SaveEffectOnArea(std::string asArea)
{
	cAreaEntity *pArea = gpInit->mpGame->GetScene()->GetWorld3D()->GetAreaEntity(asArea);
	if(pArea == nullptr)
	{
		Error("Could not find area '%s'\n", asArea.c_str());
		return;
	}

	gpInit->mpEffectHandler->GetSaveEffect()->NormalSave(pArea->m_mtxTransform.GetTranslation(), nullptr, false);
}
SCRIPT_DEFINE_FUNC_1(void, SaveEffectOnArea, string)

/**
 * Black Plague's endings: mails typed on a black screen. 1 goes back to the
 * map afterwards, 2 goes on to the credits.
 */
static void __stdcall StartEnding1()
{
	gpInit->mpEnding->SetActive(true, 0);
}
SCRIPT_DEFINE_FUNC(void, StartEnding1)

static void __stdcall StartEnding2()
{
	gpInit->mpEnding->SetActive(true, 3);
}
SCRIPT_DEFINE_FUNC(void, StartEnding2)

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// SOUND ENVIRONMENTS (Black Plague)
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

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
static void __stdcall LinkSoundEnvToSector(std::string asSector, std::string asSoundEnv, float afFadeTime, int alPrio)
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
SCRIPT_DEFINE_FUNC_4(void, LinkSoundEnvToSector, string, string, float, int)

static void __stdcall LinkSoundEnvToAllSectors(std::string asSoundEnv, float afFadeTime)
{
	iSoundEnvironment *pEnv = GetScriptSoundEnv(asSoundEnv);
	if(pEnv == nullptr) return;

	tSectorMap *pSectors = gpInit->mpGame->GetScene()->GetWorld3D()->GetPortalContainer()->GetSectorMap();
	for(tSectorMapIt it = pSectors->begin(); it != pSectors->end(); ++it)
	{
		gpInit->mpSoundEnvHandler->AddSectorSoundEnv(it->second, pEnv, afFadeTime, 0);
	}
}
SCRIPT_DEFINE_FUNC_2(void, LinkSoundEnvToAllSectors, string, float)

/**
 * The reverb heard outside the sectors that have one
 */
static void __stdcall SetDefaultSoundEnv(std::string asSoundEnv, float afFadeTime)
{
	iSoundEnvironment *pEnv = GetScriptSoundEnv(asSoundEnv);
	if(pEnv == nullptr) return;

	gpInit->mpSoundEnvHandler->SetDefaultSoundEnv(pEnv, afFadeTime);
}
SCRIPT_DEFINE_FUNC_2(void, SetDefaultSoundEnv, string, float)

//-----------------------------------------------------------------------

/**
 * The function is called, once, when the notebook is closed: Func()
 */
static void __stdcall SetNotebookCloseCallback(std::string asFunc)
{
	gpInit->mpNotebook->SetCloseCallback(asFunc);
}
SCRIPT_DEFINE_FUNC_1(void, SetNotebookCloseCallback, string)

//-----------------------------------------------------------------------

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

//-----------------------------------------------------------------------
