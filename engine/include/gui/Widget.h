// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_WIDGET_H
#define HPL_WIDGET_H

#include <list>
#include <vector>
#include "gui/GuiTypes.h"
#include "system/LowLevelSystem.h"

namespace hpl {

	class cGui;
	class cGuiSet;
	class cGuiSkin;
	class cGuiSkinFont;

	class cGuiGfxElement;
	class cGuiClipRegion;

	class iFontData;

	struct cWidgetCallback
	{
		cWidgetCallback(void *apObject, tGuiCallbackFunc apFunc)
		{
			mpObject = apObject;
			mpFunc = apFunc;
		}

		void *mpObject;
		tGuiCallbackFunc mpFunc;
	};

	typedef std::list<cWidgetCallback> tWidgetCallbackList;
	typedef tWidgetCallbackList::iterator tWidgetCallbackListIt;

	class iWidget
	{
	friend class cGuiSet;
	public:
		iWidget(eWidgetType aType,cGuiSet *apSet, cGuiSkin *apSkin);
		virtual ~iWidget();

		//General
		void Update(float afTimeStep);

		void Draw(float afTimeStep, cGuiClipRegion *apClipRegion);

		bool ProcessMessage(eGuiMessage aMessage, cGuiMessageData &aData);

		void AddCallback(eGuiMessage aMessage,void *apObject,tGuiCallbackFunc apFunc);

		eWidgetType GetType(){ return mType;}

		void Init();

		// Public Helper functions
		bool PointIsInside(const cVector2f& avPoint, bool abOnlyClipped);

		//Hierarchy
		void AttachChild(iWidget *apChild);
		void RemoveChild(iWidget *apChild);

		//Properties
		cGuiSet* GetSet(){ return mpSet; }

		iWidget *GetParent(){ return mpParent;}

		void SetEnabled(bool abX);
		bool IsEnabled();
		void SetVisible(bool abX);
		bool IsVisible();

		bool HasFocus();

		void SetName(const tString& asName){ msName = asName;}
		const tString& GetName(){ return msName;}

		void SetText(const tWString& asText);
		const tWString& GetText(){ return msText; }

		iFontData *GetDefaultFontType(){ return mpDefaultFontType;}
		void SetDefaultFontType(iFontData *apFont){ mpDefaultFontType = apFont;}

		const cColor& GetDefaultFontColor(){ return mDefaultFontColor;}
		void SetDefaultFontColor(const cColor& aColor){ mDefaultFontColor = aColor;}

		const cVector2f& GetDefaultFontSize(){ return mvDefaultFontSize;}
		void SetDefaultFontSize(const cVector2f& avSize){ mvDefaultFontSize = avSize;}

		void SetClipActive(bool abX){ mbClipsGraphics = abX;}
		bool GetClipActive(){ return mbClipsGraphics;}

		void SetPosition(const cVector3f &avPos);
		void SetGlobalPosition(const cVector3f &avPos);
		const cVector3f& GetLocalPosition();
		const cVector3f& GetGlobalPosition();

		void SetSize(const cVector2f &avSize);
		cVector2f GetSize(){ return mvSize;}

		bool ClipsGraphics();

		bool GetMouseIsOver(){ return mbMouseIsOver;}

		bool IsConnectedTo(iWidget *apWidget, bool abIsStartWidget=true);
		bool IsConnectedToChildren(){ return mbConnectedToChildren;}
		void SetConnectedToChildren(bool abX){ mbConnectedToChildren = abX;}

		cGuiGfxElement* GetPointerGfx();

	protected:
		// Upper Widget functions
		virtual void OnLoadGraphics(){}

		virtual void OnChangeSize(){}
		virtual void OnChangePosition(){}
		virtual void OnChangeText(){}

		virtual void OnInit(){}

		virtual void OnDraw(float afTimeStep, cGuiClipRegion *apClipRegion){}
		virtual void OnDrawAfterClip(float afTimeStep, cGuiClipRegion *apClipRegion){}

		virtual void OnUpdate(float afTimeStep){}

		virtual bool OnMessage(eGuiMessage aMessage, cGuiMessageData &aData){return false;}
		virtual bool OnMouseMove(cGuiMessageData &aData){return false;}
		virtual bool OnMouseDown(cGuiMessageData &aData){return false;}
		virtual bool OnMouseUp(cGuiMessageData &aData){return false;}
		virtual bool OnMouseDoubleClick(cGuiMessageData &aData){return false;}
		virtual bool OnMouseEnter(cGuiMessageData &aData){return false;}
		virtual bool OnMouseLeave(cGuiMessageData &aData){return false;}

		virtual bool OnGotFocus(cGuiMessageData &aData);
		virtual bool OnLostFocus(cGuiMessageData &aData){return false;}

		virtual bool OnKeyPress(cGuiMessageData &aData){return false;}

		// Private Helper functions
		cVector3f WorldToLocalPosition(const cVector3f &avPos);
		cVector2f GetPosRelativeToMouse(cGuiMessageData &aData);

		//The order must be like this:
		//Borders: Right, Left, Up and Down
		//Corners: LEftUp, RightUp, RightDown and LEftDown.
		void DrawBordersAndCorners(cGuiGfxElement *apBackground,
									cGuiGfxElement **apBorderVec,cGuiGfxElement **apCornerVec,
									const cVector3f &avPosition, const cVector2f &avSize);

		void DrawSkinText(const tWString& asText,eGuiSkinFont aFont,const cVector3f& avPosition,
							eFontAlign aAlign = eFontAlign_Left);

		void DrawDefaultText(	const tWString& asText,
								const cVector3f& avPosition,eFontAlign aAlign);

		void SetPositionUpdated();

		void LoadGraphics();

		// Variables
		cGuiSet *mpSet;
		cGuiSkin *mpSkin;
		cGui *mpGui;

		tWString msText;
		cVector3f mvPosition;
		cVector3f mvGlobalPosition;
		cVector2f mvSize;

		tString msName;

		eWidgetType mType;

		int mlPositionCount;

		cGuiSkinFont *mpDefaultFont;
		iFontData *mpDefaultFontType;
		cColor mDefaultFontColor;
		cVector2f mvDefaultFontSize;

		iWidget *mpParent;

		tWidgetList mlstChildren;

		bool mbEnabled;
		bool mbVisible;

		bool mbMouseIsOver;

		bool mbClipsGraphics;

		cGuiGfxElement* mpPointerGfx;

		bool mbConnectedToChildren;

	private:
		void SetMouseIsOver(bool abX){ mbMouseIsOver = abX;}
		bool ProcessCallbacks(eGuiMessage aMessage, cGuiMessageData &aData);


		std::vector<tWidgetCallbackList> mvCallbackLists;

		bool mbPositionIsUpdated;
	};

};
#endif // HPL_WIDGET_H
