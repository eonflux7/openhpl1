// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_PLAYER_HANDS_H
#define GAME_PLAYER_HANDS_H

#include "core/StdAfx.h"
#include "core/GameTypes.h"

using namespace hpl;

class cInit;

class cHudModelPose
{
public:
	cHudModelPose(){}
	cHudModelPose(const cVector3f& avPos,const cVector3f& avRot)
	{
		mvPos = avPos;
		mvRot = avRot;
	}

	cMatrixf ToMatrix()
	{
		cMatrixf mtxPose = cMath::MatrixRotate(mvRot,eEulerRotationOrder_XYZ);
		mtxPose.SetTranslation(mvPos);
		return mtxPose;
	}

	cVector3f mvPos;
	cVector3f mvRot;
};

enum eHudModelState
{
	eHudModelState_Idle,
	eHudModelState_Equip,
	eHudModelState_Unequip
};

cHudModelPose GetPoseFromElem(const tString &asName, TiXmlElement *apElem);

cMatrixf InterpolatePosesToMatrix(float afT, const cHudModelPose& aPoseA, const cHudModelPose& aPoseB);

class iHudModel
{
friend class cPlayerHands;
public:
	virtual ~iHudModel(){}
	iHudModel(ePlayerHandType aType);

	cMeshEntity *GetEntity(){return mpEntity;}

	void LoadEntities();
	void DestroyEntities();

	void EquipEffect(bool abJustCreated);
	void UnequipEffect();

	void Reset();

	tString msName;
	tString msModelFile;
	
	cHudModelPose mEquipPose;
	cHudModelPose mUnequipPose;

	tString msEquipSound;
	tString msUnequipSound;

	float mfEquipTime;
	float mfUnequipTime;

	eHudModelState GetState(){ return mState;}

	std::vector<cParticleSystem3D*> mvParticleSystems;
	std::vector<cBillboard*> mvBillboards;
	std::vector<iLight3D*> mvLights;
	std::vector<cColor> mvLightColors;
	std::vector<float> mvLightRadii;
	std::vector<cSoundEntity*> mvSoundEntities;

	virtual void LoadData(TiXmlElement *apRootElem)=0;

protected:
	virtual void ResetExtraData()=0;

	virtual bool UpdatePoseMatrix(cMatrixf& aPoseMtx, float afTimeStep){return false;}

	virtual void LoadExtraEntities(){}
	virtual void DestroyExtraEntities(){}

	virtual void PostSceneDraw(){}

	ePlayerHandType mType;

	cInit *mpInit;

	cMesh *mpMesh;
	cMeshEntity *mpEntity;
	
	eHudModelState mState;

	// Normalized equip/unequip progress; throwing can seed a negative reload offset.
	float mfEquipProgress;

	tString msNextModel;
};

class cHudModel_Normal : iHudModel
{
	friend class cPlayerHands;
public:
	cHudModel_Normal() : iHudModel(ePlayerHandType_Normal){}

	void LoadData(TiXmlElement *apRootElem) override{}

	void ResetExtraData() override{}
};

typedef std::map<tString,iHudModel*> tHudModelMap;
typedef tHudModelMap::iterator tHudModelMapIt;

class cPlayerHands : public iUpdateable
{
public:
	cPlayerHands(cInit *apInit);
	~cPlayerHands() override;

	void OnStart() override;
	void Update(float afTimeStep) override;
	void Reset() override;
	void OnDraw() override;
	void OnInterpolate(float afT) override;
	void OnPostBufferSwap() override;

	void OnWorldExit();
	void OnWorldLoad();

	void AddHudModel(iHudModel* apHudModel);
	bool AddModelFromFile(const tString &asFile);

	iHudModel* GetModel(const tString& asName);

	void SetCurrentModel(int alNum, const tString& asName);
	iHudModel* GetCurrentModel(int alNum){ return mvCurrentHudModels[alNum];}

private:
	void UpdatePreviousCameraSamples();
		
	cInit *mpInit;
	// The hud model matrices as the logic set them, while they are moved
	// along with the camera for a frame (see OnInterpolate).
	std::vector<std::pair<cMeshEntity*, cMatrixf> > mvInterpolatedModels;

	// The matrices Update() gave each current hud model in the last two
	// logic steps (cRenderInterpolation), for blending its sway.
	class cHudModelSteps
	{
	public:
		cMeshEntity *mpEntity = nullptr;
		cMatrixf m_mtxPrev;
		cMatrixf m_mtxLast;
		unsigned int mlLastStep = 0;
		bool mbPrevValid = false;
	};
	cHudModelSteps mvHudModelSteps[2];

	cMeshManager *mpMeshManager;

	tVector3fList mlstPositions;
	tVector3fList mlstRotations;
	int mlMaxPositions;
	int mlMaxRotations;

	cVector3f mvSmoothCameraPos;
	cVector3f mvSmoothCameraRot;

	tHudModelMap m_mapHudModels;
	iHudModel* mvCurrentHudModels[2];
	int mlCurrentModelNum;
};


#endif // GAME_PLAYER_HANDS_H
