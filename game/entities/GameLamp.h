// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_GAME_LAMP_H
#define GAME_GAME_LAMP_H

#include "core/StdAfx.h"
#include "entities/GameEntity.h"

using namespace hpl;

class cGameLamp_SaveData : public iGameEntity_SaveData
{
	kSerializableClassInit(cGameLamp_SaveData);
public:
	bool mbLit;
	tString msLitChangeCallback;

	bool mbFlickering;

	iGameEntity* CreateEntity() override;
};

class cGameLamp_PS
{
public:
	tString msName;
	tString msDataName;
	cMatrixf m_mtxTransform;
};

class cGameLamp :	public iGameEntity
{
#ifdef __GNUC__
	typedef iGameEntity __super;
#endif
friend class cEntityLoader_GameLamp;
public:
	cGameLamp(cInit *apInit,const tString& asName);
	~cGameLamp(void) override;

	void OnPlayerInteract() override;
	void OnPlayerPick() override;
	bool OnUseItem(cInventoryItem *apItem) override;

	void Update(float afTimeStep) override;

	void SetLitChangeCallback(const tString& asCallback){ msLitChangeCallback = asCallback;}

	void SetLit(bool abLit, bool abFade, bool abRunCallback=true);

	void SetFlicker(bool abFlickering);

	void Init();

	//SaveObject implementation
	iGameEntity_SaveData* CreateSaveData() override;
	void SaveToSaveData(iGameEntity_SaveData *apSaveData) override;
	void LoadFromSaveData(iGameEntity_SaveData *apSaveData) override;

private:
	void SetUpFlicker(int alLightIndex);
	
	bool mbLit;

    float mfBillboardFade;
	float mfTurnOnDurationSeconds;
	float mfTurnOffDurationSeconds;

	bool mbInteractOff;
	bool mbInteractOn;

	tString msTurnOnSound;
	tString msTurnOffSound;

	tString msOnItem;
	tString msOffItem;

	tString msLitChangeCallback;

	tString msOffMaterialName;
	tString msOffSubMesh;
	iMaterial *mpOffMaterial;
	iMaterial *mpOnMaterial;
	cSubMeshEntity *mpSubMesh;

	//Flickering
	bool mbFlickering;
	tString msFlickerOffSound;
	tString msFlickerOnSound;
	tString msFlickerOffPS;
	tString msFlickerOnPS;
	float mfFlickerOnMinLength;
	float mfFlickerOffMinLength;
	float mfFlickerOnMaxLength;
	float mfFlickerOffMaxLength;
	cColor mFlickerOffColor;
	cColor mOffColor = cColor(0,0);	//Black Plague: the lights' colour when off
	float mfFlickerOffRadius;
	bool mbFlickerFade;
	float mfFlickerOnFadeLength;
	float mfFlickerOffFadeLength;
	
	//Init data
	std::vector<cColor> mvLightColors;
	std::vector<cColor> mvBBColors;
	std::vector<cGameLamp_PS> mvParticleSystemNames;

};

class cEntityLoader_GameLamp : public cEntityLoader_Object
{
public:
	cEntityLoader_GameLamp(const tString &asName, cInit *apInit);
	~cEntityLoader_GameLamp() override;

private:
	void BeforeLoad(TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,cWorld3D *apWorld) override;
	void AfterLoad(TiXmlElement *apRootElem, const cMatrixf &a_mtxTransform,cWorld3D *apWorld) override;

	cInit *mpInit;
};


#endif // GAME_GAME_LAMP_H
