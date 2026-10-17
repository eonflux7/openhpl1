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

//---------------------------------------

class cGameMessageHandler : public iUpdateable
{
friend class cGameMessage;
public:
	cGameMessageHandler(cInit *apInit);
	~cGameMessageHandler() override;

	void Add(const tWString& asText);
	void ShowNext();

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
