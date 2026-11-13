// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef GAME_DEATH_MENU_H
#define GAME_DEATH_MENU_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

using namespace hpl;

class cInit;

class cDeathMenuButton
{
public:
	cDeathMenuButton(cInit *apInit, cVector2f avPos, const tWString& asText);
	virtual ~cDeathMenuButton();

	void OnUpdate(float afTimeStep);

	void OnDraw();

	virtual void OnMouseDown()=0;
	virtual void OnMouseUp()=0;
	
	void OnMouseOver(bool abOver);

	const cRect2f& GetRect(){return mRect;}

protected:
	float mfAlpha;
	
	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;
	
	cVector3f mvPosition;
	cRect2f mRect;

	iFontData *mpFont;

	cVector2f mvFontSize;
	tWString msText;

	bool mbOver;
};

class cDeathMenuButton_Continue : public cDeathMenuButton
{
public:
	cDeathMenuButton_Continue(cInit *apInit, cVector2f avPos, const tWString& asText) :
								cDeathMenuButton(apInit, avPos, asText){}						

	void OnMouseDown() override;
	void OnMouseUp() override;
};

class cDeathMenuButton_BackToMain : public cDeathMenuButton
{
public:
	cDeathMenuButton_BackToMain(cInit *apInit, cVector2f avPos, const tWString& asText) :
								cDeathMenuButton(apInit, avPos, asText){}						

	void OnMouseDown() override;
	void OnMouseUp() override;
};

typedef std::list<cDeathMenuButton*> tDeathMenuButtonList;
typedef tDeathMenuButtonList::iterator tDeathMenuButtonListIt;

class cDeathMenu : public iUpdateable
{
	friend class cDeathMenuButton;
public:
	cDeathMenu(cInit *apInit);
	~cDeathMenu() override;

	void Reset() override;

	void OnDraw() override;

	void Update(float afTimeStep) override;

	void SetMousePos(const cVector2f &avPos);
	void AddMousePos(const cVector2f &avRel);
	cVector2f GetMousePos(){ return mvMousePos;}

	void OnMouseDown(eMButton aButton);
	void OnMouseUp(eMButton aButton);

	void SetActive(bool abActive);
	bool IsActive(){ return mbActive;}

	void OnExit() override;
private:
	cInit *mpInit;
	cGraphicsDrawer *mpDrawer;

	cGfxObject *mpGfxBackground;

	bool mbMouseIsDown;

	bool mbActive;
	float mfAlpha;

	iFontData *mpFont;
	
	cVector2f mvMousePos;

	eCrossHairState mLastCrossHairState;

	tDeathMenuButtonList mlstButtons;
};

#endif // GAME_DEATH_MENU_H
