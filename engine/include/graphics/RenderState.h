// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_RENDER_SET_H
#define HPL_RENDER_SET_H

#include "math/MathTypes.h"
#include "graphics/GraphicsTypes.h"
#include "system/SystemTypes.h"
#include "graphics/Material.h"

namespace hpl {

	class cRenderSettings;
	class iRenderable;
	class iLight3D;
	class cSector;

	enum eRenderStateType
	{
		eRenderStateType_Sector = 0,
		eRenderStateType_Pass = 1,
		eRenderStateType_DepthTest = 2,
		eRenderStateType_AlphaMode = 3,
		eRenderStateType_BlendMode = 4,
		eRenderStateType_Program = 5,
		eRenderStateType_Texture = 6,
		eRenderStateType_VertexBuffer = 7,
		eRenderStateType_Matrix = 8,
		eRenderStateType_Render = 9,
		eRenderStateType_LastEnum  = 10
	};
	// INTERFACE

	class iRenderState
	{
	public:
		iRenderState() = default;

		void SetMode(cRenderSettings* apSettings);
		// Apply an ordered transparent draw without sorting by opaque state keys.
		void ApplyTransparent(cRenderSettings* apSettings);

		eRenderStateType mType;

		//Properties:
		//Sector
		cSector *mpSector;

		//Pass
		int mlPass;

		//DepthTest
		bool mbDepthTest;

		//Alpha
		eMaterialAlphaMode mAlphaMode;

		//Blend
		eMaterialBlendMode mBlendMode;
		eMaterialChannelMode mChannelMode;

		//Linked program
		iGpuProgram *mpProgram;
		iMaterialProgramSetup* mpProgramSetup;
		bool mbUsesLight;
		bool mbUsesEye;
		iLight3D* mpLight;


		//Texture
		iTexture* mpTexture[MAX_TEXTUREUNITS];

		//Vertex buffer
		iVertexBuffer *mpVtxBuffer;

		//Matrix
		cMatrixf *mpModelMatrix;
		cMatrixf *mpInvModelMatrix;

		//Scale
		cVector3f mvScale;

		//Render
		iRenderable *mpObject;


	private:
		//Set mode
		void SetSectorMode(cRenderSettings* apSettings);
		void SetPassMode(cRenderSettings* apSettings);
		void SetDepthTestMode(cRenderSettings* apSettings);
		void SetBlendMode(cRenderSettings* apSettings);
		void SetAlphaMode(cRenderSettings* apSettings);
		void SetProgramMode(cRenderSettings* apSettings);
		void SetTextureMode(cRenderSettings* apSettings);
		void SetVtxBuffMode(cRenderSettings* apSettings);
		void SetMatrixMode(cRenderSettings* apSettings);
		void SetRenderMode(cRenderSettings* apSettings);
	};

};
#endif // HPL_RENDER_SET_H
