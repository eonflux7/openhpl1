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
#ifndef GAME_RADIO_HANDLER_H
#define GAME_RADIO_HANDLER_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

using namespace hpl;

class cInit;
class iGameEntity;

class cRadioMessage
{
public:
	cRadioMessage(const tWString &asText, const tString &asSound);

	void Update(float afTimeStep);
	
	tWString msText;
	tString msSound;

	iSoundChannel *mpChannel;

	// Black Plague: a message heard from a place. Full volume inside the least
	// distance, none beyond the greatest.
	iGameEntity *mpEntity;
	float mfFullVolumeDistance;
	float mfSilentDistance;
};

typedef std::list<cRadioMessage*> tRadioMessageList;
typedef tRadioMessageList::iterator tRadioMessageListIt;

//---------------------------------------

class cRadioHandler : public iUpdateable
{
public:
	cRadioHandler(cInit *apInit);
	~cRadioHandler() override;

	void Add(const tWString& asText,const tString& asSound);
	void Add(const tWString& asText,const tString& asSound, iGameEntity *apEntity,
			float afFullVolumeDistance, float afSilentDistance);

	/**
	 * Black Plague: drops the waiting messages and fades out the one playing.
	 * \param afFadeSpeed volume lost per second
	 */
	void StopAll(float afFadeSpeed);

	void OnWorldExit();
	
	void Update(float afTimeStep) override;
	void OnDraw() override;
	void Reset() override;

	// abCallbackInGui: the function is in the script of the open computer terminal
	void SetOnEndCallback(const tString &asFunction, bool abCallbackInGui=false){ msOnEndCallback = asFunction; mbOnEndCallbackInGui = abCallbackInGui;}
	tString GetOnEndCallback(){ return msOnEndCallback;}

	bool IsActive();
	bool IsShowingText();

	/**
	 * Black Plague: cuts the message that is heard short and goes on to the next
	 */
	void NextMessage();

private:
	void StartNextMessage();
	void RunOnEndCallback();

	float GetVolumeAtPlayer(cRadioMessage *apMessage);

	cInit* mpInit;
	cSoundHandler *mpSoundHandler;
	iFontData *mpFont;

	cRadioMessage *mpCurrentMessage;

	tWString msCurrentText;
	tWString msPrevText;

	tString msOnEndCallback;
	bool mbOnEndCallbackInGui;

	float mfTextCrossfadeAlpha;
	bool mbShowText;
	
	tRadioMessageList mlstQueuedMessages;
};

#endif // GAME_RADIO_HANDLER_H
