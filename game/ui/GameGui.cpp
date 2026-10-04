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
#include "core/StdAfx.h"
#include "ui/GameGui.h"

#include "core/Init.h"
#include "player/Player.h"
#include "systems/EffectHandler.h"
#include "core/GraphicsHelper.h"

//////////////////////////////////////////////////////////////////////////
// HELPERS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

static eGuiMaterial ToGuiMaterial(const char *apString)
{
	if(apString == nullptr) return eGuiMaterial_Alpha;

	tString sType = cString::ToLowerCase(apString);

	if(sType == "diffuse")		return eGuiMaterial_Diffuse;
	if(sType == "alpha")		return eGuiMaterial_Alpha;
	if(sType == "font_normal")	return eGuiMaterial_FontNormal;
	if(sType == "additive")		return eGuiMaterial_Additive;
	if(sType == "modulative")	return eGuiMaterial_Modulative;

	Warning("Material type '%s' does not exist!\n", apString);
	return eGuiMaterial_Alpha;
}

//-----------------------------------------------------------------------

static eWidgetSliderOrientation ToSliderOrientation(const char *apString)
{
	if(apString == nullptr) return eWidgetSliderOrientation_Horisontal;

	tString sType = cString::ToLowerCase(apString);
	if(sType == "horizontal") return eWidgetSliderOrientation_Horisontal;

	return eWidgetSliderOrientation_Vertical;
}

//-----------------------------------------------------------------------

static iWidget* CreateWidgetFromType(eWidgetType aType, const tString &asName, iWidget *apParent,
									cGuiSet *apSet, eWidgetSliderOrientation aOrientation)
{
	switch(aType)
	{
	case eWidgetType_Window:	return apSet->CreateWidgetWindow(0, 0, _W(""), apParent, asName);
	case eWidgetType_Button:	return apSet->CreateWidgetButton(0, 0, _W(""), apParent, asName);
	case eWidgetType_Frame:		return apSet->CreateWidgetFrame(0, 0, false, apParent, asName);
	case eWidgetType_Label:		return apSet->CreateWidgetLabel(0, 0, _W(""), apParent, asName);
	case eWidgetType_Slider:	return apSet->CreateWidgetSlider(aOrientation, 0, 0, 10, apParent, asName);
	case eWidgetType_TextBox:	return apSet->CreateWidgetTextBox(0, 0, _W(""), apParent, asName);
	case eWidgetType_CheckBox:	return apSet->CreateWidgetCheckBox(0, 0, _W(""), apParent, asName);
	case eWidgetType_Image:		return apSet->CreateWidgetImage("", 0, -1, eGuiMaterial_Alpha, false, apParent, asName);
	case eWidgetType_ListBox:	return apSet->CreateWidgetListBox(0, 0, apParent, asName);
	case eWidgetType_ComboBox:	return apSet->CreateWidgetComboBox(0, 0, _W(""), apParent, asName);
	default: break;
	}
	return nullptr;
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// TEXT VIEW
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cGameGuiTextView::cGameGuiTextView(cInit *apInit, cGameGui *apGui, const tWString &asLabel,
									const tString &asTextCat, const tString &asTextEntry,
									const tString &asCallback, bool abNewNote)
	: iGuiPopUp(apGui->GetSet())
{
	mpInit = apInit;
	mpGameGui = apGui;
	mbNewNote = abNewNote;
	msCallback = asCallback;

	mvWindowPos = cVector3f(150, 50, 15);
	mvWindowSize = cVector2f(500, 500);
	mvFramePos = cVector3f(30, 30, 1);
	mvFrameSize = cVector2f(440, 390);

	mpFont = mpSet->GetSkin()->GetFont(eGuiSkinFont_Default);

	float fRowHeight = mpFont->mvSize.y + 2;
	mlMaxRows = (int)(mvFrameSize.y / fRowHeight);
	mlFirstRow = 0;

	mpGfxBackground = mpSet->GetGui()->CreateGfxFilledRect(cColor(0,1), eGuiMaterial_Alpha);

	//////////////////////////
	// The text
	tWString sText = kTranslate(asTextCat, asTextEntry);
	mpFont->mpFont->GetWordWrapRows(mvFrameSize.x - 3 - 20, fRowHeight, mpFont->mvSize, sText, &mvRows);

	//////////////////////////
	// Window
	mpWindow = mpSet->CreateWidgetWindow(mvWindowPos, mvWindowSize, asLabel, nullptr, "");

	//////////////////////////
	// Button
	mpButton = mpSet->CreateWidgetButton(cVector3f(mvWindowSize.x*0.5f - 100, mvWindowSize.y - 50, 1),
										cVector2f(200, 30), kTranslate("GUI", "CloseDoc"),
										mpWindow, "");
	mpButton->AddCallback(eGuiMessage_ButtonPressed, this, kGuiCallback(ButtonPress));

	//////////////////////////
	// Frame
	mpFrame = mpSet->CreateWidgetFrame(mvFramePos, mvFrameSize, true, mpWindow, "");
	mpFrame->SetBackGroundColor(cColor(1,1));
	mpFrame->SetDrawBackground(false);
	mpFrame->AddCallback(eGuiMessage_OnDraw, this, kGuiCallback(OnDraw));

	//////////////////////////
	// Slider
	mpSlider = mpSet->CreateWidgetSlider(eWidgetSliderOrientation_Vertical,
										cVector3f(mvFramePos.x + mvFrameSize.x - 20, mvFramePos.y, 1.2f),
										cVector2f(20, mvFrameSize.y), 10, mpWindow, "");
	mpSlider->AddCallback(eGuiMessage_SliderMove, this, kGuiCallback(SliderMove));

	if(mlMaxRows < (int)mvRows.size())
	{
		mpSlider->SetBarValueSize(mlMaxRows);
		mpSlider->SetMaxValue((int)mvRows.size() - mlMaxRows);
	}
	else
	{
		mpSlider->SetMaxValue(0);
		mpSlider->SetBarValueSize(1);
	}

	mpPrevAttention = mpSet->GetAttentionWidget();
	mpSet->SetAttentionWidget(mpWindow);
}

//-----------------------------------------------------------------------

cGameGuiTextView::~cGameGuiTextView()
{
	mpSet->GetGui()->DestroyGfx(mpGfxBackground);

	// A set that is being taken down destroys its widgets itself
	if(mpSet->IsDestroyingSet()) return;

	mpSet->DestroyWidget(mpWindow);
	mpSet->DestroyWidget(mpButton);
	mpSet->DestroyWidget(mpFrame);
	mpSet->DestroyWidget(mpSlider);
}

//-----------------------------------------------------------------------

bool cGameGuiTextView::OnDraw(iWidget* apWidget, cGuiMessageData& aData)
{
	cColor col(1,1);

	mpSet->DrawGfx(mpGfxBackground, apWidget->GetGlobalPosition() + cVector3f(0,0,0.1f),
					mvFrameSize, col, eGuiMaterial_LastEnum);

	cVector3f vPos = apWidget->GetGlobalPosition() + cVector3f(3,2,0.2f);

	for(int i = mlFirstRow; i < (int)mvRows.size() && i < mlFirstRow + mlMaxRows; ++i)
	{
		mpSet->DrawFont(mvRows[i], mpFont->mpFont, vPos, mpFont->mvSize, mpFont->mColor,
						eFontAlign_Left, eGuiMaterial_FontNormal);
		vPos.y += mpFont->mvSize.y + 2;
	}

	return true;
}
kGuiCalllbackDeclaredFuncEnd(cGameGuiTextView, OnDraw)

//-----------------------------------------------------------------------

bool cGameGuiTextView::SliderMove(iWidget* apWidget, cGuiMessageData& aData)
{
	mlFirstRow = aData.mlVal;
	return true;
}
kGuiCalllbackDeclaredFuncEnd(cGameGuiTextView, SliderMove)

//-----------------------------------------------------------------------

bool cGameGuiTextView::ButtonPress(iWidget* apWidget, cGuiMessageData& aData)
{
	mpSet->SetAttentionWidget(mpPrevAttention);

	if(msCallback != "")
	{
		mpGameGui->RunScriptCommand(msCallback + "()");
	}

	if(mbNewNote)
	{
		mpInit->mpEffectHandler->GetSubTitle()->Add(kTranslate("GUI", "AddNoteToNotebook"), 3.5f, true);

		mpInit->mpGame->GetSound()->GetSoundHandler()->PlayGui("gui_notebook_add_note", false, 1,
																cVector3f(0,0,1), eSoundDest_Gui);
	}

	SelfDestruct();

	return true;
}
kGuiCalllbackDeclaredFuncEnd(cGameGuiTextView, ButtonPress)

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// GAME GUI
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cGameGui::cGameGui(cInit *apInit, const tString &asName)
{
	mpInit = apInit;
	mpGui = mpInit->mpGame->GetGui();

	msName = asName;

	mbFullScreen = false;
	mbDestroyMe = false;

	mpSkin = nullptr;
	mpSet = nullptr;
	mpScript = nullptr;

	msPopUpCallback = "";
}

//-----------------------------------------------------------------------

cGameGui::~cGameGui()
{
	if(mpSet) mpGui->DestroySet(mpSet);

	if(mpScript) mpInit->mpGame->GetResources()->GetScriptManager()->Destroy(mpScript);

	for(tGameGuiImageMapIt it = m_mapImages.begin(); it != m_mapImages.end(); ++it)
	{
		mpGui->DestroyGfx(it->second);
	}

	STLMapDeleteAll(m_mapFonts);
	STLMapDeleteAll(m_mapCallbacks);
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

bool cGameGui::LoadFromFile(const tString &asFile)
{
	tString sPath = mpInit->mpGame->GetResources()->GetFileSearcher()->GetFilePath(asFile);
	if(sPath == "")
	{
		Error("Could not find file '%s'!\n", asFile.c_str());
		return false;
	}

	TiXmlDocument xmlDoc(sPath.c_str());
	if(xmlDoc.LoadFile() == false)
	{
		Error("Couldn't load XML file '%s'!\n", sPath.c_str());
		return false;
	}

	TiXmlElement *pRootElem = xmlDoc.FirstChildElement();

	/////////////////////////////
	// Main
	TiXmlElement *pMainElem = pRootElem->FirstChildElement("Main");
	if(pMainElem == nullptr)
	{
		Error("Could not find main element in '%s'", asFile.c_str());
		return false;
	}

	tString sScriptFile = cString::ToString(pMainElem->Attribute("ScriptFile"), "");
	mpScript = mpInit->mpGame->GetResources()->GetScriptManager()->CreateScript(sScriptFile);

	tString sSkinFile = cString::ToString(pMainElem->Attribute("SkinFile"), "");
	mpSkin = mpGui->CreateSkin(sSkinFile);
	if(mpSkin == nullptr) return false;

	mbFullScreen = cString::ToBool(pMainElem->Attribute("IsFullScreen"), false);

	mpSet = mpGui->CreateSet(msName, mpSkin);

	/////////////////////////////
	// Fonts
	TiXmlElement *pFontsElem = pRootElem->FirstChildElement("Fonts");
	if(pFontsElem == nullptr)
	{
		Error("Could not find Fonts element in '%s'", asFile.c_str());
		return false;
	}
	LoadFontsFromElement(pFontsElem);

	/////////////////////////////
	// Images
	TiXmlElement *pImagesElem = pRootElem->FirstChildElement("Images");
	if(pImagesElem == nullptr)
	{
		Error("Could not find Images element in '%s'", asFile.c_str());
		return false;
	}
	LoadImagesFromElement(pImagesElem);

	/////////////////////////////
	// Widgets
	TiXmlElement *pWidgetsElem = pRootElem->FirstChildElement("Widgets");
	if(pWidgetsElem == nullptr)
	{
		Error("Could not find Widget element in '%s'", asFile.c_str());
		return false;
	}
	LoadWidgetsFromElement(pWidgetsElem, nullptr);

	RunScriptCommand("OnLoad()");

	return true;
}

//-----------------------------------------------------------------------

void cGameGui::Start()
{
	RunScriptCommand("OnStart()");
}

//-----------------------------------------------------------------------

void cGameGui::Update(float afTimeStep)
{
	RunScriptCommand("OnUpdate()");
}

//-----------------------------------------------------------------------

void cGameGui::Exit()
{
	RunScriptCommand("OnExit()");

	STLMapDeleteAll(m_mapCallbacks);
}

//-----------------------------------------------------------------------

bool cGameGui::RunScriptCommand(const tString &asCommand)
{
	if(mpScript == nullptr) return false;

	if(mpScript->Run(asCommand) == false)
	{
		Warning("Could not run command '%s' for gui!\n", asCommand.c_str());
		return false;
	}
	return true;
}

//-----------------------------------------------------------------------

void cGameGui::AddCallback(const tString &asWidget, eGuiMessage aMessage, const tString &asFunc)
{
	if(aMessage >= eGuiMessage_LastEnum) return;

	iWidget *pWidget = mpSet->GetWidgetFromName(asWidget);
	if(pWidget == nullptr)
	{
		Warning("Could not find widget '%s'\n", asWidget.c_str());
		return;
	}

	cGameGuiCallback *pCallback;
	tGameGuiCallbackMapIt it = m_mapCallbacks.find(pWidget);
	if(it == m_mapCallbacks.end())
	{
		pCallback = hplNew( cGameGuiCallback, () );
		m_mapCallbacks.insert(tGameGuiCallbackMap::value_type(pWidget, pCallback));
	}
	else
	{
		pCallback = it->second;
	}

	// The widget is told of each kind of message once
	if(pCallback->mvFunctions[aMessage] == "")
	{
		pWidget->AddCallback(aMessage, this, kGuiCallback(CallbackFunc));
	}

	pCallback->mvFunctions[aMessage] = asFunc;
}

//-----------------------------------------------------------------------

void cGameGui::CreatePopUp(const tWString &asLabel, const tWString &asText,
							const tWString &asButton1, const tWString &asButton2,
							const tString &asCallback)
{
	msPopUpCallback = asCallback;

	mpSet->CreatePopUpMessageBox(asLabel, asText, asButton1, asButton2,
								this, kGuiCallback(PopUpCallback));
}

//-----------------------------------------------------------------------

void cGameGui::CreateTextViewer(const tWString &asLabel, const tString &asTextCat,
								const tString &asTextEntry, const tString &asCallback,
								bool abNewNote)
{
	// Owned by the set once it closes itself
	hplNew( cGameGuiTextView, (mpInit, this, asLabel, asTextCat, asTextEntry, asCallback, abNewNote) );
}

//-----------------------------------------------------------------------

cGuiGfxElement* cGameGui::GetImage(const tString &asName)
{
	if(asName == "") return nullptr;

	tGameGuiImageMapIt it = m_mapImages.find(asName);
	if(it == m_mapImages.end())
	{
		Error("Could not find image '%s'\n", asName.c_str());
		return nullptr;
	}

	return it->second;
}

//-----------------------------------------------------------------------

cGuiSkinFont* cGameGui::GetFont(const tString &asName)
{
	if(asName == "") return nullptr;

	tGameGuiFontMapIt it = m_mapFonts.find(asName);
	if(it == m_mapFonts.end())
	{
		Error("Could not find font '%s'\n", asName.c_str());
		return nullptr;
	}

	return it->second;
}

//-----------------------------------------------------------------------

bool cGameGui::SetWidgetFont(iWidget *apWidget, const tString &asFont)
{
	cGuiSkinFont *pFont = GetFont(asFont);
	if(pFont == nullptr) return false;

	apWidget->SetDefaultFontColor(pFont->mColor);
	apWidget->SetDefaultFontSize(pFont->mvSize);
	apWidget->SetDefaultFontType(pFont->mpFont);

	return true;
}

//-----------------------------------------------------------------------

static const char* gvGuiMessageNames[eGuiMessage_LastEnum] = {
	"MouseMove", "MouseDown", "MouseUp", "MouseDoubleClick", "MouseEnter", "MouseLeave",
	"GotFocus", "LostFocus", "OnDraw", "ButtonPressed", "TextChange", "CheckChange",
	"KeyPress", "SliderMove", "SelectionChange"
};

eGuiMessage cGameGui::StringToGuiMessage(const tString &asType)
{
	tString sType = cString::ToLowerCase(asType);

	// The screens' scripts are given no way to these four
	if(sType != "mousemove" && sType != "gotfocus" && sType != "lostfocus" && sType != "ondraw")
	{
		for(int i=0; i<eGuiMessage_LastEnum; ++i)
		{
			if(sType == cString::ToLowerCase(gvGuiMessageNames[i])) return (eGuiMessage)i;
		}
	}

	Warning("Gui Message %s does not exist!\n", asType.c_str());
	return eGuiMessage_LastEnum;
}

tString cGameGui::GuiMessageToString(eGuiMessage aMessage)
{
	if(aMessage < 0 || aMessage >= eGuiMessage_LastEnum) return "";

	return gvGuiMessageNames[aMessage];
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameGui::LoadFontsFromElement(TiXmlElement *apElement)
{
	TiXmlElement *pElem = apElement->FirstChildElement();
	for(; pElem != nullptr; pElem = pElem->NextSiblingElement())
	{
		tString sName = cString::ToString(pElem->Attribute("Name"), "");
		tString sFile = cString::ToString(pElem->Attribute("File"), "");

		cVector2f vSize = cString::ToVector2f(pElem->Attribute("Size"), 12);
		cColor color = cString::ToColor(pElem->Attribute("Color"), cColor(1,1));
		eGuiMaterial material = ToGuiMaterial(pElem->Attribute("Material"));

		cGuiSkinFont *pFont = hplNew( cGuiSkinFont, (mpGui) );

		pFont->mpFont = mpGui->GetResources()->GetFontManager()->CreateFontData(sFile);
		pFont->mvSize = vSize;
		pFont->mColor = color;
		pFont->mpMaterial = mpGui->GetMaterial(material);

		if(m_mapFonts.insert(tGameGuiFontMap::value_type(sName, pFont)).second == false)
		{
			hplDelete(pFont);
		}
	}
}

//-----------------------------------------------------------------------

void cGameGui::LoadImagesFromElement(TiXmlElement *apElement)
{
	TiXmlElement *pElem = apElement->FirstChildElement();
	for(; pElem != nullptr; pElem = pElem->NextSiblingElement())
	{
		tString sType = cString::ToLowerCase(pElem->Value());

		tString sName = cString::ToString(pElem->Attribute("Name"), "");
		tString sFile = cString::ToString(pElem->Attribute("File"), "");

		cVector2f vOffset = cString::ToVector2f(pElem->Attribute("Offset"), 0);
		cVector2f vActiveSize = cString::ToVector2f(pElem->Attribute("ActiveSize"), -1);
		cColor color = cString::ToColor(pElem->Attribute("Color"), cColor(1,1));
		eGuiMaterial material = ToGuiMaterial(pElem->Attribute("Material"));

		bool bAnimated = cString::ToBool(pElem->Attribute("Animated"), false);
		float fFrameTime = cString::ToFloat(pElem->Attribute("AnimFrameTime"), 1);

		eGuiGfxAnimationType animType = eGuiGfxAnimationType_Loop;
		tString sAnimMode = cString::ToLowerCase(cString::ToString(pElem->Attribute("AnimMode"), ""));
		if(sAnimMode == "loop" || sAnimMode == "")	animType = eGuiGfxAnimationType_Loop;
		else if(sAnimMode == "oscillate")			animType = eGuiGfxAnimationType_Oscillate;
		else if(sAnimMode == "random")				animType = eGuiGfxAnimationType_Random;
		else if(sAnimMode == "stop_at_end")			animType = eGuiGfxAnimationType_StopAtEnd;
		else Warning("Animation type '%s' does not exist!\n", sAnimMode.c_str());

		cGuiGfxElement *pGfx = nullptr;

		if(sType == "texture")
		{
			pGfx = mpGui->CreateGfxTexture(sFile, material, color, false, true);
		}
		else if(sFile == "")
		{
			pGfx = mpGui->CreateGfxFilledRect(color, material, true);
		}
		else if(bAnimated)
		{
			pGfx = mpGui->CreateGfxImageBuffer(sFile, material, true, color, true);
			if(pGfx)
			{
				cGuiGfxAnimation *pAnim = pGfx->GetAnimation(0);
				pAnim->SetFrameLength(fFrameTime);
				pAnim->SetType(animType);
				pGfx->PlayAnimation(0);
			}
		}
		else
		{
			pGfx = mpGui->CreateGfxImage(sFile, material, color, true);
		}

		if(pGfx == nullptr)
		{
			Error("Could not load gui image '%s' from '%s'\n", sName.c_str(), sFile.c_str());
			continue;
		}

		if(vActiveSize.x >= 0) pGfx->SetActiveSize(vActiveSize);
		pGfx->SetOffset(cVector3f(vOffset.x, vOffset.y, 0));

		if(m_mapImages.insert(tGameGuiImageMap::value_type(sName, pGfx)).second == false)
		{
			mpGui->DestroyGfx(pGfx);
		}
	}
}

//-----------------------------------------------------------------------

static void AddItemsFromString(iWidgetItemContainer *apContainer, const tString &asItems)
{
	if(asItems == "") return;

	tString sSeparators = "|";
	tStringVec vItems;
	cString::GetStringVec(asItems, vItems, &sSeparators);

	for(size_t i=0; i<vItems.size(); ++i)
	{
		apContainer->AddItem(cString::To16Char(vItems[i]));
	}
}

void cGameGui::LoadWidgetsFromElement(TiXmlElement *apElement, iWidget *apParent)
{
	TiXmlElement *pElem = apElement->FirstChildElement();
	for(; pElem != nullptr; pElem = pElem->NextSiblingElement())
	{
		////////////////////////////
		// The kind of widget
		tString sType = cString::ToLowerCase(pElem->Value());

		eWidgetType type;
		if(sType == "window")			type = eWidgetType_Window;
		else if(sType == "button")		type = eWidgetType_Button;
		else if(sType == "frame")		type = eWidgetType_Frame;
		else if(sType == "label")		type = eWidgetType_Label;
		else if(sType == "slider")		type = eWidgetType_Slider;
		else if(sType == "textbox")		type = eWidgetType_TextBox;
		else if(sType == "checkbox")	type = eWidgetType_CheckBox;
		else if(sType == "image")		type = eWidgetType_Image;
		else if(sType == "listbox")		type = eWidgetType_ListBox;
		else if(sType == "combobox")	type = eWidgetType_ComboBox;
		else
		{
			Error("Widget type %s does not exist!\n", sType.c_str());
			continue;
		}

		////////////////////////////
		// What all widgets have
		tString sName = cString::ToString(pElem->Attribute("Name"), "");

		eWidgetSliderOrientation orientation = eWidgetSliderOrientation_Horisontal;
		if(type == eWidgetType_Slider) orientation = ToSliderOrientation(pElem->Attribute("Orientation"));

		iWidget *pWidget = CreateWidgetFromType(type, sName, apParent, mpSet, orientation);

		pWidget->SetPosition(cString::ToVector3f(pElem->Attribute("Position"), 0));
		pWidget->SetSize(cString::ToVector2f(pElem->Attribute("Size"), 0));
		pWidget->SetEnabled(cString::ToBool(pElem->Attribute("Enabled"), true));
		pWidget->SetVisible(cString::ToBool(pElem->Attribute("Visible"), true));

		tString sTextCat = cString::ToString(pElem->Attribute("TextCat"), "");
		tString sTextEntry = cString::ToString(pElem->Attribute("TextEntry"), "");

		if(sTextCat != "")
			pWidget->SetText(kTranslate(sTextCat, sTextEntry));
		else
			pWidget->SetText(cString::To16Char(cString::ToString(pElem->Attribute("Text"), "")));

		tString sFont = cString::ToString(pElem->Attribute("Font"), "");
		tString sImage = cString::ToString(pElem->Attribute("Image"), "");

		////////////////////////////
		// What each kind has
		switch(type)
		{
		case eWidgetType_Window:
		{
			cWidgetWindow *pWindow = static_cast<cWidgetWindow*>(pWidget);
			if(sFont != "") SetWidgetFont(pWidget, sFont);
			pWindow->SetStatic(cString::ToBool(pElem->Attribute("Static"), false));
			break;
		}
		case eWidgetType_Button:
		{
			cWidgetButton *pButton = static_cast<cWidgetButton*>(pWidget);
			if(sImage != "") pButton->SetImage(GetImage(sImage), false);
			if(sFont != "") SetWidgetFont(pWidget, sFont);
			break;
		}
		case eWidgetType_Frame:
		{
			cWidgetFrame *pFrame = static_cast<cWidgetFrame*>(pWidget);
			pFrame->SetBackGroundColor(cString::ToColor(pElem->Attribute("BackgroundColor"), cColor(1,1)));
			pFrame->SetBackgroundZ(cString::ToFloat(pElem->Attribute("BackgroundZ"), 0.5f));
			pFrame->SetDrawBackground(cString::ToBool(pElem->Attribute("DrawBackground"), false));
			pFrame->SetDrawFrame(cString::ToBool(pElem->Attribute("DrawFrame"), false));
			break;
		}
		case eWidgetType_Label:
		{
			cWidgetLabel *pLabel = static_cast<cWidgetLabel*>(pWidget);
			pLabel->SetWordWrap(cString::ToBool(pElem->Attribute("WordWrap"), false));
			pLabel->SetMaxTextLength(cString::ToInt(pElem->Attribute("MaxTextLength"), -1));

			tString sAlign = cString::ToLowerCase(cString::ToString(pElem->Attribute("TextAlign"), "left"));
			if(sAlign == "left")		pLabel->SetTextAlign(eFontAlign_Left);
			else if(sAlign == "right")	pLabel->SetTextAlign(eFontAlign_Right);
			else if(sAlign == "center")	pLabel->SetTextAlign(eFontAlign_Center);

			if(sFont != "") SetWidgetFont(pWidget, sFont);
			break;
		}
		case eWidgetType_Slider:
		{
			cWidgetSlider *pSlider = static_cast<cWidgetSlider*>(pWidget);
			pSlider->SetMaxValue(cString::ToInt(pElem->Attribute("MaxValue"), 0));
			pSlider->SetValue(cString::ToInt(pElem->Attribute("Value"), 0));
			pSlider->SetBarValueSize(cString::ToInt(pElem->Attribute("BarValueSize"), 1));
			pSlider->SetButtonValueAdd(cString::ToInt(pElem->Attribute("ButtonValueAdd"), 1));
			break;
		}
		case eWidgetType_TextBox:
		{
			cWidgetTextBox *pTextBox = static_cast<cWidgetTextBox*>(pWidget);
			pTextBox->SetCanEdit(cString::ToBool(pElem->Attribute("CanEdit"), true));
			pTextBox->SetMaxTextLength(cString::ToInt(pElem->Attribute("MaxTextLength"), -1));
			if(sFont != "") SetWidgetFont(pWidget, sFont);
			break;
		}
		case eWidgetType_CheckBox:
		{
			cWidgetCheckBox *pCheckBox = static_cast<cWidgetCheckBox*>(pWidget);
			pCheckBox->SetChecked(cString::ToBool(pElem->Attribute("Checked"), false));
			if(sFont != "") SetWidgetFont(pWidget, sFont);
			break;
		}
		case eWidgetType_Image:
		{
			cWidgetImage *pImage = static_cast<cWidgetImage*>(pWidget);
			pImage->SetImage(GetImage(sImage));
			break;
		}
		case eWidgetType_ListBox:
		{
			cWidgetListBox *pListBox = static_cast<cWidgetListBox*>(pWidget);
			pListBox->SetBackgroundZ(cString::ToFloat(pElem->Attribute("BackgroundZ"), -0.5f));
			AddItemsFromString(pListBox, cString::ToString(pElem->Attribute("Items"), ""));
			pListBox->SetSelectedItem(cString::ToInt(pElem->Attribute("SelectedItem"), -1), false);
			if(sFont != "") SetWidgetFont(pWidget, sFont);
			break;
		}
		case eWidgetType_ComboBox:
		{
			cWidgetComboBox *pComboBox = static_cast<cWidgetComboBox*>(pWidget);
			AddItemsFromString(pComboBox, cString::ToString(pElem->Attribute("Items"), ""));
			pComboBox->SetSelectedItem(cString::ToInt(pElem->Attribute("SelectedItem"), -1), false);
			pComboBox->SetCanEdit(cString::ToBool(pElem->Attribute("CanEdit"), false));
			pComboBox->SetMaxShownItems(cString::ToInt(pElem->Attribute("MaxShownItems"), 12));
			if(sFont != "") SetWidgetFont(pWidget, sFont);
			break;
		}
		default: break;
		}

		////////////////////////////
		// The widgets inside it
		LoadWidgetsFromElement(pElem, pWidget);
	}
}

//-----------------------------------------------------------------------

bool cGameGui::CallbackFunc(iWidget* apWidget, cGuiMessageData& aData)
{
	tGameGuiCallbackMapIt it = m_mapCallbacks.find(apWidget);
	if(it == m_mapCallbacks.end())
	{
		Warning("widget %s was not found in callback map!\n", apWidget->GetName().c_str());
		return false;
	}

	if(aData.mMessage < 0 || aData.mMessage >= eGuiMessage_LastEnum) return true;

	tString sFunc = it->second->mvFunctions[aData.mMessage];
	if(sFunc == "") return true;

	tString sCommand = sFunc + "(\"" + apWidget->GetName() + "\",\"" +
						GuiMessageToString(aData.mMessage) + "\"," +
						cString::ToString(aData.mlVal) + ")";

	RunScriptCommand(sCommand);

	return true;
}
kGuiCalllbackDeclaredFuncEnd(cGameGui, CallbackFunc)

//-----------------------------------------------------------------------

bool cGameGui::PopUpCallback(iWidget* apWidget, cGuiMessageData& aData)
{
	if(msPopUpCallback == "") return true;

	// Cleared first: the function may open the next pop up
	tString sCommand = msPopUpCallback + "(" + cString::ToString(aData.mlVal) + ")";
	msPopUpCallback = "";

	RunScriptCommand(sCommand);

	return true;
}
kGuiCalllbackDeclaredFuncEnd(cGameGui, PopUpCallback)

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// HANDLER
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

cGameGuiHandler::cGameGuiHandler(cInit *apInit) : iUpdateable("GameGuiHandler")
{
	mpInit = apInit;
	mpGui = mpInit->mpGame->GetGui();

	mpScreenGui = nullptr;

	// The lines of a cathode ray tube, over a screen that fills the view
	mpScanSet = mpGui->CreateSet("ScanLines", nullptr);
	mpScanSet->SetActive(false);
	mpScanSet->SetDrawPriority(1);

	// Overture has neither terminals nor the picture
	mpScanGfx = nullptr;
	if(cInit::IsOverture()==false)
		mpScanGfx = mpGui->CreateGfxImage("gui_scanline.tga", eGuiMaterial_Alpha, cColor(1,1), true);

	mpDrawer = mpInit->mpGame->GetGraphics()->GetDrawer();
	mpBlackGfx = mpDrawer->CreateGfxObject("effect_black.bmp","diffalpha2d");

	mvMousePos = cVector2f(400, 300);
}

//-----------------------------------------------------------------------

cGameGuiHandler::~cGameGuiHandler()
{
	CloseScreenGui(false);

	mpDrawer->DestroyGfxObject(mpBlackGfx);
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PUBLIC METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameGuiHandler::OnStart()
{
}

//-----------------------------------------------------------------------

void cGameGuiHandler::Update(float afTimeStep)
{
	if(mpScreenGui == nullptr) return;

	if(mpScreenGui->mbDestroyMe)
	{
		DestroyScreenGui();
		return;
	}

	mpInit->mpPlayer->SetCrossHairState(eCrossHairState_None);

	mpScreenGui->Update(afTimeStep);
}

//-----------------------------------------------------------------------

void cGameGuiHandler::Reset()
{
	CloseScreenGui(false);
}

//-----------------------------------------------------------------------

void cGameGuiHandler::OnDraw()
{
	if(mpScreenGui == nullptr || mpScreenGui->IsFullScreen() == false) return;

	// The scene is not drawn behind a screen that fills the view. Where the
	// window is wider than the screen, the sides would keep what was there.
	mpDrawer->DrawGfxObject(mpBlackGfx, mpInit->mpGraphicsHelper->GetScreenPos(0),
							mpInit->mpGraphicsHelper->GetScreenSize(), cColor(1,1));

	if(mpScanGfx == nullptr) return;

	float fY = 0;
	for(int i=0; i<160; ++i)
	{
		mpScanSet->DrawGfx(mpScanGfx, cVector3f(0, fY, 0), cVector2f(800, 5), cColor(1, 0.22f),
							eGuiMaterial_LastEnum);
		fY += 5;
	}
}

//-----------------------------------------------------------------------

void cGameGuiHandler::OpenScreenGui(const tString &asFile)
{
	// One screen at a time
	if(mpScreenGui) DestroyScreenGui();

	mpScreenGui = hplNew( cGameGui, (mpInit, "ScreenGui") );

	if(mpScreenGui->LoadFromFile(asFile) == false)
	{
		hplDelete(mpScreenGui);
		mpScreenGui = nullptr;
		return;
	}

	mpGui->SetFocus(mpScreenGui->GetSet());

	mpInit->mpPlayer->SetCrossHairState(eCrossHairState_None);
	mpInit->mpEffectHandler->GetSubTitle()->Reset();

	mvMousePos = cVector2f(400, 300);
	mpGui->SendMousePos(mvMousePos, 0);

	mpScreenGui->Start();

	if(mpScreenGui->IsFullScreen())
	{
		cScene *pScene = mpInit->mpGame->GetScene();
		pScene->SetDrawScene(false);
		pScene->SetUpdateMap(false);
	}

	mpScanSet->SetActive(true);
}

//-----------------------------------------------------------------------

void cGameGuiHandler::CloseScreenGui(bool abLater)
{
	if(mpScreenGui)
	{
		if(abLater)
			mpScreenGui->mbDestroyMe = true;
		else
			DestroyScreenGui();
	}

	mpScanSet->SetActive(false);
}

//-----------------------------------------------------------------------

void cGameGuiHandler::AddMousePos(const cVector2f &avRel)
{
	mvMousePos += avRel;

	if(mvMousePos.x < 0) mvMousePos.x = 0;
	if(mvMousePos.x > 800) mvMousePos.x = 800;
	if(mvMousePos.y < 0) mvMousePos.y = 0;
	if(mvMousePos.y > 600) mvMousePos.y = 600;

	mpGui->SendMousePos(mvMousePos, avRel);
}

//-----------------------------------------------------------------------

void cGameGuiHandler::DrawText(iFontData *apFont, const tWString &asText, float afAlpha)
{
	if(mpScreenGui == nullptr || asText == _W("")) return;

	cGuiSet *pSet = mpScreenGui->GetSet();

	tWStringVec vRows;
	apFont->GetWordWrapRows(750, 16, 15, asText, &vRows);

	for(size_t i=0; i<vRows.size(); ++i)
	{
		float fY = 16.0f * (float)i;

		pSet->DrawFont(vRows[i], apFont, cVector3f(25, 500 + fY, 30), 15, cColor(1, afAlpha),
						eFontAlign_Left, eGuiMaterial_FontNormal);
		pSet->DrawFont(vRows[i], apFont, cVector3f(26, 501 + fY, 29), 15, cColor(0, afAlpha),
						eFontAlign_Left, eGuiMaterial_FontNormal);
	}
}

//-----------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////
// PRIVATE METHODS
//////////////////////////////////////////////////////////////////////////

//-----------------------------------------------------------------------

void cGameGuiHandler::DestroyScreenGui()
{
	mpScreenGui->Exit();

	if(mpScreenGui->IsFullScreen())
	{
		cScene *pScene = mpInit->mpGame->GetScene();
		pScene->SetDrawScene(true);
		pScene->SetUpdateMap(true);
	}

	hplDelete(mpScreenGui);
	mpScreenGui = nullptr;

	mpGui->SetFocus(nullptr);
	mpScanSet->SetActive(false);
}

//-----------------------------------------------------------------------
