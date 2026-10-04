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
#ifndef GAME_GAME_GUI_H
#define GAME_GAME_GUI_H

#include "core/StdAfx.h"

#include "core/GameTypes.h"

using namespace hpl;

class TiXmlElement;

class cInit;
class cGameGui;

//////////////////////////////////////////////////////////////////////////
// Black Plague's computer terminals: a screen described by a file in
// interfaces/ (.gui), built from the engine's widgets and run by a script
// of its own. Written after the game's binary.
//////////////////////////////////////////////////////////////////////////

//---------------------------------------

// The script function a widget calls for each kind of message.
class cGameGuiCallback
{
public:
	tString mvFunctions[eGuiMessage_LastEnum];
};

typedef std::map<iWidget*, cGameGuiCallback*> tGameGuiCallbackMap;
typedef tGameGuiCallbackMap::iterator tGameGuiCallbackMapIt;

typedef std::map<tString, cGuiGfxElement*> tGameGuiImageMap;
typedef tGameGuiImageMap::iterator tGameGuiImageMapIt;

typedef std::map<tString, cGuiSkinFont*> tGameGuiFontMap;
typedef tGameGuiFontMap::iterator tGameGuiFontMapIt;

//---------------------------------------

// A note opened on the screen, with a button that closes it.
class cGameGuiTextView : public iGuiPopUp
{
public:
	cGameGuiTextView(cInit *apInit, cGameGui *apGui, const tWString &asLabel,
					const tString &asTextCat, const tString &asTextEntry,
					const tString &asCallback, bool abNewNote);
	~cGameGuiTextView() override;

private:
	bool OnDraw(iWidget* apWidget, cGuiMessageData& aData);
	kGuiCalllbackDeclarationEnd(OnDraw);

	bool SliderMove(iWidget* apWidget, cGuiMessageData& aData);
	kGuiCalllbackDeclarationEnd(SliderMove);

	bool ButtonPress(iWidget* apWidget, cGuiMessageData& aData);
	kGuiCalllbackDeclarationEnd(ButtonPress);

	cInit *mpInit;
	cGameGui *mpGameGui;

	iWidget *mpPrevAttention;
	tString msCallback;
	bool mbNewNote;

	int mlFirstRow;
	int mlMaxRows;
	cGuiSkinFont *mpFont;
	tWStringVec mvRows;

	cGuiGfxElement *mpGfxBackground;

	cVector3f mvWindowPos;
	cVector2f mvWindowSize;
	cVector3f mvFramePos;
	cVector2f mvFrameSize;

	cWidgetWindow *mpWindow;
	cWidgetFrame *mpFrame;
	cWidgetSlider *mpSlider;
	cWidgetButton *mpButton;
};

//---------------------------------------

class cGameGui
{
public:
	cGameGui(cInit *apInit, const tString &asName);
	~cGameGui();

	bool LoadFromFile(const tString &asFile);

	void Start();
	void Update(float afTimeStep);
	void Exit();

	bool RunScriptCommand(const tString &asCommand);

	void AddCallback(const tString &asWidget, eGuiMessage aMessage, const tString &asFunc);

	void CreatePopUp(const tWString &asLabel, const tWString &asText,
					const tWString &asButton1, const tWString &asButton2,
					const tString &asCallback);
	void CreateTextViewer(const tWString &asLabel, const tString &asTextCat,
						const tString &asTextEntry, const tString &asCallback,
						bool abNewNote);

	cGuiGfxElement* GetImage(const tString &asName);
	cGuiSkinFont* GetFont(const tString &asName);

	// Gives the widget the font's face, size and colour. False if there is no such font.
	bool SetWidgetFont(iWidget *apWidget, const tString &asFont);

	cGuiSet* GetSet(){ return mpSet;}
	bool IsFullScreen(){ return mbFullScreen;}

	static eGuiMessage StringToGuiMessage(const tString &asType);
	static tString GuiMessageToString(eGuiMessage aMessage);

	bool mbDestroyMe;

private:
	void LoadFontsFromElement(TiXmlElement *apElement);
	void LoadImagesFromElement(TiXmlElement *apElement);
	void LoadWidgetsFromElement(TiXmlElement *apElement, iWidget *apParent);

	bool CallbackFunc(iWidget* apWidget, cGuiMessageData& aData);
	kGuiCalllbackDeclarationEnd(CallbackFunc);

	bool PopUpCallback(iWidget* apWidget, cGuiMessageData& aData);
	kGuiCalllbackDeclarationEnd(PopUpCallback);

	cInit *mpInit;
	cGui *mpGui;

	bool mbFullScreen;

	tGameGuiCallbackMap m_mapCallbacks;
	tGameGuiImageMap m_mapImages;
	tGameGuiFontMap m_mapFonts;

	tString msName;
	tString msPopUpCallback;

	cGuiSkin *mpSkin;
	cGuiSet *mpSet;
	iScript *mpScript;
};

//---------------------------------------

class cGameGuiHandler : public iUpdateable
{
public:
	cGameGuiHandler(cInit *apInit);
	~cGameGuiHandler() override;

	void OnStart() override;
	void Update(float afTimeStep) override;
	void Reset() override;
	void OnDraw() override;

	void OpenScreenGui(const tString &asFile);
	// abLater is for a screen that closes itself from its own script: it is
	// then taken down at the next update.
	void CloseScreenGui(bool abLater);

	bool HasOpenScreenGui(){ return mpScreenGui != nullptr;}
	cGameGui* GetScreenGui(){ return mpScreenGui;}

	// The pointer, moved by the mouse over the screen's 800 x 600.
	void AddMousePos(const cVector2f &avRel);

	// Subtitles and radio messages go on top of an open screen.
	void DrawText(iFontData *apFont, const tWString &asText, float afAlpha);

private:
	void DestroyScreenGui();

	cInit *mpInit;
	cGui *mpGui;

	cGraphicsDrawer *mpDrawer;
	cGfxObject *mpBlackGfx;

	cGuiSet *mpScanSet;
	cGuiGfxElement *mpScanGfx;

	cGameGui *mpScreenGui;

	cVector2f mvMousePos;
};

//---------------------------------------

#endif // GAME_GAME_GUI_H
