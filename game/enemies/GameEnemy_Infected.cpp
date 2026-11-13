// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later

// The Infected of Penumbra: Black Plague. Frictional Games never released
// Black Plague's game code; this is the Dog of Overture changed after what
// the game's binary does.
#include "enemies/GameEnemy_Infected.h"


#include "player/Player.h"
#include "player/PlayerHelper.h"
#include "systems/AttackHandler.h"
#include "systems/EffectHandler.h"
#include "systems/GameMusicHandler.h"

// BASE STATE

iGameEnemyState_Infected_Base::iGameEnemyState_Infected_Base(int alId, cInit *apInit, iGameEnemy *apEnemy)
							: iGameEnemyState_DogBase(alId,apInit,apEnemy)
{	
	mpEnemyInfected = static_cast<cGameEnemy_Infected*>(mpEnemy);
}

// HUNT STATE

void cGameEnemyState_Infected_Hunt::OnEnterState(iGameEnemyState *apPrevState)
{
	//Animation
	mpEnemy->UseMoveStateAnimations();

	//Setup body
	mpEnemy->SetupBody();

	if(mpInit->mDifficulty == eGameDifficulty_Easy)
		mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward,mpEnemyInfected->mfHuntSpeed*0.7f);
	else if(mpInit->mDifficulty == eGameDifficulty_Normal)
		mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward,mpEnemyInfected->mfHuntSpeed);
	else 
		mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward,mpEnemyInfected->mfHuntSpeed*1.25f);

	if(mfHuntSoundDelaySeconds <= 0)
	{
		mfHuntSoundDelaySeconds = cMath::RandRectf(mpEnemyInfected->mfHuntSoundMinInteraval,
											mpEnemyInfected->mfHuntSoundMaxInteraval);
	}

	//Setup enemy
	mpEnemy->SetFOV(mpEnemyInfected->mfHuntFOV);

	mfPathUpdateDelaySeconds =0;
	mfPathUpdateIntervalSeconds = 1.0f;
	mbFreePlayerPath = false;
	
	mbBreakingDoor = false;
	mbFoundNoPath = false;
	mbLostPlayer = false;
	mfLostPlayerRemainingSeconds =0;
	mfLostPlayerDurationSeconds = mpEnemyInfected->mfHuntForLostPlayerTime;

	mlStuckAtMaxCount =0;

    mpInit->mpMusicHandler->AddAttacker(mpEnemy);

	if(mpEnemy->GetOnHuntCallback() != "")
	{
		tString sCommand = mpEnemy->GetOnHuntCallback() + "(\""+mpEnemy->GetName()+"\")";
		mpInit->RunScriptCommand(sCommand);
	}
}

void cGameEnemyState_Infected_Hunt::OnLeaveState(iGameEnemyState *apNextState)
{
	mpMover->SetMaxDoorToughness(-1);
}

void cGameEnemyState_Infected_Hunt::OnUpdate(float afTimeStep)
{
	if(mpPlayer->GetHealth() <=0){
		mpEnemy->ChangeState(STATE_IDLE);
		return;
	}

	//The sound made now and then while hunting
	if(mfHuntSoundDelaySeconds <= 0)
	{
		mfHuntSoundDelaySeconds = cMath::RandRectf(mpEnemyInfected->mfHuntSoundMinInteraval,
											mpEnemyInfected->mfHuntSoundMaxInteraval);
		mpEnemy->PlaySound(mpEnemyInfected->msHuntSound);
	}
	else
	{
		mfHuntSoundDelaySeconds -= afTimeStep;
	}

	if(mpMover->GetStuckCounter() > 0.6f)
	{
		if(mpEnemy->CheckForDoor())
		{
			mbBreakingDoor = true;
			mpEnemy->ChangeState(STATE_BREAKDOOR);
			mpMover->ResetStuckCounter();
			return;
		}

		mlStuckAtMaxCount++;
		if(mlStuckAtMaxCount >= 6)
		{
			mpEnemy->ChangeState(STATE_IDLE);
			mpMover->ResetStuckCounter();
			mlStuckAtMaxCount =0;
			return;
		}
		mpMover->ResetStuckCounter();
	}

	//The wait after an attack made from afar
	if(mfAttackPauseRemainingSeconds > 0) mfAttackPauseRemainingSeconds -= afTimeStep;

	if(mfPathUpdateDelaySeconds <=0)
	{
		mbFoundNoPath = false;
		mfPathUpdateDelaySeconds = mfPathUpdateIntervalSeconds;
		
		cAINodeContainer *pNodeCont = mpEnemy->GetMover()->GetNodeContainer();

		//Check if there is a free path to the player
		if(mbLostPlayer == false && mpMover->FreeDirectPathToChar(mpPlayer->GetCharacterBody()))
		{
			mbFreePlayerPath = true;
			mpMover->Stop();
			mpMover->SetMaxDoorToughness(-1);
		}
		else
		{
			mbFreePlayerPath = false;
		}
		
		//Get path to player
		if(mbFreePlayerPath==false && mbLostPlayer == false)
		{
			if(mpEnemy->GetDoorBreakCount()> 6.0f)
			{
				mpMover->SetMaxDoorToughness(0);
			}

			if(mpMover->MoveToPos(mpEnemy->GetLastPlayerPos())==false)
			{
				bool bFoundAnotherWay = false;
				
				if(bFoundAnotherWay == false)
				{
					mfPathUpdateDelaySeconds = mfPathUpdateIntervalSeconds*5.0f;
					mpMover->Stop();
					//Set this so the enemey at least runs toward the player.
					mbFoundNoPath = true;
				}
			}

		}
	}
	else
	{
		mfPathUpdateDelaySeconds -= afTimeStep;
	}
	
	//Go directly towards the player
	if(mbFreePlayerPath || (mbFoundNoPath && mpMover->IsMoving()==false))
	{
		//Go towards player
		mpMover->MoveDirectToPos(mpPlayer->GetCharacterBody()->GetFeetPosition(),afTimeStep);
		
        //Check if he should attack.
		float fDist = mpMover->DistanceToChar2D(mpPlayer->GetCharacterBody());

		//Just after a swing made while moving in, only a player who is near is attacked
		if(mfAttackPauseRemainingSeconds > 0 && fDist > mpEnemyInfected->mfAttackStillDistance) return;

		if(fDist < mpEnemyInfected->mfAttackDistance)
		{
			float fHeight = mpMover->GetCharBody()->GetPosition().y -
							mpPlayer->GetCharacterBody()->GetPosition().y;
			
			//Player is above
			if(fHeight < 0)
			{
				fHeight += mpMover->GetCharBody()->GetSize().y/2.0f;
				float fMax = mpEnemyInfected->mvAttackDamageSize.y;
				if(fHeight <= -fMax && cMath::RandRectf(0,1) >= 0.2f)
				{
					//too far up, most of the time
					mpEnemy->ChangeState(STATE_FLEE);
					return;
				}
			}

			mpEnemy->ChangeState(STATE_ATTACK);
			if(fDist > mpEnemyInfected->mfAttackStillDistance) mfAttackPauseRemainingSeconds = 1.0f;
		}
	}
	//Update path search
	else if(mbFreePlayerPath==false)
	{
		if( mbLostPlayer==false && mpMover->IsMoving()==false && mpEnemy->CanSeePlayer()==false)
		{
			mbLostPlayer = true;
			mfLostPlayerRemainingSeconds = mfLostPlayerDurationSeconds;
		}
		
		if(mbLostPlayer)
		{
			mpMover->GetCharBody()->Move(eCharDir_Forward,1.0f,afTimeStep);
			
			mfLostPlayerRemainingSeconds -= afTimeStep;
			if(mfLostPlayerRemainingSeconds <= 0 || mpMover->GetStuckCounter()>0.5f)
			{
				mpEnemy->ChangeState(STATE_IDLE);
			}
		}
	}
}

void cGameEnemyState_Infected_Hunt::OnSeePlayer(const cVector3f &avPosition, float afChance)
{
	if(mbLostPlayer && afChance >= mpEnemyInfected->mfHuntMinSeeChance)
	{
		mbLostPlayer = false;
		mfPathUpdateDelaySeconds =0;
	}
}

bool cGameEnemyState_Infected_Hunt::OnHearNoise(const cVector3f &avPosition, float afVolume)
{
	//If player is lost the sound might be of help
	if(mbLostPlayer)
	{
		//Check if sound can be heard
		if(afVolume >= mpEnemyInfected->mfHuntMinHearVolume)
		{
			//Check if a node is found near the sound.
			cAINode *pNode = mpMover->GetAINodeAtPosInRange(avPosition,0.0f,5.0f,true, 0.1f);
			if(pNode)
			{
				//Update last player postion.
				mbLostPlayer = false;
				mfPathUpdateDelaySeconds =0;
				mpEnemy->SetLastPlayerPos(pNode->GetPosition());

				return true;
			}
		}
	}
	
	return false;
}

void cGameEnemyState_Infected_Hunt::OnDraw()
{
	float fWantedSpeed = mpMover->GetCharBody()->GetMoveSpeed(eCharDir_Forward);
	float fRealSpeed = cMath::Vector3Dist(	mpMover->GetCharBody()->GetPosition(), 
											mpMover->GetCharBody()->GetLastPosition());
	fRealSpeed = fRealSpeed / (1.0f/60.0f);

	float fDist = mpMover->DistanceToChar2D(mpPlayer->GetCharacterBody());

	mpInit->mpDefaultFont->Draw(cVector3f(0,110,100),14,cColor(1,1,1,1),eFontAlign_Left,
		_W("LostPlayerCount: %f FreePath: %d NoPath: %d MaxStuck: %d Dist: %f / %f"),
															mfLostPlayerRemainingSeconds,mbFreePlayerPath,
															mbFoundNoPath,
															mlStuckAtMaxCount,
															fDist,
															mpEnemyInfected->mfAttackDistance);
}

// ATTACK STATE

void cGameEnemyState_Infected_Attack::OnEnterState(iGameEnemyState *apPrevState)
{
	//The kind of attack: a grab for the throat when close (never twice in a
	//row), a swing standing still when near, else a swing while moving in.
	float fDist = mpMover->DistanceToChar2D(mpPlayer->GetCharacterBody());
	float fRand = cMath::RandRectf(0,1);
	bool bStill;

	if(	mbGrabThroat==false && mpEnemyInfected->mbAttackGrabThroat &&
		fDist < mpEnemyInfected->mfAttackGrabThroatDist &&
		fRand < mpEnemyInfected->mfAttackGrabThroatChance)
	{
		mbGrabThroat = true;
		bStill = true;
		mfJumpTimer = 0;
		//Not in the original, where the count is never set back and so a
		//second grab did no damage and let go at once.
		mfDamageTimer = 0;
	}
	else if(fDist < mpEnemyInfected->mfAttackStillDistance)
	{
		mbGrabThroat = false;
		bStill = true;
		mfJumpTimer = 0;
	}
	else
	{
		mbGrabThroat = false;
		bStill = false;
		mfJumpTimer = mpEnemyInfected->mfAttackJumpTime;
	}

	//Setup body
	mpEnemy->SetupBody();
	if(mpEnemyInfected->mfAttackSpeed>0)
	{
		mpMover->GetCharBody()->SetMaxPositiveMoveSpeed(eCharDir_Forward,mpEnemyInfected->mfAttackSpeed);
		mpMover->SetMaxTurnSpeed(10000.0f);
	}
	
	//Animation
	if(mbGrabThroat)
	{
		mpEnemy->PlayAnim("GrabThroat",false,0.2f);

		//The player is held and made to look at the enemy
		mpPlayer->SetActive(false);

		cVector3f vPos = mpMover->GetCharBody()->GetPosition() +
						cVector3f(0,mpMover->GetCharBody()->GetSize().y*0.5f,0);

		mpInit->mpPlayer->GetLookAt()->SetTarget(vPos,12.0f,30.0f);
		mpInit->mpPlayer->GetLookAt()->SetActive(true);
		mpInit->mpPlayer->GetLookAt()->SetZoomTarget(0.61f,0.075f,1.0f);

		if(mpInit->mpPlayer->GetMoveState() == ePlayerMoveState_Crouch)
			mpInit->mpPlayer->ChangeMoveState(ePlayerMoveState_Walk);

		mfLookAtTimer = 3.0f;
	}
	else
	{
		tString sAnim = "";
		if(mpEnemyInfected->HasWeapon())
		{
			if(bStill)	sAnim = "SwingStill_0" + cString::ToString(cMath::RandRectl(1,3));
			else		sAnim = "SwingLaunch";
		}
		else
		{
			if(bStill)	sAnim = "SwingClawsStill_0" + cString::ToString(cMath::RandRectl(1,3));
			else		sAnim = "SwingClawsLaunch";
		}

		mpEnemy->PlayAnim(sAnim,false,0.2f);
		mpAttackAnim = mpEnemy->GetMeshEntity()->GetAnimationStateFromName(sAnim);
	}

	//Other
	if(mbGrabThroat==false)
		mpEnemyInfected->PlaySound(mpEnemyInfected->msAttackStartSound);

	mbAttacked = false;

	if(mpEnemy->GetOnAttackCallback() != "")
	{
		tString sCommand = mpEnemy->GetOnAttackCallback() + "(\""+mpEnemy->GetName()+"\")";
		mpInit->RunScriptCommand(sCommand);
	}
}

void cGameEnemyState_Infected_Attack::OnLeaveState(iGameEnemyState *apNextState)
{
	mpEnemyInfected->SetSkipSoundTriggerCount(2.0f);
	mpMover->ResetStuckCounter();

	if(mbGrabThroat)
	{
		mpPlayer->SetActive(true);
		mpInit->mpPlayer->GetLookAt()->SetActive(false);
	}
}

void cGameEnemyState_Infected_Attack::OnUpdate(float afTimeStep)
{
	//The grab for the throat: the time held is counted in the damage timer.
	if(mbGrabThroat)
	{
		if(mfDamageTimer >= mpEnemyInfected->mfAttackGrabThroatLength) return;

		mfDamageTimer += afTimeStep;

		//The view shakes: a new point near the enemy's middle to look at
		mfLookAtTimer -= afTimeStep * 2.5f;
		if(mfLookAtTimer <= 0)
		{
			cVector3f vPos = mpMover->GetCharBody()->GetPosition() +
							cVector3f(0,mpMover->GetCharBody()->GetSize().y*0.5f,0);
			vPos += cMath::RandRectVector3f(cVector3f(-0.1f),cVector3f(0.1f));

			mpInit->mpPlayer->GetLookAt()->SetTarget(vPos,4.0f,30.0f);
			mfLookAtTimer = 1.0f;
		}

		if(mbAttacked==false && mfDamageTimer >= mpEnemyInfected->mfAttackGrabThroatDamageTime)
		{
			mpPlayer->Damage(mpEnemyInfected->mfAttackGrabThroatDamage,ePlayerDamageType_BloodSplash);
			mbAttacked = true;
		}

		if(mfDamageTimer >= mpEnemyInfected->mfAttackGrabThroatLength)
		{
			mpPlayer->SetActive(true);
			mpInit->mpPlayer->GetLookAt()->SetActive(false);
		}
		return;
	}

	//Move forward
	if(mpEnemyInfected->mfAttackSpeed>0)
	{
		if(mfJumpTimer > 0)
		{
			mpMover->MoveDirectToPos(mpPlayer->GetCharacterBody()->GetFeetPosition(),afTimeStep);
			mfJumpTimer-= afTimeStep;
		}
		else
		{
			mpMover->TurnToPos(mpPlayer->GetCharacterBody()->GetFeetPosition());
		}
	}

	if(mbAttacked) return;

	// The blow lands at the animation's special event
	if(mpAttackAnim==nullptr || mpAttackAnim->IsAfterSpecialEvent()==false) return;

	cVector3f vPos =	mpMover->GetCharBody()->GetPosition() +
						mpMover->GetCharBody()->GetForward() * 
						mpEnemyInfected->mfAttackDamageRange;

	cVector3f vRot = cVector3f(0,mpMover->GetCharBody()->GetYaw(),0);
	cMatrixf mtxOffset = cMath::MatrixRotate(vRot,eEulerRotationOrder_XYZ);
	mtxOffset.SetTranslation(vPos);
	
	eAttackTargetFlag target = eAttackTargetFlag_Player | eAttackTargetFlag_Bodies;

	float fDamage = cMath::RandRectf(	mpEnemyInfected->mfAttackMinDamage,
										mpEnemyInfected->mfAttackMaxDamage);
	if(mpEnemyInfected->HasWeapon()) fDamage += mpEnemyInfected->GetWeaponDamage();
	
	if(mpInit->mpAttackHandler->CreateShapeAttack(mpEnemyInfected->GetAttackShape(),
								mtxOffset,
								mpMover->GetCharBody()->GetPosition(),
								fDamage,
								
								mpEnemyInfected->mfAttackMinMass, mpEnemyInfected->mfAttackMaxMass,
								mpEnemyInfected->mfAttackMinImpulse, mpEnemyInfected->mfAttackMaxImpulse,
								
								mpEnemyInfected->mlAttackStrength,

								target,nullptr))
	{
		mpEnemyInfected->PlaySound(mpEnemyInfected->msAttackHitSound);
	}
	mbAttacked = true;
}

void cGameEnemyState_Infected_Attack::OnAnimationOver(const tString &asName)
{
	mpEnemy->ChangeState(STATE_HUNT);
}

void cGameEnemyState_Infected_Attack::OnPostSceneDraw()
{
	cCamera3D *pCamera = static_cast<cCamera3D*>(mpInit->mpGame->GetScene()->GetCamera());
	
	cVector3f vPos =	mpMover->GetCharBody()->GetPosition() +
						mpMover->GetCharBody()->GetForward() * 
						mpEnemyInfected->mfAttackDamageRange;

	cVector3f vRot = cVector3f(0,mpMover->GetCharBody()->GetYaw(),0);
	cMatrixf mtxOffset = cMath::MatrixRotate(vRot,eEulerRotationOrder_XYZ);
	mtxOffset.SetTranslation(vPos);

	mpInit->mpGame->GetGraphics()->GetLowLevel()->SetDrawMatrices(mtxOffset, pCamera->GetViewMatrix(), pCamera->GetProjectionMatrix());

	cVector3f vSize = mpEnemyInfected->GetAttackShape()->GetSize();
	mpInit->mpGame->GetGraphics()->GetLowLevel()->DrawBoxMaxMin(vSize*0.5f,vSize*-0.5f,
		cColor(1,0,1,1));
}

// BREAK DOOR STATE

void cGameEnemyState_Infected_BreakDoor::OnUpdate(float afTimeStep)
{
	cGameEnemyState_DogBase_BreakDoor::OnUpdate(afTimeStep);

	//Black Plague counts the blow's delay down twice per update while the
	//blow is still to come. Overture's Dog no longer does.
	if(mfDamageTimer > 0) mfDamageTimer -= afTimeStep;
}

void cGameEnemyState_Infected_BreakDoor::OnAnimationOver(const tString &asName)
{
	mpEnemy->ChangeState(mlPreviousState);
}

// KNOCK DOWN STATE

void cGameEnemyState_Infected_KnockDown::OnEnterState(iGameEnemyState *apPrevState)
{
	//A blow only makes the Infected flinch, and it is whole again after it.
	mpEnemy->SetHealth(mpEnemyInfected->GetMaxHealth());

	//Animation
	mpEnemy->PlayAnim("Flinch",false,0.2f);

	//Sound
	mpEnemy->PlaySound(mpEnemyInfected->msKnockDownSound);

	//Setup body
	mpEnemy->SetupBody();
}

void cGameEnemyState_Infected_KnockDown::OnLeaveState(iGameEnemyState *apNextState)
{
}

void cGameEnemyState_Infected_KnockDown::OnUpdate(float afTimeStep)
{
}

void cGameEnemyState_Infected_KnockDown::OnAnimationOver(const tString &asName)
{
	mpEnemy->ChangeState(STATE_HUNT);
}

// DEAD STATE

void cGameEnemyState_Infected_Dead::OnEnterState(iGameEnemyState *apPrevState)
{
	//Setup body
	mpEnemy->SetupBody();
	mpEnemy->GetMover()->Stop();

	//Animation: no rag doll, the Infected falls by animation
	mpEnemy->PlayAnim("DeathStart",false,0.7f);

	//Sound
	if(mpEnemy->IsLoading()==false)
		mpEnemy->PlaySound(mpEnemyInfected->msDeathSound);

	mpInit->mpMusicHandler->RemoveAttacker(mpEnemy);

	mlAnimCount =0;
}

void cGameEnemyState_Infected_Dead::OnLeaveState(iGameEnemyState *apNextState)
{
}

void cGameEnemyState_Infected_Dead::OnUpdate(float afTimeStep)
{
}

void cGameEnemyState_Infected_Dead::OnAnimationOver(const tString &asName)
{
	if(mlAnimCount==0)
	{
		mpEnemy->PlayAnim("DeathMid",false,0.7f);

		mpEnemy->GetMover()->GetCharBody()->SetEntity(nullptr);
		mpEnemy->GetMover()->GetCharBody()->SetActive(false);
	}
	mlAnimCount++;
}

cGameEnemy_Infected::cGameEnemy_Infected(cInit *apInit,const tString& asName,TiXmlElement *apGameElem) : iGameEnemy_DogBase(apInit,asName,apGameElem)
{
	mbHasWeapon = false;
	mbHasFlashLight = false;
	mfWeaponDamage = 0;
	mpWeaponEntity = nullptr;
	mpFlashLightEntity = nullptr;

	mbAccurateClimbing = true;
	mbBlackPlagueInvestigate = true;

	msIdleAnim = "Idle";

	//Black Plague's defaults are 0, not the Idle values
	mfHuntMinSeeChance = cString::ToFloat(apGameElem->Attribute("HuntMinSeeChance"),0);
	mfHuntMinHearVolume = cString::ToFloat(apGameElem->Attribute("HuntMinHearVolume"),0);
	msHuntSound = cString::ToString(apGameElem->Attribute("HuntSound"),"");
	mfHuntSoundMinInteraval = cString::ToFloat(apGameElem->Attribute("HuntSoundMinInteraval"),0);
	mfHuntSoundMaxInteraval = cString::ToFloat(apGameElem->Attribute("HuntSoundMaxInteraval"),0);
	mvPreloadSounds.push_back(msHuntSound);

	mfAttackStillDistance = cString::ToFloat(apGameElem->Attribute("AttackStillDistance"),0);
	mfAttackStillDamageTime = cString::ToFloat(apGameElem->Attribute("AttackStillDamageTime"),0);
	mbAttackGrabThroat = cString::ToBool(apGameElem->Attribute("AttackGrabThroat"),false);
	mfAttackGrabThroatChance = cString::ToFloat(apGameElem->Attribute("AttackGrabThroatChance"),0);
	mfAttackGrabThroatDist = cString::ToFloat(apGameElem->Attribute("AttackGrabThroatDist"),0);
	mfAttackGrabThroatDamage = cString::ToFloat(apGameElem->Attribute("AttackGrabThroatDamage"),0);
	mfAttackGrabThroatLength = cString::ToFloat(apGameElem->Attribute("AttackGrabThroatLength"),0);
	mfAttackGrabThroatDamageTime = cString::ToFloat(apGameElem->Attribute("AttackGrabThroatDamageTime"),0);

	//Set up states
	AddState(new cGameEnemyState_DogBase_Idle(STATE_IDLE,mpInit,this) );
	AddState(new cGameEnemyState_Infected_Hunt(STATE_HUNT,mpInit,this) );
	AddState(new cGameEnemyState_Infected_Attack(STATE_ATTACK,mpInit,this) );
	AddState(new cGameEnemyState_DogBase_Flee(STATE_FLEE,mpInit,this) );
	AddState(new cGameEnemyState_Infected_KnockDown(STATE_KNOCKDOWN,mpInit,this) );
	AddState(new cGameEnemyState_Infected_Dead(STATE_DEAD,mpInit,this) );
	AddState(new cGameEnemyState_DogBase_Patrol(STATE_PATROL,mpInit,this) );
	AddState(new cGameEnemyState_DogBase_Investigate(STATE_INVESTIGATE,mpInit,this) );
	AddState(new cGameEnemyState_Infected_BreakDoor(STATE_BREAKDOOR,mpInit,this) );
	AddState(new cGameEnemyState_DogBase_CallBackup(STATE_CALLBACKUP,mpInit,this) );
	AddState(new cGameEnemyState_DogBase_MoveTo(STATE_MOVETO,mpInit,this) );
	AddState(new cGameEnemyState_DogBase_Eat(STATE_EAT,mpInit,this) );
	AddState(new cGameEnemyState_DogBase_Attention(STATE_ATTENTION,mpInit,this) );
}

cGameEnemy_Infected::~cGameEnemy_Infected()
{

}

void cGameEnemy_Infected::OnUpdate(float afTimeStep)
{
	if(IsActive()==false) return;
	
	//Regenerate health: fast, an Infected is not worn down
	if(mfHealth > 0 && mfHealth <= mfMaxHealth *0.7f)
	{
		mfHealth += afTimeStep * (500.0f / 60.0f); //500 heal units / min
	}
}

void cGameEnemy_Infected::SetWeapon(const tString &asFile, const cMatrixf &a_mtxOffset, float afDamage, bool abVisible)
{
	if(mbHasWeapon) return;

	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();
	tString sJoint = "WeaponJoint";

	cMesh *pMesh = mpInit->mpGame->GetResources()->GetMeshManager()->CreateMesh(asFile);
	if(pMesh==nullptr)
	{
		Warning("Could not load mesh '%s'\n",asFile.c_str());
		return;
	}

	cMeshEntity *pEntity = pWorld->CreateMeshEntity("WeaponObject",pMesh,true);
	pEntity->SetMatrix(a_mtxOffset);

	AttachMeshEntityToBone(pEntity,sJoint);

	pEntity->SetIsSaved(false);
	pEntity->SetActive(abVisible);
	pEntity->SetVisible(abVisible);

	mpWeaponEntity = pEntity;
	mbHasWeapon = true;
	msWeaponFile = asFile;
	mfWeaponDamage = afDamage;
	m_mtxWeaponMatrix = a_mtxOffset;
}

void cGameEnemy_Infected::AddFlashLight(bool abVisible)
{
	if(mbHasFlashLight) return;

	tString sFile = "weapon_flashlight_no_ray.dae";
	tString sJoint = "FlashlightJoint";

	cMesh *pMesh = mpInit->mpGame->GetResources()->GetMeshManager()->CreateMesh(sFile);
	if(pMesh==nullptr)
	{
		Warning("Could not load mesh '%s'\n",sFile.c_str());
		return;
	}

	cWorld3D *pWorld = mpInit->mpGame->GetScene()->GetWorld3D();

	cMeshEntity *pEntity = pWorld->CreateMeshEntity("WeaponObject",pMesh,true);
	cMatrixf mtxOffset = cMath::MatrixRotate(cVector3f(cMath::ToRad(35),cMath::ToRad(0),cMath::ToRad(60)),
											eEulerRotationOrder_XYZ);
	pEntity->SetMatrix(mtxOffset);

	//The light and the glow of the lamp
	for(int i=0; i< pMesh->GetLightNum(); ++i)
	{
		iLight3D *pLight = pMesh->CreateLightInWorld("Flashlight_light",pMesh->GetLight(i),pEntity,pWorld);
		pLight->SetActive(abVisible);
		pLight->SetVisible(abVisible);
		pLight->SetIsSaved(false);
		mvLights.push_back(pLight);
	}
	for(int i=0; i< pMesh->GetBillboardNum(); ++i)
	{
		cBillboard *pBillboard = pMesh->CreateBillboardInWorld("Flashlight_bb",pMesh->GetBillboard(i),pEntity,pWorld);
		pBillboard->SetActive(abVisible);
		pBillboard->SetVisible(abVisible);
		mvBillboards.push_back(pBillboard);
	}

	AttachMeshEntityToBone(pEntity,sJoint);

	pEntity->SetIsSaved(false);
	pEntity->SetActive(abVisible);
	pEntity->SetVisible(abVisible);

	mbHasFlashLight = true;
	mpFlashLightEntity = pEntity;

	//Walk about holding it up
	msStoppedAnim = "IdleFlashlight";
	msWalkAnim = "WalkFlashlight";
	msRunAnim = "RunFlashlight";
	msIdleAnim = "IdleFlashlight";
}

kBeginSerialize(cGameEnemy_Infected_SaveData,iGameEnemy_SaveData)
kSerializeVar(mbHasWeapon, eSerializeType_Bool)
kSerializeVar(mbHasFlashLight, eSerializeType_Bool)
kSerializeVar(msWeaponFile, eSerializeType_String)
kSerializeVar(mfWeaponDamage, eSerializeType_Float32)
kSerializeVar(m_mtxWeaponMatrix, eSerializeType_Matrixf)
kEndSerialize()

iGameEntity* cGameEnemy_Infected_SaveData::CreateEntity()
{
	return nullptr;
}

iGameEntity_SaveData* cGameEnemy_Infected::CreateSaveData()
{
	return new cGameEnemy_Infected_SaveData();
}

void cGameEnemy_Infected::SaveToSaveData(iGameEntity_SaveData *apSaveData)
{
	//The original calls iGameEntity's here, passing the enemy's own data by.
	__super::SaveToSaveData(apSaveData);
	cGameEnemy_Infected_SaveData *pData = static_cast<cGameEnemy_Infected_SaveData*>(apSaveData);

	kCopyToVar(pData,mbHasWeapon);
	kCopyToVar(pData,mbHasFlashLight);
	kCopyToVar(pData,msWeaponFile);
	kCopyToVar(pData,mfWeaponDamage);
	kCopyToVar(pData,m_mtxWeaponMatrix);
}

void cGameEnemy_Infected::LoadFromSaveData(iGameEntity_SaveData *apSaveData)
{
	__super::LoadFromSaveData(apSaveData);
	cGameEnemy_Infected_SaveData *pData = static_cast<cGameEnemy_Infected_SaveData*>(apSaveData);

	if(pData->mbHasWeapon)
		SetWeapon(pData->msWeaponFile,pData->m_mtxWeaponMatrix,pData->mfWeaponDamage,mbActive);
	if(pData->mbHasFlashLight)
		AddFlashLight(mbActive);
}
