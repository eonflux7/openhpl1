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
#ifndef HPL_OCCLUSION_QUERY_OGL_H
#define HPL_OCCLUSION_QUERY_OGL_H

#include "graphics/OcclusionQuery.h"

namespace hpl {

	/**
	 * Each Begin() uses the next of a few GL queries, so FetchResults() can
	 * take the newest result the GPU has finished without waiting for the
	 * one just issued. Until a newer one is ready, GetSampleCount() keeps
	 * the last result.
	 */
	class cOcclusionQueryOGL : public iOcclusionQuery
	{
	public:
		cOcclusionQueryOGL();
		~cOcclusionQueryOGL() override;

		void Begin() override;
		void End() override;
		bool FetchResults() override;
		unsigned int GetSampleCount() override;

	private:
		static const int kQueries = 3;

		int mlLastSampleCount;
		unsigned int mvQueryIds[kQueries];
		bool mvIssued[kQueries];	// begun, result not read yet
		int mlCurrent;				// the last one begun
	};

};
#endif // HPL_OCCLUSION_QUERY_H
