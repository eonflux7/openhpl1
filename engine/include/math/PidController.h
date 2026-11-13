// Copyright (C) 2006-2010 - Frictional Games
// SPDX-License-Identifier: GPL-3.0-or-later
#ifndef HPL_PID_CONTROLLER_H
#define HPL_PID_CONTROLLER_H

#include "math/MathTypes.h"

namespace hpl {

	template <class T>
	class cPidController
	{
	public:
		float p, i ,d;
		cPidController()
		{
			p = i = d = 0;
			SetErrorNum(1);
		}
		cPidController(float afP, float afI, float afD, int alErrorNum)
		{
			p = afP; i=afI; d = afD;
			SetErrorNum(alErrorNum);
			Reset();
		}

		// Public

		T Output(T aError, float afTimeStep)
		{
			if(afTimeStep <= 0) return aError * p;
			mvErrors[mlErrorNum] = aError;
			mvTimeSteps[mlErrorNum] = afTimeStep;

			integral =0;
			size_t lCount = mvErrors.size();
			for(size_t error=0; error<lCount;++error){
				integral += mvErrors[error] * mvTimeSteps[error];
			}

			derivative = 0.0f;
			if(mlLastNum>=0)
			{
				derivative = (mvErrors[mlErrorNum] - mvErrors[mlLastNum]) / afTimeStep;
			}

			mlLastNum = mlErrorNum;
			mlErrorNum++;
			if(mlErrorNum >= (int)mvErrors.size())mlErrorNum =0;

			return mvErrors[mlLastNum]*p + integral*i + derivative*d;
		}

		void SetErrorNum(int alErrorNum)
		{
			if(alErrorNum < 1) alErrorNum = 1;
			mvErrors.resize(alErrorNum);
			mvTimeSteps.resize(alErrorNum,0);
			Reset();
		}

		void Reset()
		{
			mlErrorNum = 0;
			mlLastNum =-1;
			mvTimeSteps.assign(mvTimeSteps.size(),0);

			integral = 0;
			derivative = 0;
		}

		T GetLastError()
		{
			if(mlLastNum>=0) return mvErrors[mlLastNum];
			return 0;
		}

		T GetLastDerivative()
		{
			return derivative;
		}

		T GetLastIntegral()
		{
			return integral;
		}

	private:
		std::vector<T> mvErrors;
		std::vector<float> mvTimeSteps;

		T integral,derivative;

		int mlErrorNum;
		int mlLastNum;
	};

	typedef cPidController<float> cPidControllerf;
	typedef cPidController<cVector3f> cPidControllerVec3;
};
#endif // HPL_PID_CONTROLLER_H
