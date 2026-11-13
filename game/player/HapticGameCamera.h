// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_HAPTIC_GAME_CAMERA_H
#define GAME_HAPTIC_GAME_CAMERA_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

#include <list>

using namespace hpl;

class cInit;
class cPlayer;

enum eHapticGameCameraType
{
	eHapticGameCameraType_Frame,
	eHapticGameCameraType_Centre,
	eHapticGameCameraType_LastEnum
};

class cHapticGameCamera
{
public:
	cHapticGameCamera(cInit *apInit, cPlayer *apPlayer);
	~cHapticGameCamera();

	void Update(float afTimeStep);

	void OnWorldLoad();
	void OnWorldExit();
	void Reset();

	void OnPostSceneDraw();

	bool IsTurning(){ return mbIsTurning;}

	void SetActive(bool abActive);
	void SetCamMoveActive(bool abActive){ mbCamMove = abActive; }

	bool ShowCrosshair();

	void SetType(eHapticGameCameraType aType);

	cMeshEntity* GetHandEntity(){ return mpHandEntity;}
	iCollideShape *GetHandShape() { return mpHandShape;}

	void SetHandVisible(bool abVisible);
	bool GetHandVisible();

    float GetHandReachRadius();

	void SetRenderActive(bool abActive);

	void SetInteractModeCameraSpeed(float afSpeed);
	void SetActionModeCameraSpeed(float afSpeed);
	float GetInteractModeCameraSpeed(){ return mfInteractModeCameraSpeed;}
	float GetActionModeCameraSpeed(){ return mfActionModeCameraSpeed;}

	void SetUseFrame(bool abUseFrame){mbUseFrame = abUseFrame;}
	

private:
	void UpdateCameraOrientation(float afTimeStep);
	void UpdateHand(float afTimeStep);
	void UpdateProxyCovered(float afTimeStep);
	void UpdateProxyInteraction(float afTimeStep);

    	
	bool mbActive;

	bool mbRenderActive;

	cVector3f mvPrevProxyPos;

	int mlContactCount;

	bool mbUseFrame;

	bool mbHandVisible;

	cVector3f mvCentrePos;

	tHapticShapeList mlstActiveShapes;

	float mfInteractModeCameraSpeed;
	float mfActionModeCameraSpeed;

	float mfFrameW;
	float mfFrameH;

	float mfFrameForceStrength;
	float mfFrameMaxForce;
	float mfFrameMoveSpeedX;
	float mfFrameMoveSpeedY;
	float mfFrameMaxMoveSpeed;

	bool mbCamMove;

	eHapticGameCameraType mType;

	cInit *mpInit;
	cPlayer *mpPlayer;
	iLowLevelHaptic *mpLowLevelHaptic;
	cScene *mpScene;

	bool mbHapticWasActive;
	bool mbIsTurning;
	bool mbAtEdge;

	iHapticForce *mpForce;

	cMeshEntity *mpHandEntity;
	iCollideShape *mpHandShape;

	iGpuProgram *mpVtxProgram;

	std::list<cVector3f> mlstProxyPos;
};

#endif //GAME_HAPTIC_GAME_CAMERA_H
