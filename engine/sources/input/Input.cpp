// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "input/Input.h"
#include "system/LowLevelSystem.h"
#include "input/Mouse.h"
#include "input/Keyboard.h"
#include "input/LowLevelInput.h"
#include "input/Action.h"
#include "input/ActionKeyboard.h"
#include "input/ActionMouseButton.h"


namespace hpl
{
	cInput::cInput(iLowLevelInput *apLowLevelInput) : iUpdateable("HPL_Input")
	{
		mpLowLevelInput = apLowLevelInput;

		mpKeyboard = mpLowLevelInput->CreateKeyboard();
		mpMouse = mpLowLevelInput->CreateMouse();

		mlstInputDevices.push_back(mpMouse);
		mlstInputDevices.push_back(mpKeyboard);
	}

	cInput::~cInput()
	{
		Log("Exiting Input Module\n");
		Log("--------------------------------------------------------\n");

		STLMapDeleteAll(m_mapActions);

		if(mpKeyboard)hplDelete(mpKeyboard);
		if(mpMouse)hplDelete(mpMouse);

		Log("--------------------------------------------------------\n\n");
	}

	void cInput::Update(float afTimeStep)
	{
		mpLowLevelInput->BeginInputUpdate();

		for(tInputDeviceListIt it = mlstInputDevices.begin(); it!= mlstInputDevices.end();++it)
		{
			(*it)->Update();
		}

		mpLowLevelInput->EndInputUpdate();

		for(tActionMapIt it = m_mapActions.begin(); it!= m_mapActions.end();++it)
		{
			it->second->Update(afTimeStep);
		}
	}

	void cInput::AddAction(iAction *apAction)
	{
		tString sName = apAction->GetName();
		tActionMap::value_type val = tActionMap::value_type(sName,apAction);
		m_mapActions.insert(val);
	}

	bool cInput::IsTriggered(tString asName)
	{
		iAction *pAction = GetAction(asName);
		if(pAction==nullptr){return false;}//Log("doesn't exist!!!");return false;}

		return pAction->IsTriggered();
	}

	bool cInput::WasTriggered(tString asName)
	{
		iAction *pAction = GetAction(asName);
		if(pAction==nullptr)return false;

		return pAction->WasTriggered();
	}

	bool cInput::BecameTriggered(tString asName)
	{
		iAction *pAction = GetAction(asName);
		if(pAction==nullptr)return false;

		return pAction->BecameTriggered();
	}

	bool cInput::DoubleTriggered(tString asName, float afMaxIntervalSeconds)
	{
		iAction *pAction = GetAction(asName);
		if(pAction==nullptr)return false;

		return pAction->DoubleTriggered(afMaxIntervalSeconds);
	}

	iKeyboard* cInput::GetKeyboard()
	{
		return mpKeyboard;
	}

	iMouse* cInput::GetMouse()
	{
		return mpMouse;
	}

	iAction* cInput::GetAction(tString asName)
	{
		tActionMapIt it = m_mapActions.find(asName);
		if(it==m_mapActions.end())return nullptr;

		return it->second;
	}

	void cInput::DestroyAction(tString asName)
	{
		iAction *pOldAction = GetAction(asName);
		if(pOldAction) hplDelete(pOldAction);
		m_mapActions.erase(asName);
	}

	bool cInput::CheckForInput()
	{
		//Keyboard
		for(int i=0; i< eKey_LastEnum; ++i)
		{
			if(mpKeyboard->KeyIsDown((eKey)i)) return true;
		}

		//Mouse
		for(int i=0; i< eMButton_LastEnum; ++i)
		{
			if(mpMouse->ButtonIsDown((eMButton)i)) return true;
		}

		return false;
	}

	iAction* cInput::InputToAction(const tString &asName)
	{
		iAction *pAction=nullptr;

		//Keyboard
		for(int i=0; i< eKey_LastEnum; ++i)
		{
			if(mpKeyboard->KeyIsDown((eKey)i))
			{
				pAction = hplNew( cActionKeyboard, (asName,this,(eKey)i));
				break;
			}
		}

		//Mouse
		if(pAction==nullptr)
		{
			for(int i=0; i< eMButton_LastEnum; ++i)
			{
				if(mpMouse->ButtonIsDown((eMButton)i))
				{
					pAction = hplNew( cActionMouseButton, (asName,this,(eMButton)i));
					break;
				}
			}
		}

		if(pAction)
		{
			iAction *pOldAction = GetAction(asName);
			if(pOldAction) hplDelete(pOldAction);

			m_mapActions.erase(asName);

			AddAction(pAction);
		}

		return pAction;
	}
}
