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
#ifndef HPL_GPU_PROGRAM_H
#define HPL_GPU_PROGRAM_H

#include "system/SystemTypes.h"
#include "graphics/ProgramParameters.h"
#include "math/MathTypes.h"
#include "resources/ResourceBase.h"
#include "graphics/GraphicsTypes.h"

namespace hpl {

	class iTexture;

	class iGpuProgram : public iResourceBase
	{
	public:
		iGpuProgram(tString asName) : iResourceBase(asName,0){}
		~iGpuProgram() override{}

		static void SetLogDebugInformation(bool abX){mbDebugInfo = abX;}

		// GLSL base name (or vertex:fragment) and space-separated defines.
		virtual bool CreateFromFile(const tString& asFile, const tString& asDefines)=0;

		/**
		 * Bind the program to the GPU
		 */
		virtual void Bind()=0;
		/**
		 * Unbind the program to the GPU
		 */
		virtual void UnBind()=0;

		// Values belong to this linked program and survive shader reloads.
		// Typed slots update its CPU block; the backend uploads dirty values
		// once at draw time. Matrices are converted to std140 column order.
		bool SetFloat(eProgramParam aParam, float afX);

		bool SetVec2f(eProgramParam aParam, const cVector2f avVec)
		{
			return SetVec2f(aParam,avVec.x, avVec.y);
		}
		bool SetVec2f(eProgramParam aParam, float afX,float afY);

		bool SetVec3f(eProgramParam aParam, const cVector3f& avVec)
		{
			return SetVec3f(aParam, avVec.x,avVec.y,avVec.z);
		}
		bool SetColor3f(eProgramParam aParam, const cColor &aCol)
		{
			return SetVec3f(aParam,aCol.r, aCol.g, aCol.b);
		}
		bool SetVec3f(eProgramParam aParam, float afX,float afY,float afZ);

		bool SetColor4f(eProgramParam aParam, const cColor &aCol)
		{
			return SetVec4f(aParam,aCol.r, aCol.g, aCol.b, aCol.a);
		}
		bool SetVec4f(eProgramParam aParam, float afX,float afY,float afZ, float afW);

		bool SetMatrixf(eProgramParam aParam, const cMatrixf& mMtx);

		virtual bool SetTexture(const tString& asName,iTexture* apTexture, bool abAutoDisable=true)=0;
		virtual bool SetTextureToUnit(int alUnit, iTexture* apTexture)=0;


		static eProgramParam Element(eProgramParam aParam, unsigned index){
			return static_cast<eProgramParam>(static_cast<unsigned>(aParam) + index);
		}
		const float* GetParameterData() const { return mvParameters; }
		bool ParametersChanged() const { return mbParametersChanged; }
		void ParametersUploaded(){ mbParametersChanged = false; }

	protected:
		float mvParameters[static_cast<unsigned>(eProgramParam::Count) * 4] = {};
		bool mbParametersChanged = true;

		static bool mbDebugInfo;
	};
};
#endif // HPL_GPU_PROGRAM_H
