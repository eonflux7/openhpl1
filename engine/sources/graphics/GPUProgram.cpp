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
#include "graphics/GPUProgram.h"
#include <cstring>

namespace hpl{

	bool iGpuProgram::mbDebugInfo=false;

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	//-----------------------------------------------------------------------


	bool iGpuProgram::SetVec4f(eProgramParam aParam, float x, float y, float z, float w)
	{
		if(static_cast<unsigned>(aParam) >= static_cast<unsigned>(eProgramParam::Count)) return false;
		float* dst = mvParameters + static_cast<unsigned>(aParam) * 4;
		const float src[] = {x, y, z, w};
		if(std::memcmp(dst, src, sizeof(src)) != 0){
			std::memcpy(dst, src, sizeof(src)); mbParametersChanged = true;
		}
		return true;
	}
	bool iGpuProgram::SetFloat(eProgramParam p, float x){ return SetVec4f(p, x, 0, 0, 0); }
	bool iGpuProgram::SetVec2f(eProgramParam p, float x, float y){ return SetVec4f(p, x, y, 0, 0); }
	bool iGpuProgram::SetVec3f(eProgramParam p, float x, float y, float z){ return SetVec4f(p, x, y, z, 0); }
	bool iGpuProgram::SetMatrixf(eProgramParam p, const cMatrixf& matrix)
	{
		if(static_cast<unsigned>(p) + 4 > static_cast<unsigned>(eProgramParam::Count)) return false;
		for(unsigned col=0; col<4; ++col)
			SetVec4f(Element(p,col), matrix.m[0][col], matrix.m[1][col], matrix.m[2][col], matrix.m[3][col]);
		return true;
	}
}
