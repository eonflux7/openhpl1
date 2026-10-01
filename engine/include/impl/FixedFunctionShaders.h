/*
 * This file is part of Overture Remade, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#ifndef HPL_FIXED_FUNCTION_SHADERS_H
#define HPL_FIXED_FUNCTION_SHADERS_H

#include <map>

#include <glad/gl.h>

#include "graphics/GraphicsTypes.h"
#include "graphics/LowLevelGraphics.h"
#include "math/MathTypes.h"
#include "system/SystemTypes.h"

namespace hpl {

	/**
	 * The shaders for drawing without an engine program, in place of the
	 * fixed-function pipeline: a vertex stage that transforms by the
	 * low-level graphics' matrices and passes the colour and texture
	 * coordinates on, and fragment stages that combine the enabled texture
	 * units as the texture environment (GL_MODULATE, or GL_COMBINE as set
	 * with SetTextureEnv) would. A fragment stage is generated for each
	 * combination of unit targets and environment settings that is drawn
	 * with, when first drawn.
	 *
	 * Vertex stage outputs, for fragment stages written to pair with it:
	 * hpl_ffColor (primary colour, clamped to 0..1) and hpl_ffTexCoordN
	 * for units 0..kTexCoordUnits-1.
	 */
	class cFixedFunctionShaders
	{
	public:
		static const int kTexCoordUnits = 5;

		cFixedFunctionShaders();

		bool Init();
		void Exit();

		/// A unit samples alTarget, or nothing with -1 (texturing off).
		void SetUnitTarget(int alUnit, int alTarget);
		/// SetTextureEnv on a unit; the unit uses GL_COMBINE from then on.
		void SetEnv(int alUnit, eTextureParam aParam, int alVal);
		void SetConstantColor(int alUnit, const cColor &aColor);

		/// Clip plane in eye space; distances go to gl_ClipDistance[alIdx].
		void SetClipPlane(int alIdx, const cPlanef &aEyePlane);

		/// The vertex stage, with the given matrices if they changed.
		GLuint GetVertexStage(const cMatrixf &a_mtxModelView, const cMatrixf &a_mtxProjection,
								bool abMatricesChanged);
		/// The fragment stage for the current texture environment.
		GLuint GetFragmentStage();

	private:
		struct cUnit
		{
			int mlTarget;
			bool mbCombine;
			int mlColorFunc;
			int mlAlphaFunc;
			int mvColorSource[3];
			int mvAlphaSource[3];
			int mvColorOp[3];
			int mvAlphaOp[3];
			float mfColorScale;
			float mfAlphaScale;
		};

		struct cFragmentStage
		{
			GLuint mlProgram;
			GLint mlConstantLoc;
			unsigned int mlConstantVersion;
		};

		tString GetKey();
		tString GenerateFragmentSource();
		GLuint CreateFragmentStage();

		cUnit mvUnits[MAX_TEXTUREUNITS];
		float mvConstants[MAX_TEXTUREUNITS][4];
		unsigned int mlConstantVersion;

		float mvClipPlanes[kMaxClipPlanes][4];
		bool mbClipPlanesChanged;

		bool mbKeyChanged;
		cFragmentStage *mpFragmentStage;
		std::map<tString, cFragmentStage> m_mapFragmentStages;

		GLuint mlVertexStage;
		GLint mlModelViewLoc;
		GLint mlModelViewProjLoc;
		GLint mlClipPlaneLoc;
	};

};
#endif // HPL_FIXED_FUNCTION_SHADERS_H
