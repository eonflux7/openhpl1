/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#include "scene/RenderInterpolation.h"

#include <algorithm>
#include <cmath>

#include "scene/Entity3D.h"
#include "math/Math.h"

namespace hpl {

	//-----------------------------------------------------------------------

	void cRenderInterpolation::RemoveEntity(iEntity3D *apEntity)
	{
		mvEntities.erase(std::remove(mvEntities.begin(), mvEntities.end(), apEntity), mvEntities.end());
	}

	//-----------------------------------------------------------------------

	void cRenderInterpolation::ApplyEntities(float afT)
	{
		if(IsBlending()==false) return;

		for(size_t i=0; i< mvEntities.size(); ++i)
			mvEntities[i]->ApplyRenderInterpolation(afT);
	}

	void cRenderInterpolation::RestoreEntities()
	{
		for(size_t i=mvEntities.size(); i>0; --i)
			mvEntities[i-1]->RestoreRenderInterpolation();
	}

	//-----------------------------------------------------------------------

	cMatrixf cRenderInterpolation::BlendMatrix(float afT, const cMatrixf& a_mtxA, const cMatrixf& a_mtxB)
	{
		for(int c=0; c<3; ++c)
		{
			float fLenA = a_mtxA.m[0][c]*a_mtxA.m[0][c] + a_mtxA.m[1][c]*a_mtxA.m[1][c] + a_mtxA.m[2][c]*a_mtxA.m[2][c];
			float fLenB = a_mtxB.m[0][c]*a_mtxB.m[0][c] + a_mtxB.m[1][c]*a_mtxB.m[1][c] + a_mtxB.m[2][c]*a_mtxB.m[2][c];
			if(std::abs(fLenA - 1.0f) > 1e-3f || std::abs(fLenB - 1.0f) > 1e-3f)
			{
				cMatrixf mtxOut = a_mtxB;
				for(int r=0; r<3; ++r)
					for(int k=0; k<4; ++k)
						mtxOut.m[r][k] = a_mtxA.m[r][k] + (a_mtxB.m[r][k] - a_mtxA.m[r][k]) * afT;
				return mtxOut;
			}
		}
		return cMath::MatrixInterpolate(afT, a_mtxA, a_mtxB);
	}

	//-----------------------------------------------------------------------

}
