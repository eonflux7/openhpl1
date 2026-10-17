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
#ifndef GAME_GAME_ENTITY_H
#define GAME_GAME_ENTITY_H

#include "core/StdAfx.h"

using namespace hpl;

#include "core/GameTypes.h"
#include "core/SaveTypes.h"

class cInit;
class iGameEntity;
class cInventoryItem;

//-----------------------------------------

class cGameEntityScript : public iSerializable
{
	kSerializableClassInit(cGameEntityScript)
public:
	cGameEntityScript() : msScriptFunc(""){}
	
	int mlNum;
	tString msScriptFunc;
};

typedef std::map<tString, cGameEntityScript*> tGameEntityScriptMap;
typedef tGameEntityScriptMap::iterator tGameEntityScriptMapIt;

//-----------------------------------------

typedef std::map<tString, int> tGameEntityVarMap;
typedef tGameEntityVarMap::iterator tGameEntityVarMapIt;

//------------------------------------------

class iGameEntity;

//------------------------------------------

class cGameEntityAnimation_SaveData : public iSerializable
{
	kSerializableClassInit(cGameEntityAnimation_SaveData)
public:
	bool mbActive;
	bool mbLoop;
	
	float mfWeight;
	float mfFadeStep;
	float mfTimePos;
	float mfSpeed;
};
//------------------------------------------

class iGameEntity_SaveData : public iSerializable
{
	kSerializableClassInit(iGameEntity_SaveData)
public:
	~iGameEntity_SaveData() override;

	bool mbActive;

	tString msFileName;

	tString msName;

	float mfHealth;
	
	float mfMaxExamineDist;
	float mfMaxInteractDist;

	bool mbHasBeenExamined;
    	
	tWString msGameName;
	tWString msDescription;

	bool mbShowDescritionOnce;

	eGameEntityType mType;

	cMatrixf m_mtxTransform;

	//Black Plague: where the mesh is now, and a move under way. Saves made
	//before have none of these and leave the mesh where the map put it.
	bool mbHasMeshTransform = false;
	cMatrixf m_mtxMeshTransform;

	cVector3f mvMoveGoal = 0;
	float mfMoveAcceleration = 0;
	float mfMoveSpeed = 0;
	float mfMoveMaxSpeed = 0;
	float mfMoveSlowDownDist = 0;
	bool mbMoving = false;
	tString msMoveCallback;

	//Engine types
	cContainerVec<cEngineBody_SaveData> mvBodies;
	cContainerVec<cEnginePS_SaveData> mvPS;
	cContainerVec<cEnginePS_SaveData> mvBeams;
	cContainerVec<cEngineLight_SaveData> mvLights;
	cContainerVec<cEngineSound_SaveData> mvSounds;

	cEnginePS_SaveData* GetParticleSystem(cParticleSystem3D* apPS);
	cEngineSound_SaveData* GetSoundEntity(cSoundEntity* apSound);

	//Animations
	cContainerVec<cGameEntityAnimation_SaveData> mvAnimations;
	
	//Callbacks
	cContainerList<cSaveGame_cGameCollideScript> mlstCollideCallbacks;

	cContainerList<cGameEntityScript> mlstCallbackScripts;

	//Script variables
	cContainerList<cScriptVar> mlstVars;
	
	//Creation
	virtual iGameEntity* CreateEntity()=0;
};

//------------------------------------------

class iGameEntity
{
friend class cPlayer;
public:
	iGameEntity(cInit *apInit, const tString &asName);
	virtual ~iGameEntity();

	const tString& GetName(){ return msName;}

	bool IsActive(){ return mbActive;}
	void SetActive(bool abActive);
	virtual void OnSetActive(bool abActive){}

	virtual bool IsSaved(){ return mbIsSaved;}

	virtual void OnWorldLoad(){}
	virtual void OnPostLoadScripts(){}
	virtual void OnWorldExit(){}
	virtual void Update(float afTimeStep){}
	virtual void OnDraw(){}
	virtual void OnPostSceneDraw(){}

	void OnUpdate(float afTimeStep);
	void UpdateMove(float afTimeStep);

	// On success, the attached mesh follows entity visibility and destruction;
	// failure leaves it unattached when the skeleton or named bone is absent.
	bool AttachMeshEntityToBone(cMeshEntity *apEntity, const tString &asBone);

	/**
	 * Black Plague: moves the first body, or the mesh if bodyless, toward a world position.
	 * \param afMaxSpeed speed reached, in m/s
	 * \param afAcceleration in m/s^2
	 * \param afSlowDownDist the speed falls off linearly inside this distance from the goal
	 * \param asCallback script function called on arrival: Func(string asEntity)
	 */
	void MoveToPosition(const cVector3f& avPos, float afMaxSpeed, float afAcceleration,
						float afSlowDownDist, const tString& asCallback);
	bool IsMoving(){ return mbMoving;}

	eGameEntityType GetType(){ return mType;}

	const tString& GetSubType(){ return msSubType;}	

	void SetDescription(const tWString& asDesc){ msDescription = asDesc;}
	const tWString& GetDescription(){ return msDescription;}

	void SetShowDescritionOnce(bool abShowOnce){mbShowDescritionOnce = abShowOnce;}
	bool GetShowDescritionOnce(){ return mbShowDescritionOnce;}

	bool GetHasBeenExamined(){ return mbHasBeenExamined;}

	void SetGameName(const tWString& asName){ msGameName = asName;}
	const tWString& GetGameName(){ return msGameName;}

	iPhysicsBody *GetBody(int alIndex){ return mvBodies[alIndex];}
	int GetBodyNum(){ return (int)mvBodies.size();}
	void SetBodies(std::vector<iPhysicsBody*> &avBodies){ mvBodies = avBodies;}

	iPhysicsJoint *GetJoint(int alIndex){ return mvJoints[alIndex];}
	int GetJointNum(){ return (int)mvJoints.size();}
	void SetJoints(std::vector<iPhysicsJoint*> &avJoints){ mvJoints = avJoints;}

	int GetLightNum(){ return (int)mvLights.size();}
	iLight3D* GetLight(int alIndex){ return mvLights[alIndex];}
	void SetLights(std::vector<iLight3D*> &avLights){mvLights = avLights;}

	
	void SetParticleSystems(std::vector<cParticleSystem3D*> &avParticleSystems){mvParticleSystems = avParticleSystems;}
	void SetBillboards(std::vector<cBillboard*> &avBillboards){mvBillboards = avBillboards;}
	void SetBeams(std::vector<cBeam*> &avBeams){mvBeams = avBeams;}
	void SetSoundEntities(std::vector<cSoundEntity*> &avSoundEntities){mvSoundEntities = avSoundEntities;}
	
	size_t GetHapticShapeNum(){ return mvHapticShapes.size();}
	iHapticShape* GetHapticShape(size_t alIndex){ return mvHapticShapes[alIndex];}
	void SetHapticShapes(std::vector<iHapticShape*> &avShapes){mvHapticShapes = avShapes;}

	void DestroyLight(iLight3D *apLight);
	void DestroyParticleSystem(cParticleSystem3D* apPS);
	void DestroyBillboard(cBillboard* apBillboard);
	// Remove from the entity list and destroy through the world.
	void DestroySoundEntity(cSoundEntity* apSound);
	
	cMeshEntity *GetMeshEntity(){return mpMeshEntity;}
	void SetMeshEntity(cMeshEntity *apEntity){mpMeshEntity = apEntity;}
	
	void PlayerPick();
	void PlayerInteract();
	void PlayerExamine();

	virtual void OnPlayerPick(){}
	virtual void OnPlayerInteract(){}
	virtual void OnPlayerExamine();

	virtual bool OnUseItem(cInventoryItem *apItem){return false;}

	virtual void BreakAction(){}

	virtual void OnDamage(float afDamage){}
	virtual void OnDeath(float afDamage){}

	virtual void OnPlayerGravityCollide(iCharacterBody *apCharBody, cCollideData *apCollideData){}

	bool GetHasInteraction(){ return mbHasInteraction;}

	float GetMaxExamineDist(){ return mfMaxExamineDist;}
	void SetMaxExamineDist(float afDistance){ mfMaxExamineDist = afDistance;}
	float GetMaxInteractDist(){ return mfMaxInteractDist;}
	void SetMaxInteractDist(float afDistance){ mfMaxInteractDist = afDistance;}

	virtual eCrossHairState GetPickCrossHairState(iPhysicsBody *apBody);
	virtual float GetPickedDistance();

	void AddCollideScript(eGameCollideScriptType aType,const tString &asFunc, const tString &asEntity);
	void RemoveCollideScript(eGameCollideScriptType aType,const tString &asEntity);
	void RemoveCollideScriptWithChildEntity(iGameEntity *apEntity);
	
	void AddScript(eGameEntityScriptType aType,const tString &asFunc);
	void RemoveScript(eGameEntityScriptType aType);

	void CreateVar(const tString &asName, int alVal);
	void SetVar(const tString &asName, int alVal);
	void AddVar(const tString &asName, int alVal);
	int GetVar(const tString &asName);

	void Damage(float afDamage, int alStrength);
	float GetHealth(){ return mfHealth;}
	void SetHealth(float afHealth);

	void SetToughness(int alToughness){ mlToughness = alToughness;}
	int GetToughness(){ return mlToughness;}

	bool GetDestroyMe(){ return mbDestroyMe;}
	bool GetBreakMe(){ return mbBreakMe;}
	
	void Break(){ mbBreakMe = true;}
	void DestroyMe(){ mbDestroyMe = true;}

    void SetCharBody(iCharacterBody *apCharBody){ mpCharBody = apCharBody;}
	iCharacterBody* GetCharBody(){ return mpCharBody;}

	void SetUpTransMaterials();
	void SetTransActive(bool abActive);

	void SetPauseControllers(bool abPause){ mbPauseControllers = abPause;}
	bool GetPauseControllers(){ return mbPauseControllers;}

	void SetPauseGravity(bool abPause){ mbPauseGravity = abPause;}
	bool GetPauseGravity(){ return mbPauseGravity;}

	const cMatrixf& GetOnLoadTransform(){ return m_mtxOnLoadTransform;}
	void SetOnLoadTransform(const cMatrixf& a_mtxPose){ m_mtxOnLoadTransform = a_mtxPose;}

	void SetLastImpulse(const cVector3f &avVec){ mvLastImpulse = avVec;}

	const tString& GetFileName(){ return msFileName;}

	cGameEntityScript* GetCallbackScript(eGameEntityScriptType aType) {return mvCallbackScripts[aType];}

	//Save data stuff
	void SetSaveData(iGameEntity_SaveData *apData){mpSaveData = apData;}
	iGameEntity_SaveData *GetSaveData(){ return mpSaveData;}

	virtual iGameEntity_SaveData* CreateSaveData()=0;
	virtual void LoadFromSaveData(iGameEntity_SaveData* apSaveData);
	virtual void SaveToSaveData(iGameEntity_SaveData* apSaveData);
	virtual void SetupSaveData(iGameEntity_SaveData *apSaveData);

protected:
	tString GetScriptCommand(eGameEntityScriptType aType);

	void PreloadModel(const tString &asFile);


	cInit *mpInit;

	bool mbActive;

	tString msFileName;

	bool mbIsSaved;

	bool mbDestroyMe;
	bool mbBreakMe;

	bool mbHasInteraction;

	bool mbHasBeenExamined;

	tString msName;

	tString msSubType;

	float mfHealth;
	int mlToughness;
	
	tWString msGameName;
	tWString msDescription;

	bool mbShowDescritionOnce;

	float mfMaxExamineDist;
	float mfMaxInteractDist;

	eGameEntityType mType;

	cVector3f mvMoveGoal;
	float mfMoveAcceleration;
	float mfMoveSpeed;
	float mfMoveMaxSpeed;
	float mfMoveSlowDownDist;
	bool mbMoving;
	tString msMoveCallback;

	bool mbPauseControllers;
	bool mbPauseGravity;

	cMatrixf m_mtxOnLoadTransform;

	iGameEntity_SaveData *mpSaveData;

	std::vector<iPhysicsBody*> mvBodies;
	std::vector<iPhysicsJoint*> mvJoints;

	std::vector<iHapticShape*> mvHapticShapes;
	
	std::vector<iLight3D*> mvLights;
	std::vector<cParticleSystem3D*> mvParticleSystems;
	std::vector<cBillboard*> mvBillboards;

	//Black Plague: mesh entities hung on a bone (an Infected's weapon)
	std::list<cMeshEntity*> mlstAttachedMeshEntities;
	std::vector<cBeam*> mvBeams;
	std::vector<cSoundEntity*> mvSoundEntities;

	bool mbSaveLights;
	
	cMeshEntity *mpMeshEntity;

	std::vector<iMaterial*> mvNormalMaterials;
	std::vector<iMaterial*> mvTransMaterials;
	bool mbTransActive;
	bool mbTransShadow;

	cVector3f mvLastImpulse; 

	std::vector<cMesh*> mvPreloadedBreakMeshes;

	iCharacterBody *mpCharBody;

	cGameEntityScript *mvCallbackScripts[eGameEntityScriptType_LastEnum];
	tGameCollideScriptMap m_mapCollideCallbacks;

	tGameEntityVarMap m_mapVars;

	bool mbUpdatingCollisionCallbacks;
};



#endif // GAME_GAME_ENTITY_H
