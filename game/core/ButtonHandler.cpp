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
#include "core/ButtonHandler.h"

#include "core/Init.h"
#include "player/Player.h"
#include "ui/Inventory.h"
#include "ui/Notebook.h"
#include "ui/NumericalPanel.h"
#include "ui/IntroStory.h"
#include "core/SaveHandler.h"
#include "core/MapHandler.h"
#include "ui/DeathMenu.h"
#include "ui/MapLoadText.h"
#include "ui/PreMenu.h"
#include "ui/Credits.h"
#include "ui/DemoEndText.h"

#include "ui/MainMenu.h"

struct cButtonHandlerAction
{
	tString msName;
	tString msType;
	int mlVal;
	bool mbConfig;
};

// The window height at which turning per mouse count is what it was before
// it stopped depending on the resolution.
static const float kLookReferenceHeight = 1440.0f;

static tString gsLastPlayerAction = "GlowStick";
static cButtonHandlerAction gvDefaultActions[] = {
{"Forward","Keyboard",eKey_w,true},
{"Backward","Keyboard",eKey_s,true},
{"Left","Keyboard",eKey_a,true},
{"Right","Keyboard",eKey_d,true},

{"LeanLeft","Keyboard",eKey_q,true},
{"LeanRight","Keyboard",eKey_e,true},

{"Run","Keyboard",eKey_LSHIFT,true},
{"Jump","Keyboard",eKey_SPACE,true},
{"Crouch","Keyboard",eKey_LCTRL,true},

{"InteractMode","Keyboard",eKey_r,true},
{"LookMode","MouseButton",eMButton_Middle,true},

{"Holster","Keyboard",eKey_x,true},

{"Examine","MouseButton",eMButton_Right,true},
{"Interact","MouseButton",eMButton_Left,true},

{"Inventory","Keyboard",eKey_TAB,true},
{"NoteBook","Keyboard",eKey_n,true},
{"PersonalNotes","Keyboard",eKey_p,true},

{"WheelUp","MouseButton",eMButton_WheelUp,true},
{"WheelDown","MouseButton",eMButton_WheelDown,true},

{"Flashlight","Keyboard",eKey_f,true},
{"GlowStick","Keyboard",eKey_g,true},

{"Escape","Keyboard",eKey_ESCAPE,false},
{"Enter","Keyboard",eKey_RETURN,false},
{"MouseClick","MouseButton",eMButton_Left,false},
{"MouseClickRight","MouseButton",eMButton_Right,false},

{"RightClick","MouseButton",eMButton_Right,false},
{"LeftClick","MouseButton",eMButton_Left,false},

{"One","Keyboard",eKey_1,false},
{"Two","Keyboard",eKey_2,false},
{"Three","Keyboard",eKey_3,false},
{"Four","Keyboard",eKey_4,false},
{"Five","Keyboard",eKey_5,false},
{"Six","Keyboard",eKey_6,false},
{"Seven","Keyboard",eKey_7,false},
{"Eight","Keyboard",eKey_8,false},
{"Nine","Keyboard",eKey_9,false},

//Debug:
{"ResetGame","Keyboard",eKey_F1,false},
{"SaveGame","Keyboard",eKey_F4,false},
{"LoadGame","Keyboard",eKey_F5,false},
#ifdef __APPLE__
{"QuitGame","Keyboard",eKeyModifier_META | eKey_q,false},
#endif
//{"LockInput","Keyboard",eKey_k,false},
{"Screenshot","Keyboard",eKey_F12,false},

//{"Hit","Keyboard",eKey_h,false},
//{"Log","Keyboard",eKey_l,false},
//{"Taunt","Keyboard",eKey_t,false},
{"PrintLog","Keyboard",eKey_l,false},

{"","",0}
};

//////////////////////////////////////////////////////////////////////////
// CONSTRUCTORS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cButtonHandler::cButtonHandler(cInit *apInit)  : iUpdateable("ButtonHandler")
{
	mpInit = apInit;

	mpInput = mpInit->mpGame->GetInput();
	mpLowLevelGraphics = mpInit->mpGame->GetGraphics()->GetLowLevel();
	if(mpInit->mbHasHaptics)
		mpLowLevelHaptic = mpInit->mpGame->GetHaptic()->GetLowLevel();
	else
		mpLowLevelHaptic = nullptr;

	
	mState = eButtonHandlerState_Game;

	mlNumOfActions =0;

	// INIT ALL ACTIONS USED
	cButtonHandlerAction *pBHAction = &gvDefaultActions[0];
	while(pBHAction->msName != "")
	{
		tString sName = pBHAction->msName;
		tString sType = mpInit->mpConfig->GetString("Keys",sName+"_Type",pBHAction->msType);
		tString sVal = mpInit->mpConfig->GetString("Keys",sName+"_Val",cString::ToString(pBHAction->mlVal));

		iAction *pAction = ActionFromTypeAndVal(sName, sType, sVal);
		if(pAction)
		{
			mpInput->AddAction(pAction);
		}
		else
		{
			Warning("Couldn't create action from '%s' and %d\n",pBHAction->msType.c_str(),
																pBHAction->mlVal);
		}
		
		++pBHAction;
		++mlNumOfActions;
	}


	// LOAD SETTINGS
	mfMouseSensitivity = mpInit->mpConfig->GetFloat("Controls","MouseSensitivity",1.0f);

	mlLookStep = 0;
	mvLookDelta = cVector2f(0,0);
	mvLookPerRel = cVector2f(0,0);
	mlLookPlayerState = -1;

	mlCursorStep = 0;
	mvCursorRel = cVector2f(0,0);
	for(int i=0; i<2; ++i)
	{
		mvCursorStart[i] = cVector2f(0,0);
		mvCursorPerRel[i] = cVector2f(0,0);
	}
	mbInvertMouseY = mpInit->mpConfig->GetBool("Controls","InvertMouseY",false);
	mbToggleCrouch = mpInit->mpConfig->GetBool("Controls","ToggleCrouch",true);
}

//-----------------------------------------------------------------------

cButtonHandler::~cButtonHandler(void)
{
	
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cButtonHandler::ChangeState(eButtonHandlerState aState)
{
	mState = aState;
}

//-----------------------------------------------------------------------

void cButtonHandler::OnStart()
{
	mpPlayer = mpInit->mpPlayer;
}

//-----------------------------------------------------------------------

void cButtonHandler::OnPostSceneDraw()
{
	if(mpInit->mpGame->GetGraphics()->GetRenderer3D()->GetDebugFlags() & eRendererDebugFlag_LogRendering)
	{
		Log("-------------- STOP RENDERING LOG ------------------------\n");
		mpInit->mpGame->GetGraphics()->GetRenderer3D()->SetDebugFlags(0);
	}
}

//-----------------------------------------------------------------------

// The short way from one angle to another
static float InterpolateAngle(float afA, float afB, float afT)
{
	float fDist = fmod(afB - afA, k2Pif);
	if(fDist > kPif) fDist -= k2Pif;
	if(fDist < -kPif) fDist += k2Pif;
	return afA + fDist * afT;
}

/**
 * A frame drawn between two logic steps shows the camera between where it
 * was before and after the last step, which is behind the mouse by up to a
 * step. Looking around must not lag: the part of the last step's turn that
 * came from the mouse is shown in full, and the movement that has arrived
 * since then is added the way that step turned it into a rotation. Turns
 * that did not come from the mouse (scripts, head swing) are still blended.
 */
void cButtonHandler::OnInterpolate(float afT)
{
	cCamera3D *pCam = mpPlayer->GetCamera();
	if(pCam->IsInterpolated()==false) return;

	// Only if the last step read the mouse for looking.
	if(mlLookStep + 1 != mpInit->mpGame->GetLogicStepCount()) return;

	cVector2f vPending = mpInput->GetMouse()->GetPendingRelPosition();
	if(mlLookPlayerState != (int)mpPlayer->GetState()) vPending = cVector2f(0,0);

	float fYaw = InterpolateAngle(pCam->GetInterpolationPrevYaw(), pCam->GetLogicYaw() - mvLookDelta.x, afT)
					+ mvLookDelta.x + vPending.x * mvLookPerRel.x;
	float fPitch = InterpolateAngle(pCam->GetInterpolationPrevPitch(), pCam->GetLogicPitch() - mvLookDelta.y, afT)
					+ mvLookDelta.y + vPending.y * mvLookPerRel.y;

	pCam->SetYaw(fYaw);
	pCam->SetPitch(fPitch);
}

//-----------------------------------------------------------------------

cVector2f cButtonHandler::ReadMouseRel()
{
	cVector2f vRel = mpInput->GetMouse()->GetRelPosition();
	mvCursorRel += vRel;
	return vRel;
}

// Update() runs more than once in a logic step; the first run starts the
// measurement of how far the step's mouse movement moves the cursors.
void cButtonHandler::BeginCursorStep()
{
	unsigned int lStep = mpInit->mpGame->GetLogicStepCount();
	if(mlCursorStep == lStep) return;

	mlCursorStep = lStep;
	mvCursorRel = cVector2f(0,0);
	mvCursorStart[0] = mpInit->mpMainMenu->GetMousePos();
	mvCursorStart[1] = mpPlayer->GetCrossHairPos();
}

void cButtonHandler::EndCursorStep()
{
	cVector2f vEnd[2] = { mpInit->mpMainMenu->GetMousePos(), mpPlayer->GetCrossHairPos() };

	for(int i=0; i<2; ++i)
	{
		cVector2f vMove = vEnd[i] - mvCursorStart[i];

		// Cursor movement per unit of mouse movement. An axis the mouse did
		// not move on keeps its value. Anything far from the sensitivity
		// setting is a cursor that was put somewhere, not moved.
		float fMax = mfMouseSensitivity * 4.0f;
		if(mvCursorRel.x != 0)
		{
			float fPerRel = vMove.x / mvCursorRel.x;
			mvCursorPerRel[i].x = (fPerRel >= 0 && fPerRel <= fMax) ? fPerRel : 0;
		}
		if(mvCursorRel.y != 0)
		{
			float fPerRel = vMove.y / mvCursorRel.y;
			mvCursorPerRel[i].y = (fPerRel >= 0 && fPerRel <= fMax) ? fPerRel : 0;
		}
	}
}

cVector2f cButtonHandler::GetCursorDrawOffset(bool abMenu)
{
	if(mpInit->mpGame->GetLimitFPS()) return cVector2f(0,0);

	// Only if the last step measured it.
	if(mlCursorStep + 1 != mpInit->mpGame->GetLogicStepCount()) return cVector2f(0,0);

	return mpInput->GetMouse()->GetPendingRelPosition() * mvCursorPerRel[abMenu ? 0 : 1];
}

//-----------------------------------------------------------------------

void cButtonHandler::Update(float afTimeStep)
{
	BeginCursorStep();
	struct cCursorStepEnd
	{
		cButtonHandler *mpHandler;
		~cCursorStepEnd(){ mpHandler->EndCursorStep(); }
	} cursorStepEnd = { this };

	static bool bLockState = true;
	///////////////////////////////////
	// GLOBAL Key Strokes
	///////////////////////////////////
	if(mpInput->BecameTriggerd("QuitGame"))
	{
		mpInit->mpGame->Exit();
	}
	if(mpInput->BecameTriggerd("Screenshot"))
	{
		int lCount = 1;
		tString sFileName = "screenshot000.bmp";
		while(FileExists(cString::To16Char(sFileName)))
		{
			sFileName = "screenshot";
            if(lCount < 10)sFileName+= "00";
			else if(lCount < 100)sFileName+= "0";
			sFileName += cString::ToString(lCount);
			sFileName += ".bmp";
			++lCount;
		}

		mpInit->mpGame->GetGraphics()->GetLowLevel()->SaveScreenToBMP(sFileName);
	}
	if(mpInput->BecameTriggerd("LockInput"))
	{
#ifndef WIN32
		bLockState = !bLockState;
		mpInit->mpGame->GetInput()->GetLowLevel()->LockInput(bLockState);
#endif
	}
	///////////////////////////////////
	// DEMO END TEXT
	///////////////////////////////////
	if(mState == eButtonHandlerState_DemoEndText)
	{
		if(	mpInput->BecameTriggerd("Escape"))	
			mpInit->mpDemoEndText->OnButtonDown();
		if(mpInput->BecameTriggerd("LeftClick"))
			mpInit->mpDemoEndText->OnMouseDown(eMButton_Left);
		if(mpInput->BecameTriggerd("RightClick"))
			mpInit->mpDemoEndText->OnMouseDown(eMButton_Right);
	}
	///////////////////////////////////
	// CREDITS STATE
	///////////////////////////////////
	else if(mState == eButtonHandlerState_Credits)
	{
		if(	mpInput->BecameTriggerd("Escape"))	
			mpInit->mpCredits->OnButtonDown();
		if(mpInput->BecameTriggerd("LeftClick"))
			mpInit->mpCredits->OnMouseDown(eMButton_Left);
		if(mpInput->BecameTriggerd("RightClick"))
			mpInit->mpCredits->OnMouseDown(eMButton_Right);
	}
	///////////////////////////////////
	// PRE MENU STATE
	///////////////////////////////////
	else if(mState == eButtonHandlerState_PreMenu)
	{
		if(	mpInput->BecameTriggerd("Escape"))	
			mpInit->mpPreMenu->OnButtonDown();
		if(mpInput->BecameTriggerd("LeftClick"))
			mpInit->mpPreMenu->OnMouseDown(eMButton_Left);
		if(mpInput->BecameTriggerd("RightClick"))
			mpInit->mpPreMenu->OnMouseDown(eMButton_Right);
	}
	///////////////////////////////////
	// MAP LOAD TEXT STATE
	///////////////////////////////////
	else if(mState == eButtonHandlerState_MapLoadText)
	{
		if(	mpInput->BecameTriggerd("Escape") ||
			mpInput->BecameTriggerd("RightClick") ||
			mpInput->BecameTriggerd("LeftClick"))
		{
			mpInit->mpMapLoadText->SetActive(false);
		}
	}
	///////////////////////////////////
	// MAIN MENU BUTTON STATE
	///////////////////////////////////
	else if(mState == eButtonHandlerState_MainMenu)
	{
		if(mpInput->BecameTriggerd("Escape"))
		{
			mpInit->mpMainMenu->Exit();
		}

		if(	mpInput->BecameTriggerd("RightClick") ||
			(mpInit->mbHasHaptics && mpInput->BecameTriggerd("MouseClickRight")) )
		{
			mpInit->mpMainMenu->OnMouseDown(eMButton_Right);
			mpInput->BecameTriggerd("Examine");
		}
		if(mpInput->WasTriggerd("RightClick"))
		{
			mpInit->mpMainMenu->OnMouseUp(eMButton_Right);
		}
		if(mpInput->DoubleTriggerd("RightClick",0.15f))
		{
			mpInit->mpMainMenu->OnMouseDoubleClick(eMButton_Right);
		}

		if(	mpInput->BecameTriggerd("LeftClick") || 
			(mpInit->mbHasHaptics && mpInput->BecameTriggerd("MouseClick")) )
		{
			mpInit->mpMainMenu->OnMouseDown(eMButton_Left);
			mpInput->BecameTriggerd("Interact");
		}
		if(mpInput->WasTriggerd("LeftClick"))
		{
			mpInit->mpMainMenu->OnMouseUp(eMButton_Left);
		}
		if(mpInput->DoubleTriggerd("LeftClick",0.15f))
		{
			mpInit->mpMainMenu->OnMouseDoubleClick(eMButton_Left);
		}

		if(mpInit->mbHasHaptics)
		{
			mpInit->mpMainMenu->AddMousePos(mpLowLevelHaptic->GetRelativeVirtualMousePos() * mfMouseSensitivity);

			cVector2f vRel = ReadMouseRel();
			mpInit->mpMainMenu->AddMousePos(vRel * mfMouseSensitivity);
		}
		else
		{
			/// Mouse Movement
			cVector2f vRel = ReadMouseRel();
			mpInit->mpMainMenu->AddMousePos(vRel * mfMouseSensitivity);
		}
		
	}
	///////////////////////////////////
	// INTRO BUTTON STATE
	///////////////////////////////////
	else if(mState == eButtonHandlerState_Intro)
	{
		if(mpInput->BecameTriggerd("Escape"))
		{
			mpInit->mpIntroStory->Exit();
		}
	}
	///////////////////////////////////
	// GAME BUTTON STATE
	///////////////////////////////////
	else if(mState == eButtonHandlerState_Game)
	{
		///////////////////////////////////////
		// Global ////////////////////
		/*if(mpInput->BecameTriggerd("ResetGame"))
		{
			mpInit->ResetGame(true);
			mpInit->mpMapHandler->Load(	mpInit->msStartMap,mpInit->msStartLink);
		}*/
		if(mpInit->mbAllowQuickSave)
		{
			if(mpInput->BecameTriggerd("SaveGame"))
			{
				mpInit->mpSaveHandler->AutoSave(_W("auto"),5);
			}
			if(mpInput->BecameTriggerd("LoadGame"))
			{
				mpInit->mpSaveHandler->AutoLoad(_W("auto"));
			}
		}
		if(mpInput->BecameTriggerd("PrintLog"))
		{
			Log("-------------- START RENDERING LOG ------------------------\n");
			mpInit->mpGame->GetGraphics()->GetRenderer3D()->SetDebugFlags(eRendererDebugFlag_LogRendering);
		}
		//Check if no jump is pressed always.
		bool bPlayerStateIsActive = false;
		///////////////////////////////////////
		// Death menu ////////////////////
		if(mpInit->mpDeathMenu->IsActive())
		{
			if(mpInput->BecameTriggerd("Escape"))
			{
				mpInit->mpGame->GetUpdater()->Reset();
				mpInit->mpMainMenu->SetActive(true);
			}

			if(mpInput->BecameTriggerd("RightClick"))
			{
				mpInit->mpDeathMenu->OnMouseDown(eMButton_Right);
				mpInput->BecameTriggerd("Examine");
			}

			if(mpInput->BecameTriggerd("LeftClick"))
			{
				mpInit->mpDeathMenu->OnMouseDown(eMButton_Left);
				mpInput->BecameTriggerd("Interact");
			}
			if(mpInput->WasTriggerd("LeftClick"))
			{
				mpInit->mpDeathMenu->OnMouseUp(eMButton_Left);
			}

			if(mpInit->mbHasHaptics)
			{
				mpInit->mpDeathMenu->AddMousePos(mpLowLevelHaptic->GetRelativeVirtualMousePos() * mfMouseSensitivity);
			}
			else
			{
				/// Mouse Movement
				cVector2f vRel = ReadMouseRel();
				mpInit->mpDeathMenu->AddMousePos(vRel * mfMouseSensitivity);
			}
		}
		///////////////////////////////////////
		// Death ////////////////////
		else if(mpPlayer->IsDead())
		{
			if(mpInput->BecameTriggerd("Escape"))
			{
				mpInit->mpMainMenu->SetActive(true);
			}
		}
		///////////////////////////////////////
		// Numerical panel ////////////////////
		else if(mpInit->mpNumericalPanel->IsActive())
		{
			if(mpInput->BecameTriggerd("Inventory") || mpInput->BecameTriggerd("Escape"))
			{
				mpInit->mpNumericalPanel->OnExit();
			}
			if(mpInput->BecameTriggerd("RightClick"))
			{
				mpInit->mpNumericalPanel->OnExit();
			}

			if(mpInput->BecameTriggerd("LeftClick"))
			{
				mpInit->mpNumericalPanel->OnMouseDown(eMButton_Left);
				mpInput->BecameTriggerd("Interact");
			}
			if(mpInput->WasTriggerd("LeftClick"))
			{
				mpInit->mpNumericalPanel->OnMouseUp(eMButton_Left);
			}

			if(mpInit->mbHasHaptics)
			{
				mpInit->mpNumericalPanel->AddMousePos(mpLowLevelHaptic->GetRelativeVirtualMousePos() * mfMouseSensitivity);
			}
			else
			{
				/// Mouse Movement
				cVector2f vRel = ReadMouseRel();
				mpInit->mpNumericalPanel->AddMousePos(vRel * mfMouseSensitivity);
			}
		}
		///////////////////////////////////////
		// Notebook ////////////////////
		else if(mpInit->mpNotebook->IsActive())
		{
			if(mpInput->BecameTriggerd("Inventory") || mpInput->BecameTriggerd("Escape"))
			{
				mpInit->mpNotebook->OnExit();
			}
			
			if(mpInput->BecameTriggerd("LeftClick"))
			{
				mpInit->mpNotebook->OnMouseDown(eMButton_Left);

				mpInput->BecameTriggerd("Interact");
			}

			if(mpInput->BecameTriggerd("NoteBook"))
			{
				mpInit->mpNotebook->OnExit();
			}
			if(mpInput->BecameTriggerd("PersonalNotes"))
			{
				cStateMachine *pStateMachine = mpInit->mpNotebook->GetStateMachine();
				if(pStateMachine->CurrentState()->GetId() == eNotebookState_TaskList)
				{
					pStateMachine->ChangeState(eNotebookState_Front);
					mpInit->mpNotebook->OnExit();
				}
				else
				{
					pStateMachine->ChangeState(eNotebookState_TaskList);
				}
			}
			
			if(mpInit->mbHasHaptics)
			{
				mpInit->mpNotebook->AddMousePos(mpLowLevelHaptic->GetRelativeVirtualMousePos() * mfMouseSensitivity);
			}
			else
			{
				/// Mouse Movement
				cVector2f vRel = ReadMouseRel();
				mpInit->mpNotebook->AddMousePos(vRel * mfMouseSensitivity);
			}
		}
		///////////////////////////////////////
		// Inventory ////////////////////
		else if(mpInit->mpInventory->IsActive())
		{
			////////////////////////////
			//Normal Input
			if(mpInput->BecameTriggerd("Inventory") || mpInput->BecameTriggerd("Escape"))
			{
				mpInit->mpInventory->OnInventoryDown();
			}
			
			if(mpInput->BecameTriggerd("LeftClick"))
			{
				mpInit->mpInventory->OnMouseDown(eMButton_Left);

				mpInput->BecameTriggerd("Interact");
			}
			
			if(mpInput->DoubleTriggerd("LeftClick",0.2f))
			{
				mpInit->mpInventory->OnDoubleClick(eMButton_Left);
			}
			if(mpInput->WasTriggerd("LeftClick"))
			{
				mpInit->mpInventory->OnMouseUp(eMButton_Left);
			}

				
			if(mpInput->BecameTriggerd("RightClick"))
			{
				mpInit->mpInventory->OnMouseDown(eMButton_Right);
				
				mpInput->BecameTriggerd("Examine");
			}
			if(mpInput->WasTriggerd("RightClick"))
			{
				mpInit->mpInventory->OnMouseUp(eMButton_Right);
			}
			
			//////////////////////////////
			//Short cut keys
			if(mpInput->BecameTriggerd("One")) mpInit->mpInventory->OnShortcutDown(0);
			if(mpInput->BecameTriggerd("Two")) mpInit->mpInventory->OnShortcutDown(1);
			if(mpInput->BecameTriggerd("Three")) mpInit->mpInventory->OnShortcutDown(2);
			if(mpInput->BecameTriggerd("Four")) mpInit->mpInventory->OnShortcutDown(3);
			if(mpInput->BecameTriggerd("Five")) mpInit->mpInventory->OnShortcutDown(4);
			if(mpInput->BecameTriggerd("Six")) mpInit->mpInventory->OnShortcutDown(5);
			if(mpInput->BecameTriggerd("Seven")) mpInit->mpInventory->OnShortcutDown(6);
			if(mpInput->BecameTriggerd("Eight")) mpInit->mpInventory->OnShortcutDown(7);
			if(mpInput->BecameTriggerd("Nine")) mpInit->mpInventory->OnShortcutDown(8);
			
			if(mpInit->mbHasHaptics)
			{
				cVector2f vRel = mpLowLevelHaptic->GetRelativeVirtualMousePos();
				mpInit->mpInventory->AddMousePos(vRel * mfMouseSensitivity);
			}
			else
			{
				/// Mouse Movement
				cVector2f vRel = ReadMouseRel();
				mpInit->mpInventory->AddMousePos(vRel * mfMouseSensitivity);
			}
		}
		else 
		{
			bPlayerStateIsActive = true;

			if(mpInput->BecameTriggerd("Escape"))
			{
				mpInit->mpMainMenu->SetActive(true);
			}
			if(mpInput->BecameTriggerd("Hit"))
			{
				mpInit->mpPlayer->Damage(20,ePlayerDamageType_BloodSplash);
			}
			/*if(mpInput->BecameTriggerd("Log"))
			{
				mpInit->mpGame->GetPhysics()->SetDebugLog(!mpInit->mpGame->GetPhysics()->GetDebugLog());
			}*/
			if(mpInput->BecameTriggerd("Taunt"))
			{
				for(int i=0;i<10; ++i)
					mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui("gui_notebook_add_note1",
																		false,0.01f);
				/*cVector3f vPos = mpInit->mpPlayer->GetCharacterBody()->GetPosition();
				cSoundEntity *pSound = mpInit->mpGame->GetScene()->GetWorld3D()->CreateSoundEntity("Taunt","interact_homer",true);
                if(pSound)
				{
					pSound->SetPosition(vPos);
				}*/
			}
			
			if(mpPlayer->IsActive() || mpPlayer->GetState() == ePlayerState_Message)
			{
				if(mpPlayer->IsActive())
				{
					if(mpInput->BecameTriggerd("Inventory"))
					{
						mpPlayer->StartInventory();
					}

					if(mpInput->BecameTriggerd("NoteBook"))
					{
						mpInit->mpNotebook->SetActive(true);
					}
					if(mpInput->BecameTriggerd("PersonalNotes"))
					{
						mpInit->mpNotebook->SetActive(true);
						mpInit->mpNotebook->GetStateMachine()->ChangeState(eNotebookState_TaskList);
					}
	                
					if(mpInput->BecameTriggerd("Flashlight"))
					{
						mpPlayer->StartFlashLightButton();
					}

					if(mpInput->BecameTriggerd("GlowStick"))
					{
						mpPlayer->StartGlowStickButton();
					}

					///////////////////////////////////////
					// Player Movement ////////////////////
					if(mpInput->IsTriggerd("Forward"))
					{
						mpPlayer->MoveForwards(1,afTimeStep);
					}
					else if(mpInput->IsTriggerd("Backward"))
					{
						mpPlayer->MoveForwards(-1,afTimeStep);
					}
					else
					{
						mpPlayer->MoveForwards(0,afTimeStep);
					}


					if(mpInput->IsTriggerd("Left"))
					{
						mpPlayer->MoveSideways(-1,afTimeStep);
					}
					else if(mpInput->IsTriggerd("Right"))
					{
						mpPlayer->MoveSideways(1,afTimeStep);
					}
					else
					{
						mpPlayer->MoveSideways(0,afTimeStep);
					}

					if(mpInput->IsTriggerd("LeanLeft"))
					{
						mpPlayer->Lean(-1,afTimeStep);
					}
					else if(mpInput->IsTriggerd("LeanRight"))
					{
						mpPlayer->Lean(1,afTimeStep);
					}
					

					if(mpInput->BecameTriggerd("Jump"))
					{
						mpPlayer->Jump();
					}
					if(mpInput->IsTriggerd("Jump"))
					{
						mpPlayer->SetJumpButtonDown(true);
					}
					
					if(mpInput->BecameTriggerd("Run"))
					{
						mpPlayer->StartRun();
					}
					if(mpInput->WasTriggerd("Run"))
					{
						mpPlayer->StopRun();	
					}

					if(mpInput->BecameTriggerd("Crouch"))
					{
						mpPlayer->StartCrouch();
					}
					if(GetToggleCrouch())
					{
						if(mpInput->WasTriggerd("Crouch"))	mpPlayer->StopCrouch();	
					}
					else
					{
						if(mpInput->IsTriggerd("Crouch")==false) mpPlayer->StopCrouch();	
					}

					if(mpInput->BecameTriggerd("InteractMode"))
					{
						if(mpInit->mbHasHaptics==false)
						{
							mpPlayer->StartInteractMode();
						}
						else
						{
							//DO nothing for the time being.
						}
					}
					
					//Get the mouse pos and convert it to 0 - 1
					if(mpInit->mbHasHaptics==false)
					{
						cVector2f vRel = ReadMouseRel();
						cVector2f vRawRel = vRel;
						vRel /= mpLowLevelGraphics->GetVirtualSize();

						// The movement is in virtual units, 600 / window height
						// per mouse count, so the higher the window the slower
						// the player turned. Turn as fast as at the reference
						// height whatever the window's size.
						vRel *= mpLowLevelGraphics->GetScreenSize().y / kLookReferenceHeight;

						cCamera3D *pCam = mpPlayer->GetCamera();
						cVector2f vLookStart(pCam->GetYaw(), pCam->GetPitch());

						mpPlayer->AddYaw(vRel.x * mfMouseSensitivity);
						mpPlayer->AddPitch(vRel.y * mfMouseSensitivity);

						// Remember how far the mouse turned the camera in this
						// step, and by how much per unit of movement. This can
						// be reached more than once in a step.
						unsigned int lStep = mpInit->mpGame->GetLogicStepCount();
						if(mlLookStep != lStep)
						{
							mlLookStep = lStep;
							mvLookDelta = cVector2f(0,0);
						}
						if(mlLookPlayerState != (int)mpPlayer->GetState())
						{
							mlLookPlayerState = (int)mpPlayer->GetState();
							mvLookPerRel = cVector2f(0,0);
						}
						cVector2f vTurn = cVector2f(pCam->GetYaw(), pCam->GetPitch()) - vLookStart;
						mvLookDelta += vTurn;
						if(vRawRel.x != 0) mvLookPerRel.x = vTurn.x / vRawRel.x;
						if(vRawRel.y != 0) mvLookPerRel.y = vTurn.y / vRawRel.y;
					}
				}

				///////////////////////////////////////
				// Player Interaction /////////////////
				if(	mpInput->BecameTriggerd("Interact"))
				{
					mpPlayer->StartInteract();
					mpInput->BecameTriggerd("LeftClick");
				}
				if(	mpInput->WasTriggerd("Interact"))
				{
					mpPlayer->StopInteract();
				}
				if(	mpInput->BecameTriggerd("Examine"))
				{
					mpPlayer->StartExamine();
				}
				if(mpInput->WasTriggerd("Examine"))
				{
					mpPlayer->StopExamine();
				}
				if(mpInput->BecameTriggerd("Holster"))
				{
					mpPlayer->StartHolster();
				}

				if(mpPlayer->IsActive())
				{
					if(mpInput->BecameTriggerd("One")) mpPlayer->StartInventoryShortCut(0);
					if(mpInput->BecameTriggerd("Two")) mpPlayer->StartInventoryShortCut(1);
					if(mpInput->BecameTriggerd("Three")) mpPlayer->StartInventoryShortCut(2);
					if(mpInput->BecameTriggerd("Four")) mpPlayer->StartInventoryShortCut(3);
					if(mpInput->BecameTriggerd("Five")) mpPlayer->StartInventoryShortCut(4);
					if(mpInput->BecameTriggerd("Six")) mpPlayer->StartInventoryShortCut(5);
					if(mpInput->BecameTriggerd("Seven")) mpPlayer->StartInventoryShortCut(6);
					if(mpInput->BecameTriggerd("Eight")) mpPlayer->StartInventoryShortCut(7);
					if(mpInput->BecameTriggerd("Nine")) mpPlayer->StartInventoryShortCut(8);
				}
			}
		}
		if(mpInput->IsTriggerd("Jump")==false || bPlayerStateIsActive==false)
		{
			mpPlayer->SetJumpButtonDown(false);
		}
	}
}

//-----------------------------------------------------------------------

void cButtonHandler::Reset()
{

}

//-----------------------------------------------------------------------

void cButtonHandler::OnExit()
{
	// SAVE SETTINGS
	Log("  Saving to config\n");
	mpInit->mpConfig->SetFloat("Controls","MouseSensitivity",mfMouseSensitivity);
	mpInit->mpConfig->SetBool("Controls","InvertMouseY",mbInvertMouseY);
	mpInit->mpConfig->SetBool("Controls","ToggleCrouch",mbToggleCrouch);

	// SAVE KEYS
	Log("  Saving keys\n");
	for(int i=0; i< mlNumOfActions; ++i)
	{
		//Log(" Action %s\n",gvDefaultActions[i].msName.c_str());
		
		iAction *pAction = mpInput->GetAction(gvDefaultActions[i].msName);
		tString sType="", sVal="";
		TypeAndValFromAction(pAction,&sType,&sVal);

		//Log(" type %s val: %s\n",sType.c_str(),sVal.c_str());
		
		mpInit->mpConfig->SetString("Keys",gvDefaultActions[i].msName+"_Type",sType);
		mpInit->mpConfig->SetString("Keys",gvDefaultActions[i].msName+"_Val",sVal);
	}
}

//-----------------------------------------------------------------------

void cButtonHandler::SetDefaultKeys()
{
	cButtonHandlerAction *pBHAction = &gvDefaultActions[0];
	while(pBHAction->msName != "")
	{
		tString sName = pBHAction->msName;
		tString sType = pBHAction->msType;
		tString sVal = cString::ToString(pBHAction->mlVal);
		
		iAction *pAction = ActionFromTypeAndVal(sName, sType, sVal);
		
		if(pAction)
		{
			mpInput->DestroyAction(sName);
			mpInput->AddAction(pAction);
		}
		else
		{
			Warning("Couldn't create action from '%s' and %d\n",pBHAction->msType.c_str(),
																	pBHAction->mlVal);
		}

		++pBHAction;
	}
}

//-----------------------------------------------------------------------

tString cButtonHandler::GetActionName(const tString &asInputName,const tString &asSkipAction)
{
	cButtonHandlerAction *pBHAction = &gvDefaultActions[0];
	while(pBHAction->msName != "")
	{
		tString sName = pBHAction->msName;
		tString sType = pBHAction->msType;
		tString sVal = cString::ToString(pBHAction->mlVal);

		iAction *pAction = mpInput->GetAction(sName);
		
		if(asSkipAction != sName && pAction && pAction->GetInputName() == asInputName) return sName;

		//If at last player action, skip the rest.
		if(sName == gsLastPlayerAction) return "";

		++pBHAction;
	}

	return "";
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

iAction* cButtonHandler::ActionFromTypeAndVal(const tString& asName,const tString& asType, const tString& asVal)
{
	//Log("Action %s from %s\n",asName.c_str(),asType.c_str());

	if(asType == "Keyboard")
	{
		return hplNew( cActionKeyboard, (asName,mpInit->mpGame->GetInput(),(eKey)cString::ToInt(asVal.c_str(),0)) );
	}
	else if(asType == "MouseButton" || asType == "HapticDeviceButton")
	{
		if(mpInit->mbHasHaptics && asName != "MouseClick")
		{
			int lNum = cString::ToInt(asVal.c_str(),0);
			if(lNum==2)lNum = 2; 
			else if(lNum==1)lNum = 1;
			return hplNew( cActionHaptic, (asName,mpInit->mpGame->GetHaptic(),lNum) );
		}
		else
		{
			return hplNew( cActionMouseButton, (asName,mpInit->mpGame->GetInput(),(eMButton)cString::ToInt(asVal.c_str(),0)) );
		}
	}

	return nullptr;
}

//-----------------------------------------------------------------------

void cButtonHandler::TypeAndValFromAction(iAction *apAction, tString *apType, tString *apVal)
{
	if(apAction)
	{
		*apType = apAction->GetInputType();

		if(apAction->GetInputType() == "Keyboard")
		{
			cActionKeyboard *pKeyAction = static_cast<cActionKeyboard*>(apAction);
			*apVal = cString::ToString((int)pKeyAction->GetKey() | (int)pKeyAction->GetModifier());
		}
		else if(apAction->GetInputType() == "MouseButton" ||
				apAction->GetInputType() == "HapticDeviceButton")
		{
			if(mpInit->mbHasHaptics && apAction->GetName() != "MouseClick")
			{
				cActionHaptic *pHapticAction = static_cast<cActionHaptic*>(apAction);
				*apVal = cString::ToString(pHapticAction->GetButton());
				if(*apVal=="2")*apVal = "2"; 
				else if(*apVal=="1")*apVal = "1";
			}
			else
			{
				cActionMouseButton *pMouseAction = static_cast<cActionMouseButton*>(apAction);
				*apVal = cString::ToString((int)pMouseAction->GetButton());
			}
		}
	}
	else
	{
		*apVal = "";
		*apType = "";
	}
}

//-----------------------------------------------------------------------
