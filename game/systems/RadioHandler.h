// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
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
