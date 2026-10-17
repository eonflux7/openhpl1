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

namespace hpl
{
	bool iGpuProgram::mbDebugInfo = false;

	bool iGpuProgram::SetVec4f(eProgramParam aParam, float afX, float afY, float afZ, float afW)
	{
		if(static_cast<unsigned>(aParam) >= static_cast<unsigned>(eProgramParam::Count)) return false;
		float *pDestination = mvParameters + static_cast<unsigned>(aParam) * 4;
		const float vSource[] = {afX, afY, afZ, afW};
		if(std::memcmp(pDestination, vSource, sizeof(vSource)) != 0)
		{
			std::memcpy(pDestination, vSource, sizeof(vSource));
			const unsigned lSlot = static_cast<unsigned>(aParam);
			if(lSlot < mlFirstChangedParameter) mlFirstChangedParameter = lSlot;
			if(lSlot + 1 > mlEndChangedParameter) mlEndChangedParameter = lSlot + 1;
		}
		return true;
	}

	bool iGpuProgram::SetFloat(eProgramParam aParam, float afX)
	{
		return SetVec4f(aParam, afX, 0, 0, 0);
	}

	bool iGpuProgram::SetVec2f(eProgramParam aParam, float afX, float afY)
	{
		return SetVec4f(aParam, afX, afY, 0, 0);
	}

	bool iGpuProgram::SetVec3f(eProgramParam aParam, float afX, float afY, float afZ)
	{
		return SetVec4f(aParam, afX, afY, afZ, 0);
	}

	bool iGpuProgram::SetMatrixf(eProgramParam aParam, const cMatrixf &aMatrix)
	{
		if(static_cast<unsigned>(aParam) + 4 > static_cast<unsigned>(eProgramParam::Count)) return false;
		for(unsigned lColumn = 0; lColumn < 4; ++lColumn)
			SetVec4f(Element(aParam, lColumn), aMatrix.m[0][lColumn], aMatrix.m[1][lColumn], aMatrix.m[2][lColumn], aMatrix.m[3][lColumn]);
		return true;
	}
} // namespace hpl
