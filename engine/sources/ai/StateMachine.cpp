// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#include "ai/StateMachine.h"

#include "system/LowLevelSystem.h"


namespace hpl {

	// STATE

	iAIState::iAIState()
	{
		mfUpdateElapsedSeconds = 0;
	}

	void iAIState::Sleep(float afTime)
	{

	}

	void iAIState::Update(float afTime)
	{
		//Update according to the time step
		mfUpdateElapsedSeconds	+= afTime;
		while(mfUpdateElapsedSeconds >= mfUpdateStep)
		{
			OnUpdate(mfUpdateStep);
			mfUpdateElapsedSeconds -= mfUpdateStep;
		}
	}

	cStateMachine::cStateMachine()
	{
		mbActive = true;

		mpCurrentState = nullptr;
	}

	cStateMachine::~cStateMachine()
	{
		STLMapDeleteAll(m_mapStates);
	}

	void cStateMachine::Update(float afTime)
	{
		if(mbActive== false || mpCurrentState==nullptr) return;

		mpCurrentState->Update(afTime);
	}

	void cStateMachine::AddState(iAIState *apState, const tString& asName, int alId, float afUpdateStep)
	{
		apState->SetStateMachine(this);
		apState->mlId = alId;
		apState->msName = asName;
		apState->mfUpdateStep = afUpdateStep;

		if(m_mapStates.empty()) mpCurrentState = apState;

		m_mapStates.insert(tAIStateMap::value_type(alId, apState));
	}

	void cStateMachine::ChangeState(int alId)
	{
		if(alId == mpCurrentState->GetId()) return;

		iAIState *pState = GetState(alId);
		if(pState==nullptr){
			Warning("State %d does not exist!\n",alId); return;
		}

		if(mpCurrentState) mpCurrentState->OnLeaveState(pState->GetId());
		pState->OnEnterState(mpCurrentState==nullptr ? -1 : mpCurrentState->GetId());

		mpCurrentState = pState;
	}

	iAIState* cStateMachine::GetState(int alId)
	{
		tAIStateMapIt it = m_mapStates.find(alId);
		if(it == m_mapStates.end()) return nullptr;

		return it->second;
	}

	iAIState* cStateMachine::CurrentState()
	{
		return mpCurrentState;
	}
}
