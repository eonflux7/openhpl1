/*
 * Copyright (C) 2006-2010 - Frictional Games
 *
 * This file is part of HPL1 Engine.
 *
 * HPL1 Engine is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * HPL1 Engine is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with HPL1 Engine.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "graphics/RenderState.h"
#include "graphics/Renderer3D.h"

#include "graphics/Material.h"
#include "graphics/VertexBuffer.h"
#include "graphics/GPUProgram.h"
#include "graphics/LowLevelGraphics.h"
#include "scene/Light3D.h"
#include "scene/Light3DSpot.h"
#include "scene/Camera3D.h"
#include "scene/PortalContainer.h"
#include "math/Math.h"


namespace hpl {

	void iRenderState::SetMode(cRenderSettings* apSettings)
	{
		switch(mType)
		{
		case eRenderStateType_Sector:				SetSectorMode(apSettings); break;
		case eRenderStateType_Pass:					SetPassMode(apSettings); break;
		case eRenderStateType_DepthTest:			SetDepthTestMode(apSettings); break;
		case eRenderStateType_AlphaMode:			SetAlphaMode(apSettings); break;
		case eRenderStateType_BlendMode:			SetBlendMode(apSettings); break;
		case eRenderStateType_Program:		SetProgramMode(apSettings); break;
		case eRenderStateType_Texture:			SetTextureMode(apSettings); break;
		case eRenderStateType_VertexBuffer:		SetVtxBuffMode(apSettings); break;
		case eRenderStateType_Matrix:			SetMatrixMode(apSettings); break;
		case eRenderStateType_Render:			SetRenderMode(apSettings); break;
		}
	}

	//-----------------------------------------------------------------------

	void iRenderState::ApplyTransparent(cRenderSettings* apSettings)
	{
		// Transparent programs supply their own lighting/eye values through
		// setup callbacks. Do not inherit the preceding opaque light pass.
		apSettings->mbUsesLight = false;
		apSettings->mbUsesEye = false;
		apSettings->mpLight = nullptr;
		SetDepthTestMode(apSettings);
		SetAlphaMode(apSettings);
		SetBlendMode(apSettings);
		if(mpProgram != apSettings->mpProgram || mpProgramSetup != apSettings->mpProgramSetup)
			SetProgramMode(apSettings);
		SetTextureMode(apSettings);
		SetVtxBuffMode(apSettings);
		SetMatrixMode(apSettings);
	}

	//-----------------------------------------------------------------------

	void iRenderState::SetSectorMode(cRenderSettings* apSettings)
	{
		if(apSettings->mbLog) Log("Sector: %d\n",mpSector);

		apSettings->mpSector = mpSector;

	}

	//-----------------------------------------------------------------------

	void iRenderState::SetPassMode(cRenderSettings* apSettings)
	{
		if(apSettings->mbLog) Log("Pass: %d\n",mlPass);
	}

	//-----------------------------------------------------------------------

	void iRenderState::SetDepthTestMode(cRenderSettings* apSettings)
	{
		if(apSettings->mbDepthTest != mbDepthTest)
		{
			apSettings->mpLowLevel->SetDepthTestActive(mbDepthTest);
			apSettings->mbDepthTest = mbDepthTest;

			if(apSettings->mbLog) Log("Setting depth test: %d\n",mbDepthTest?1:0);
		}
	}
	//-----------------------------------------------------------------------

	void iRenderState::SetAlphaMode(cRenderSettings* apSettings)
	{
		if(mAlphaMode != apSettings->mAlphaMode)
		{
			if(apSettings->mbLog) Log("Setting alpha: ");
			apSettings->mAlphaMode = mAlphaMode;

			if(mAlphaMode == eMaterialAlphaMode_Solid)
			{
				apSettings->mpLowLevel->SetAlphaCutoff(0);
				if(apSettings->mbLog)Log("Solid");
			}
			else
			{

				apSettings->mpLowLevel->SetAlphaCutoff(0.6f);
				if(apSettings->mbLog)Log("Trans");
			}

			if(apSettings->mbLog) Log("\n");
		}
	}

	//-----------------------------------------------------------------------

	void iRenderState::SetBlendMode(cRenderSettings* apSettings)
	{
		if(mBlendMode != apSettings->mBlendMode)
		{
			if(apSettings->mbLog)Log("Setting blend mode: ");
			apSettings->mBlendMode = mBlendMode;

			if(mBlendMode == eMaterialBlendMode_None)
			{
				apSettings->mpLowLevel->SetBlendActive(false);
				if(apSettings->mbLog)Log("None");
			}
			else
			{
				apSettings->mpLowLevel->SetBlendActive(true);

				switch(mBlendMode)
				{
				case eMaterialBlendMode_Add:
					apSettings->mpLowLevel->SetBlendFunc(eBlendFunc_One,eBlendFunc_One);
					if(apSettings->mbLog)Log("Add");
					break;
				case eMaterialBlendMode_Replace:
					apSettings->mpLowLevel->SetBlendFunc(eBlendFunc_One,eBlendFunc_Zero);
					if(apSettings->mbLog)Log("Replace");
					break;
				case eMaterialBlendMode_Mul:
					apSettings->mpLowLevel->SetBlendFunc(eBlendFunc_Zero,eBlendFunc_SrcColor);
					if(apSettings->mbLog)Log("Mul");
					break;
				case eMaterialBlendMode_MulX2:
					apSettings->mpLowLevel->SetBlendFunc(eBlendFunc_DestColor,eBlendFunc_SrcColor);
					if(apSettings->mbLog)Log("MulX2");
					break;
				case eMaterialBlendMode_Alpha:
					apSettings->mpLowLevel->SetBlendFunc(eBlendFunc_SrcAlpha,eBlendFunc_OneMinusSrcAlpha);
					if(apSettings->mbLog)Log("Alpha");
					break;
				case eMaterialBlendMode_DestAlphaAdd:
					apSettings->mpLowLevel->SetBlendFunc(eBlendFunc_DestAlpha,eBlendFunc_One);
					if(apSettings->mbLog)Log("DestAlphaAdd");
					break;
				}
			}

			if(apSettings->mbLog)Log("\n");
		}

		if(mChannelMode != apSettings->mChannelMode)
		{
			if(apSettings->mbLog)Log("Setting channel: ");
			apSettings->mChannelMode = mChannelMode;

			switch(mChannelMode)
			{
			case eMaterialChannelMode_RGBA:
				apSettings->mpLowLevel->SetColorWriteActive(true, true, true, true);
				if(apSettings->mbLog)Log("RGBA");
				break;
			case eMaterialChannelMode_RGB:
				apSettings->mpLowLevel->SetColorWriteActive(true, true, true, false);
				if(apSettings->mbLog)Log("RGB");
				break;
			case eMaterialChannelMode_A:
				apSettings->mpLowLevel->SetColorWriteActive(false, false, false, true);
				if(apSettings->mbLog)Log("A");
				break;
			case eMaterialChannelMode_Z:
				apSettings->mpLowLevel->SetColorWriteActive(false, false, false, false);
				if(apSettings->mbLog)Log("Z");
				break;
			}

			if(apSettings->mbLog)Log("\n");
		}
	}

	//-----------------------------------------------------------------------

	void iRenderState::SetProgramMode(cRenderSettings* settings)
	{
		if(settings->mpProgram && settings->mpProgram != mpProgram) settings->mpProgram->UnBind();
		settings->mpProgram = mpProgram;
		settings->mpProgramSetup = mpProgramSetup;
		settings->mbUsesLight = mbUsesLight;
		settings->mbUsesEye = mbUsesEye;
		settings->mpLight = mbUsesLight ? mpLight : nullptr;
		settings->mbMatrixWasNULL = false;
		if(mpProgram){
			mpProgram->Bind();
			if(mpProgramSetup) mpProgramSetup->Setup(mpProgram, settings);
			if(mbUsesLight) mpProgram->SetColor4f(eProgramParam::LightColor, mpLight->GetDiffuseColor());
		}
	}

	void iRenderState::SetTextureMode(cRenderSettings* apSettings)
	{
		for(int i=0;i<MAX_TEXTUREUNITS;i++)
		{
			if(apSettings->mpTexture[i] != mpTexture[i])
			{
				if(apSettings->mbLog)
				{
					if(mpTexture[i]==nullptr)
						Log("Setting texture: %d / %d : NULL\n",i, (size_t)mpTexture[i]);
					else
						Log("Setting texture: %d / %d : '%s'\n",i, (size_t)mpTexture[i],
																	mpTexture[i]->GetName().c_str());
				}

				apSettings->mpLowLevel->SetTexture(i,mpTexture[i]);
				apSettings->mpTexture[i] = mpTexture[i];
			}
		}
	}

	//-----------------------------------------------------------------------

	void iRenderState::SetVtxBuffMode(cRenderSettings* apSettings)
	{
		if(mpVtxBuffer != apSettings->mpVtxBuffer)
		{
			if(apSettings->mbLog)Log("Setting vertex buffer: %d\n",(size_t)mpVtxBuffer);
			if(apSettings->mpVtxBuffer) apSettings->mpVtxBuffer->UnBind();
			apSettings->mpVtxBuffer = mpVtxBuffer;

			if(mpVtxBuffer)
			{
				mpVtxBuffer->Bind();
			}
		}
	}

	//-----------------------------------------------------------------------

	void iRenderState::SetMatrixMode(cRenderSettings* apSettings)
	{
		//It is a normal matrix
		if(mpModelMatrix)
		{

			if(apSettings->mbLog)Log("Setting model matrix: %s ",cMath::MatrixToChar(*mpModelMatrix));

			apSettings->mpLowLevel->SetDrawMatrices(*mpModelMatrix, apSettings->mpCamera->GetViewMatrix(),
				apSettings->mpCamera->GetProjectionMatrix());

			apSettings->mbMatrixWasNULL = false;
		}
		//NULL matrix
		else
		{
			//If NULL already is set, no need for changes.
			if(apSettings->mbMatrixWasNULL)return;

			if(apSettings->mbLog)Log("Setting model matrix: Identity (NULL) ");

			apSettings->mpLowLevel->SetDrawMatrices(cMatrixf::Identity, apSettings->mpCamera->GetViewMatrix(),
				apSettings->mpCamera->GetProjectionMatrix());

			apSettings->mbMatrixWasNULL = true;
		}

		if(apSettings->mpProgram)
		{
			//Might be quicker if this is set directly
			apSettings->mpProgram->SetMatrixf(eProgramParam::worldViewProj,
				apSettings->mpLowLevel->GetDrawMatrices().GetModelViewProjection());
			if(apSettings->mpProgramSetup)
			{
				apSettings->mpProgramSetup->SetupMatrix(	mpModelMatrix,apSettings);
			}

			if(apSettings->mbUsesLight)
			{
				if(apSettings->mbLog)Log("Light ");
				if(mpModelMatrix)
				{
					//Light position
					cVector3f vLocalLight = cMath::MatrixMul(*mpInvModelMatrix,
													apSettings->mpLight->GetLightPosition());
					apSettings->mpProgram->SetVec3f(eProgramParam::LightPos,vLocalLight);

					//LightDir Div, use scale to make attenuation correct!
					cVector3f vLightDirDiv = mvScale / apSettings->mpLight->GetFarAttenuation();
					apSettings->mpProgram->SetVec3f(eProgramParam::LightDirMul, vLightDirDiv);

					if(apSettings->mbLog) Log("(%s) LightDirMul (%s) ",vLocalLight.ToString().c_str(),vLightDirDiv.ToString().c_str());

					//Light view projection
					if(apSettings->mpLight->GetLightType() == eLight3DType_Spot)
					{
						if(apSettings->mbLog)Log("SpotLightViewProj ");
						cLight3DSpot *pSpotLight = static_cast<cLight3DSpot*>(apSettings->mpLight);
						apSettings->mpProgram->SetMatrixf(eProgramParam::spotViewProj,
							cMath::MatrixMul(pSpotLight->GetViewProjMatrix(),*mpModelMatrix));
						apSettings->mpProgram->SetMatrixf(eProgramParam::shadowMtx,
							cMath::MatrixMul(pSpotLight->GetViewMatrix(),*mpModelMatrix));
					}
					else
					{
						apSettings->mpProgram->SetMatrixf(eProgramParam::shadowMtx,
							cMath::MatrixMul(cMath::MatrixTranslate(apSettings->mpLight->GetLightPosition() * -1.0f),
										*mpModelMatrix));
					}
				}
				else
				{
					//Light position
					apSettings->mpProgram->SetVec3f(eProgramParam::LightPos,apSettings->mpLight->GetLightPosition());

					//LightDir Div
					apSettings->mpProgram->SetVec3f(eProgramParam::LightDirMul,1.0f / apSettings->mpLight->GetFarAttenuation());

					//Light view projection
					if(apSettings->mpLight->GetLightType() == eLight3DType_Spot)
					{
						if(apSettings->mbLog)Log("SpotLightViewProj ");
						cLight3DSpot *pSpotLight = static_cast<cLight3DSpot*>(apSettings->mpLight);
						apSettings->mpProgram->SetMatrixf(eProgramParam::spotViewProj,pSpotLight->GetViewProjMatrix());
						apSettings->mpProgram->SetMatrixf(eProgramParam::shadowMtx,pSpotLight->GetViewMatrix());
					}
					else
					{
						apSettings->mpProgram->SetMatrixf(eProgramParam::shadowMtx,
							cMath::MatrixTranslate(apSettings->mpLight->GetLightPosition() * -1.0f));
					}
				}
			}

			if(apSettings->mbUsesEye)
			{
				if(apSettings->mbLog)Log("Eye ");
				if(mpModelMatrix)
				{
					cVector3f vLocalEye =  cMath::MatrixMul(*mpInvModelMatrix,
															apSettings->mpCamera->GetEyePosition());
					apSettings->mpProgram->SetVec3f(eProgramParam::EyePos,vLocalEye);
				}
				else
				{
					apSettings->mpProgram->SetVec3f(eProgramParam::EyePos,apSettings->mpCamera->GetEyePosition());
				}

				// A tube light's middle and half its length, for the
				// highlight (cLightLook, hpl1_light_look.glsl)
				if(apSettings->mbUsesLight && apSettings->mpLight->GetLook().HasTube())
				{
					const cLightLook &look = apSettings->mpLight->GetLook();
					cVector3f vCentre = apSettings->mpLight->GetLightPosition() + look.mvTubeOffset;
					cVector3f vAxis = look.mvTubeAxis;
					if(mpModelMatrix)
					{
						vCentre = cMath::MatrixMul(*mpInvModelMatrix, vCentre);
						vAxis = cMath::MatrixMul(mpInvModelMatrix->GetRotation(), vAxis);
					}
					apSettings->mpProgram->SetVec3f(eProgramParam::TubeCentre,vCentre);
					apSettings->mpProgram->SetVec3f(eProgramParam::TubeAxis,vAxis);
				}
			}
		}

		if(apSettings->mbLog)Log("\n");
	}

	//-----------------------------------------------------------------------

	void iRenderState::SetRenderMode(cRenderSettings* apSettings)
	{
		if(apSettings->mbLog)Log("Drawing\n-----------------\n");

		if(apSettings->mDebugFlags & eRendererDebugFlag_RenderLines)
		{
			apSettings->mpVtxBuffer->Draw(eVertexBufferDrawType_Lines);
		}
		else
		{
			apSettings->mpVtxBuffer->Draw();
		}
	}

	//-----------------------------------------------------------------------

}
