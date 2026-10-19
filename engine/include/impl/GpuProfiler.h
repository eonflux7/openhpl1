/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#ifndef HPL_GPU_PROFILER_H
#define HPL_GPU_PROFILER_H

#include <cstdio>
#include <map>
#include <string>
#include <vector>

#include <glad/gl.h>

namespace hpl {

	/**
	 * Named sections of the frame, as GL debug groups (seen by RenderDoc and
	 * the driver's debug output) and, while timing is on, GPU time stamps.
	 * Results are read a few frames late, once the GPU has them, so timing
	 * never waits for the GPU. Sections of the same name and parent are
	 * added up. Also counts draw calls and triangles.
	 *
	 * With OPENHPL_GPU_LOG set to a file name, timing is always on and
	 * every frame's times are written to that file, one line a frame.
	 */
	class cGpuProfiler
	{
	public:
		struct cSectionResult
		{
			std::string msName;
			int mlDepth;
			int mlCount;		// times the section ran in the frame
			float mfMs;			// last frame
			float mfAverageMs;	// smoothed over recent frames
		};

		cGpuProfiler();
		~cGpuProfiler();

		/// Needs a GL context. Debug groups are used if KHR_debug is there.
		void Init();
		void Exit();

		/// Time stamps are only taken while timing is on.
		void SetTiming(bool abX){ mbTiming = abX; }
		bool GetTiming(){ return mbTiming; }

		void BeginFrame();
		void EndFrame();

		void Begin(const char *asName);
		void End();

		void CountDraw(GLenum aMode, int alVertices);

		/// Names the current frame in the log (OPENHPL_GPU_LOG).
		void Mark(const std::string& asText);

		/// Names a GL object for RenderDoc and the debug output (glObjectLabel).
		static void Label(GLenum aIdentifier, GLuint alName, const std::string& asLabel);

		/// The last frame whose times are known, sections in the order they began.
		const std::vector<cSectionResult>& GetSections(){ return mvResults; }
		float GetGpuFrameMs(){ return mfGpuFrameMs; }
		int GetDrawCalls(){ return mlLastDrawCalls; }
		int GetTriangles(){ return mlLastTriangles; }

	private:
		struct cEvent
		{
			const char *msName;
			int mlDepth;
			GLuint mlBeginQuery;
			GLuint mlEndQuery;
		};
		struct cFrame
		{
			std::vector<cEvent> mvEvents;
			std::vector<GLuint> mvQueries;	// pool, grows as needed
			size_t mlQueriesUsed = 0;
			GLuint mlFrameBegin = 0;
			GLuint mlFrameEnd = 0;
			bool mbPending = false;
			int mlNumber = 0;
			int mlDrawCalls = 0, mlTriangles = 0;
			std::string msMark;
		};

		GLuint NewQuery(cFrame &aFrame);
		void Collect(cFrame &aFrame);

		static const int kFrameLag = 4;

		bool mbInitialized;
		bool mbDebugGroups;
		bool mbTiming;
		bool mbTimingThisFrame;
		cFrame mvFrames[kFrameLag];
		int mlFrame;
		int mlFrameNumber;
		int mlDepth;
		FILE *mpLog;
		std::vector<int> mvOpen;	// current frame's events not ended yet

		int mlDrawCalls, mlTriangles;
		int mlLastDrawCalls, mlLastTriangles;

		std::vector<cSectionResult> mvResults;
		std::map<std::string, float> m_mapAverage;
		float mfGpuFrameMs;
	};

};
#endif // HPL_GPU_PROFILER_H
