// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_GAME_MESSAGE_HANDLER_H
#define GAME_GAME_MESSAGE_HANDLER_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

using namespace hpl;

class cInit;

class cGameMessageHandler;
class cGameMessage
{
public:
	cGameMessage(const tWString &asText, cGameMessageHandler *apMessageHandler);

	void Update(float afTimeStep);
	void Draw(iFontData *apFont);

	bool mbActive;

	tWString msText;
	float mfTextAlpha;
	float mfTextAlphaRatePerSecond;

	cGameMessageHandler *mpMessageHandler;
};

typedef std::list<cGameMessage*> tGameMessageList;
typedef tGameMessageList::iterator tGameMessageListIt;

class cGameMessageHandler : public iUpdateable
{
friend class cGameMessage;
public:
	cGameMessageHandler(cInit *apInit);
	~cGameMessageHandler() override;

	void Add(const tWString& asText);
	void ShowNext();
	void ClearMessages();

	void Update(float afTimeStep) override;
	void OnDraw() override;
	void Reset() override;

	void OnWorldLoad();
	void OnWorldExit();

	bool HasMessage(){ return !mlstMessages.empty();}

	void SetOnMessagesOverCallback(const tString& asFunction);

	void SetDepthOfFieldUsed(bool abUsed){mbDepthOfFieldUsed = abUsed;}

	void SetBlackText(bool abBlackText){ mbBlackText = abBlackText;}
	
private:
	cInit* mpInit;

	iFontData *mpFont;
	ePlayerState mLastState;
	tString msOverCallback;

	bool mbDepthOfFieldUsed;

	bool mbBlackText;

	tGameMessageList mlstMessages;
};

#endif // GAME_GAME_MESSAGE_HANDLER_H
