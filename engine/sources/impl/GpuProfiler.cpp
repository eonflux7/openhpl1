/*
 * This file is part of Overture Remade, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#include "impl/GpuProfiler.h"

#include <cstdlib>

namespace hpl {

	//-----------------------------------------------------------------------

	cGpuProfiler::cGpuProfiler()
	{
		mbInitialized = false;
		mbDebugGroups = false;
		mbTiming = false;
		mbTimingThisFrame = false;
		mlFrame = 0;
		mlFrameNumber = 0;
		mlDepth = 0;
		mpLog = nullptr;
		mlDrawCalls = mlTriangles = 0;
		mlLastDrawCalls = mlLastTriangles = 0;
		mfGpuFrameMs = 0;
	}

	cGpuProfiler::~cGpuProfiler()
	{
	}

	//-----------------------------------------------------------------------

	void cGpuProfiler::Init()
	{
		mbDebugGroups = GLAD_GL_VERSION_4_3 || GLAD_GL_KHR_debug;
		const char *pLog = getenv("OVERTURE_GPU_LOG");
		if(pLog && pLog[0] && mpLog == nullptr) mpLog = fopen(pLog, "w");
		mbInitialized = true;
	}

	void cGpuProfiler::Exit()
	{
		for(cFrame& frame : mvFrames)
		{
			if(!frame.mvQueries.empty())
				glDeleteQueries((GLsizei)frame.mvQueries.size(), frame.mvQueries.data());
			frame = cFrame();
		}
		if(mpLog) fclose(mpLog);
		mpLog = nullptr;
		mbInitialized = false;
	}

	//-----------------------------------------------------------------------

	void cGpuProfiler::BeginFrame()
	{
		mlLastDrawCalls = mlDrawCalls;
		mlLastTriangles = mlTriangles;
		mlDrawCalls = mlTriangles = 0;

		if(!mbInitialized) return;

		// The oldest frame is reused: read its times if the GPU has them.
		mlFrame = (mlFrame + 1) % kFrameLag;
		cFrame& frame = mvFrames[mlFrame];
		Collect(frame);

		frame.mvEvents.clear();
		frame.mlQueriesUsed = 0;
		frame.mlNumber = ++mlFrameNumber;
		frame.msMark = "";
		mvOpen.clear();
		mlDepth = 0;

		mbTimingThisFrame = mbTiming || mpLog != nullptr;
		if(mbTimingThisFrame)
		{
			frame.mlFrameBegin = NewQuery(frame);
			glQueryCounter(frame.mlFrameBegin, GL_TIMESTAMP);
		}
	}

	void cGpuProfiler::EndFrame()
	{
		if(!mbInitialized) return;

		while(mlDepth > 0) End();

		cFrame& frame = mvFrames[mlFrame];
		frame.mlDrawCalls = mlDrawCalls;
		frame.mlTriangles = mlTriangles;
		if(mbTimingThisFrame)
		{
			frame.mlFrameEnd = NewQuery(frame);
			glQueryCounter(frame.mlFrameEnd, GL_TIMESTAMP);
			frame.mbPending = true;
		}
	}

	//-----------------------------------------------------------------------

	void cGpuProfiler::Begin(const char *asName)
	{
		if(!mbInitialized) return;

		if(mbDebugGroups) glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, asName);

		if(mbTimingThisFrame)
		{
			cFrame& frame = mvFrames[mlFrame];
			cEvent event = { asName, mlDepth, NewQuery(frame), 0 };
			glQueryCounter(event.mlBeginQuery, GL_TIMESTAMP);
			mvOpen.push_back((int)frame.mvEvents.size());
			frame.mvEvents.push_back(event);
		}
		++mlDepth;
	}

	void cGpuProfiler::End()
	{
		if(!mbInitialized || mlDepth == 0) return;
		--mlDepth;

		if(mbDebugGroups) glPopDebugGroup();

		if(mbTimingThisFrame && !mvOpen.empty())
		{
			cFrame& frame = mvFrames[mlFrame];
			cEvent& event = frame.mvEvents[mvOpen.back()];
			mvOpen.pop_back();
			event.mlEndQuery = NewQuery(frame);
			glQueryCounter(event.mlEndQuery, GL_TIMESTAMP);
		}
	}

	//-----------------------------------------------------------------------

	void cGpuProfiler::CountDraw(GLenum aMode, int alVertices)
	{
		++mlDrawCalls;
		if(aMode == GL_TRIANGLES)	mlTriangles += alVertices / 3;
		else if(aMode == GL_TRIANGLE_FAN && alVertices > 2)	mlTriangles += alVertices - 2;
	}

	//-----------------------------------------------------------------------

	void cGpuProfiler::Mark(const std::string& asText)
	{
		mvFrames[mlFrame].msMark = asText;
	}

	//-----------------------------------------------------------------------

	void cGpuProfiler::Label(GLenum aIdentifier, GLuint alName, const std::string& asLabel)
	{
		if(alName == 0 || !(GLAD_GL_VERSION_4_3 || GLAD_GL_KHR_debug)) return;
		// GL guarantees labels of 256 characters
		std::string sLabel = asLabel.size() > 255 ? asLabel.substr(asLabel.size() - 255) : asLabel;
		glObjectLabel(aIdentifier, alName, (GLsizei)sLabel.size(), sLabel.c_str());
	}

	//-----------------------------------------------------------------------

	GLuint cGpuProfiler::NewQuery(cFrame &aFrame)
	{
		if(aFrame.mlQueriesUsed == aFrame.mvQueries.size())
		{
			GLuint lQuery;
			glGenQueries(1, &lQuery);
			aFrame.mvQueries.push_back(lQuery);
		}
		return aFrame.mvQueries[aFrame.mlQueriesUsed++];
	}

	//-----------------------------------------------------------------------

	void cGpuProfiler::Collect(cFrame &aFrame)
	{
		if(!aFrame.mbPending) return;
		aFrame.mbPending = false;

		// The frame's last time stamp is available once all of them are;
		// if it is not, the GPU is far behind and the frame is skipped.
		GLuint lAvailable = GL_FALSE;
		glGetQueryObjectuiv(aFrame.mlFrameEnd, GL_QUERY_RESULT_AVAILABLE, &lAvailable);
		if(!lAvailable) return;

		GLuint64 lBegin, lEnd;
		glGetQueryObjectui64v(aFrame.mlFrameBegin, GL_QUERY_RESULT, &lBegin);
		glGetQueryObjectui64v(aFrame.mlFrameEnd, GL_QUERY_RESULT, &lEnd);
		mfGpuFrameMs = (float)((lEnd - lBegin) / 1.0e6);

		// Sections are keyed by their path, so the same name under
		// different parents stays apart.
		mvResults.clear();
		std::map<std::string, size_t> mapIndex;
		std::vector<std::string> vPath;
		std::vector<std::string> vKeys;
		for(const cEvent& event : aFrame.mvEvents)
		{
			if(event.mlEndQuery == 0) continue;

			vPath.resize(event.mlDepth);
			vPath.push_back(event.msName);
			std::string sKey;
			for(const std::string& sPart : vPath) sKey += "/" + sPart;

			glGetQueryObjectui64v(event.mlBeginQuery, GL_QUERY_RESULT, &lBegin);
			glGetQueryObjectui64v(event.mlEndQuery, GL_QUERY_RESULT, &lEnd);
			float fMs = (float)((lEnd - lBegin) / 1.0e6);

			std::map<std::string, size_t>::iterator it = mapIndex.find(sKey);
			if(it == mapIndex.end())
			{
				mapIndex[sKey] = mvResults.size();
				mvResults.push_back({event.msName, event.mlDepth, 1, fMs, 0});
				vKeys.push_back(sKey);
			}
			else
			{
				mvResults[it->second].mlCount++;
				mvResults[it->second].mfMs += fMs;
			}
		}

		for(size_t i=0; i<mvResults.size(); ++i)
		{
			std::map<std::string, float>::iterator it = m_mapAverage.find(vKeys[i]);
			float fAverage = it == m_mapAverage.end() ? mvResults[i].mfMs :
							it->second + (mvResults[i].mfMs - it->second) * 0.05f;
			m_mapAverage[vKeys[i]] = fAverage;
			mvResults[i].mfAverageMs = fAverage;
		}

		if(mpLog)
		{
			fprintf(mpLog, "%d\t%.3f\t%d\t%d\t%s", aFrame.mlNumber, mfGpuFrameMs, aFrame.mlDrawCalls,
					aFrame.mlTriangles, aFrame.msMark.c_str());
			for(size_t i=0; i<mvResults.size(); ++i)
				fprintf(mpLog, "\t%s=%.3f", vKeys[i].c_str(), mvResults[i].mfMs);
			fprintf(mpLog, "\n");
			fflush(mpLog);
		}
	}

	//-----------------------------------------------------------------------

}
