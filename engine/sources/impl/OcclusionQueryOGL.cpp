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
#include "impl/OcclusionQueryOGL.h"

#include <glad/gl.h>


namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cOcclusionQueryOGL::cOcclusionQueryOGL()
	{
		glGenQueries(kQueries, mvQueryIds);
		for(bool& bIssued : mvIssued) bIssued = false;
		mlCurrent = 0;
		mlLastSampleCount =0;
	}

	//-----------------------------------------------------------------------

	cOcclusionQueryOGL::~cOcclusionQueryOGL()
	{
		glDeleteQueries(kQueries, mvQueryIds);
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	void cOcclusionQueryOGL::Begin()
	{
		mlCurrent = (mlCurrent + 1) % kQueries;
		glBeginQuery(GL_SAMPLES_PASSED, mvQueryIds[mlCurrent]);
		mvIssued[mlCurrent] = true;
	}

	void cOcclusionQueryOGL::End()
	{
		glEndQuery(GL_SAMPLES_PASSED);
	}

	bool cOcclusionQueryOGL::FetchResults()
	{
		// Newest first; a result read makes the older ones stale.
		for(int i=0; i<kQueries; ++i)
		{
			int lIdx = (mlCurrent - i + kQueries) % kQueries;
			if(!mvIssued[lIdx]) continue;

			GLint lAvailable = 0;
			glGetQueryObjectiv(mvQueryIds[lIdx], GL_QUERY_RESULT_AVAILABLE, &lAvailable);
			if(lAvailable == 0) continue;

			glGetQueryObjectiv(mvQueryIds[lIdx], GL_QUERY_RESULT, &mlLastSampleCount);
			for(int j=i; j<kQueries; ++j) mvIssued[(mlCurrent - j + kQueries) % kQueries] = false;
			return true;
		}
		return false;
	}

	unsigned int cOcclusionQueryOGL::GetSampleCount()
	{
		return mlLastSampleCount;
	}

	//-----------------------------------------------------------------------
}
