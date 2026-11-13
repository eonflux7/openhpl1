// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_GUI_GFX_ELEMENT_H
#define HPL_GUI_GFX_ELEMENT_H

#include "gui/GuiTypes.h"

namespace hpl {

	class iGuiMaterial;
	class iTexture;
	class cResourceImage;

	class cGui;

	#define kMaxGuiTextures (4)

	enum eGuiGfxAnimationType
	{
		eGuiGfxAnimationType_Loop,
		eGuiGfxAnimationType_StopAtEnd,
		eGuiGfxAnimationType_Oscillate,
		eGuiGfxAnimationType_Random,
		eGuiGfxAnimationType_LastEnum
	};

	class cGuiGfxAnimation
	{
	friend class cGuiGfxElement;
	public:
		cGuiGfxAnimation(): mfFrameLength(1), mType(eGuiGfxAnimationType_Loop){}

		void AddFrame(int alNum);
		void SetType(eGuiGfxAnimationType aType);
		void SetFrameLength(float afLength);

	private:
		tString msName;
		std::vector<int> mvFrames;
		float mfFrameLength;
		eGuiGfxAnimationType mType;
	};

	class cGuiGfxElement
	{
		friend class cGuiSet;
		friend class cGuiRenderObjectCompare;
	public:
		cGuiGfxElement(cGui* apGui);
		~cGuiGfxElement();

		void Update(float afTimeStep);

		void AddImage(cResourceImage* apImage);
		void AddTexture(iTexture* apTexture);

		void AddImageToBuffer(cResourceImage* apImage);

		void SetOffset(const cVector3f& avOffset){mvOffset = avOffset;}
		const cVector3f& GetOffset()const { return mvOffset;}

		void SetActiveSize(const cVector2f& avSize){mvActiveSize = avSize;}
		const cVector2f& GetActiveSize(){ return mvActiveSize;}

		cGuiGfxAnimation* CreateAnimation(const tString& asName);
		void PlayAnimation(int alNum);
		cGuiGfxAnimation* GetAnimation(int alIdx){return mvAnimations[alIdx];}
		void SetAnimationTime(float afTime);

		void SetAnimationPaused(bool abX){mbAnimationPaused = abX;}
		bool GetAnimationPaused(){ return mbAnimationPaused;}

		void SetMaterial(iGuiMaterial *apMat);

		void SetColor(const cColor &aColor);

		cVector2f GetImageSize();

		void Flush();
	private:
		void SetImage(cResourceImage* apImage, int alNum);

		cVector2f mvImageSize;
		cGui *mpGui;
		tVertexVec mvVtx;

		cVector3f mvOffset;
		cVector2f mvActiveSize;

		iGuiMaterial *mpMaterial;
		iTexture* mvTextures[kMaxGuiTextures];
		cResourceImage* mvImages[kMaxGuiTextures];

		std::vector<cResourceImage*> mvImageBufferVec;

		std::vector<cGuiGfxAnimation*> mvAnimations;
		int mlCurrentAnimation;
		float mfCurrentFrame;
		int mlActiveImage;
		bool mbForwardAnim;
		bool mbAnimationPaused;

		int mlTextureNum;

		bool mbFlushed;
	};
};
#endif // HPL_GUI_GFX_ELEMENT_H
