/*
 * This file is part of Overture Remade, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#include "impl/FixedFunctionShaders.h"

#include "impl/GLSLProgram.h"
#include "impl/GpuProfiler.h"
#include "system/String.h"
#include "math/Math.h"

#include <cstring>

namespace hpl {

	//////////////////////////////////////////////////////////////////////////
	// SHADER SOURCE
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	// HPL_CLIP_PLANES is defined before it
	static const char *kVertexSource =
		"uniform mat4 hpl_ffModelView;\n"
		"uniform mat4 hpl_ffModelViewProj;\n"
		"uniform vec4 hpl_ffClipPlane[HPL_CLIP_PLANES];\n"
		"\n"
		"out vec4 hpl_ffColor;\n"
		"out vec4 hpl_ffTexCoord0;\n"
		"out vec4 hpl_ffTexCoord1;\n"
		"out vec4 hpl_ffTexCoord2;\n"
		"out vec4 hpl_ffTexCoord3;\n"
		"out vec4 hpl_ffTexCoord4;\n"
		"out float gl_ClipDistance[HPL_CLIP_PLANES];\n"
		"\n"
		"void main()\n"
		"{\n"
		"	gl_Position = hpl_ffModelViewProj * gl_Vertex;\n"
		"	hpl_ffColor = clamp(gl_Color, 0.0, 1.0);\n"
		"	hpl_ffTexCoord0 = gl_MultiTexCoord0;\n"
		"	hpl_ffTexCoord1 = gl_MultiTexCoord1;\n"
		"	hpl_ffTexCoord2 = gl_MultiTexCoord2;\n"
		"	hpl_ffTexCoord3 = gl_MultiTexCoord3;\n"
		"	hpl_ffTexCoord4 = gl_MultiTexCoord4;\n"
		"\n"
		"	vec4 eyePos = hpl_ffModelView * gl_Vertex;\n"
		"	for(int i=0; i<HPL_CLIP_PLANES; ++i)\n"
		"		gl_ClipDistance[i] = dot(eyePos, hpl_ffClipPlane[i]);\n"
		"}\n";

	//-----------------------------------------------------------------------

	static const char* SamplerType(int alTarget)
	{
		switch(alTarget)
		{
		case eTextureTarget_1D:			return "sampler1D";
		case eTextureTarget_2D:			return "sampler2D";
		case eTextureTarget_Rect:		return "sampler2DRect";
		case eTextureTarget_CubeMap:	return "samplerCube";
		case eTextureTarget_3D:			return "sampler3D";
		}
		return "sampler2D";
	}

	// An argument of GL_COMBINE: source and operand. asTexture is the
	// unit's texel, asPrevious the result of the units before.
	static tString ColorArg(int alSource, int alOp, int alUnit)
	{
		tString sSource;
		switch(alSource)
		{
		case eTextureSource_Texture:	sSource = "tex"; break;
		case eTextureSource_Constant:	sSource = "hpl_ffConstant[" + cString::ToString(alUnit) + "]"; break;
		case eTextureSource_Primary:	sSource = "hpl_ffColor"; break;
		default:						sSource = "prev"; break;
		}

		switch(alOp)
		{
		case eTextureOp_Color:			return sSource + ".rgb";
		case eTextureOp_OneMinusColor:	return "(1.0 - " + sSource + ".rgb)";
		case eTextureOp_Alpha:			return "vec3(" + sSource + ".a)";
		default:						return "vec3(1.0 - " + sSource + ".a)";
		}
	}

	static tString AlphaArg(int alSource, int alOp, int alUnit)
	{
		// vec3(x.a) or vec3(1.0 - x.a)
		tString sArg = ColorArg(alSource, alOp, alUnit);
		return sArg.substr(5, sArg.size() - 6);
	}

	static tString CombineFunc(int alFunc, const tString& asArg0, const tString& asArg1, const tString& asArg2,
								const tString& asType)
	{
		switch(alFunc)
		{
		case eTextureFunc_Replace:		return asArg0;
		case eTextureFunc_Add:			return asArg0 + " + " + asArg1;
		case eTextureFunc_Substract:	return asArg0 + " - " + asArg1;
		case eTextureFunc_AddSigned:	return asArg0 + " + " + asArg1 + " - " + asType + "(0.5)";
		case eTextureFunc_Interpolate:	return asArg0 + " * " + asArg2 + " + " + asArg1 + " * (" + asType + "(1.0) - " + asArg2 + ")";
		case eTextureFunc_Dot3RGB:
		case eTextureFunc_Dot3RGBA:		return asType + "(4.0 * dot(" + asArg0 + " - vec3(0.5), " + asArg1 + " - vec3(0.5)))";
		default:						return asArg0 + " * " + asArg1;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cFixedFunctionShaders::cFixedFunctionShaders()
	{
		for(int i=0; i<MAX_TEXTUREUNITS; ++i)
		{
			// The GL defaults: GL_MODULATE, and these for GL_COMBINE
			cUnit &unit = mvUnits[i];
			unit.mlTarget = -1;
			unit.mbCombine = false;
			unit.mlColorFunc = unit.mlAlphaFunc = eTextureFunc_Modulate;
			unit.mvColorSource[0] = unit.mvAlphaSource[0] = eTextureSource_Texture;
			unit.mvColorSource[1] = unit.mvAlphaSource[1] = eTextureSource_Previous;
			unit.mvColorSource[2] = unit.mvAlphaSource[2] = eTextureSource_Constant;
			unit.mvColorOp[0] = unit.mvColorOp[1] = eTextureOp_Color;
			unit.mvColorOp[2] = eTextureOp_Alpha;
			unit.mvAlphaOp[0] = unit.mvAlphaOp[1] = unit.mvAlphaOp[2] = eTextureOp_Alpha;
			unit.mfColorScale = unit.mfAlphaScale = 1;

			for(int j=0; j<4; ++j) mvConstants[i][j] = 0;
		}
		mlConstantVersion = 1;

		memset(mvClipPlanes, 0, sizeof(mvClipPlanes));
		mbClipPlanesChanged = true;

		mbKeyChanged = true;
		mpFragmentStage = nullptr;

		mlVertexStage = 0;
		mlModelViewLoc = mlModelViewProjLoc = mlClipPlaneLoc = -1;
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	bool cFixedFunctionShaders::Init()
	{
		tString sSource = "#define HPL_CLIP_PLANES " + cString::ToString(kMaxClipPlanes) + "\n" + kVertexSource;
		mlVertexStage = cGLSLProgram::CompileSource(GL_VERTEX_SHADER, sSource, "fixed-function vertex", false);
		if(mlVertexStage==0) return false;

		mlModelViewLoc = glGetUniformLocation(mlVertexStage, "hpl_ffModelView");
		mlModelViewProjLoc = glGetUniformLocation(mlVertexStage, "hpl_ffModelViewProj");
		mlClipPlaneLoc = glGetUniformLocation(mlVertexStage, "hpl_ffClipPlane");
		return true;
	}

	void cFixedFunctionShaders::Exit()
	{
		if(mlVertexStage) glDeleteProgram(mlVertexStage);
		mlVertexStage = 0;

		for(auto& entry : m_mapFragmentStages)
			if(entry.second.mlProgram) glDeleteProgram(entry.second.mlProgram);
		m_mapFragmentStages.clear();
		mpFragmentStage = nullptr;
		mbKeyChanged = true;
	}

	//-----------------------------------------------------------------------

	void cFixedFunctionShaders::SetUnitTarget(int alUnit, int alTarget)
	{
		if(alUnit < 0 || alUnit >= MAX_TEXTUREUNITS) return;
		if(mvUnits[alUnit].mlTarget == alTarget) return;
		mvUnits[alUnit].mlTarget = alTarget;
		mbKeyChanged = true;
	}

	//-----------------------------------------------------------------------

	// Values GL would reject (an alpha operand of colour, a DOT3 alpha
	// function, a scale other than 1, 2 or 4) leave the setting as it was,
	// as GL did.
	void cFixedFunctionShaders::SetEnv(int alUnit, eTextureParam aParam, int alVal)
	{
		if(alUnit < 0 || alUnit >= MAX_TEXTUREUNITS) return;
		cUnit &unit = mvUnits[alUnit];
		unit.mbCombine = true;
		mbKeyChanged = true;

		bool bAlphaOp = alVal == eTextureOp_Alpha || alVal == eTextureOp_OneMinusAlpha;
		bool bScale = alVal == 1 || alVal == 2 || alVal == 4;

		switch(aParam)
		{
		case eTextureParam_ColorFunc:		unit.mlColorFunc = alVal; break;
		case eTextureParam_AlphaFunc:
			if(alVal != eTextureFunc_Dot3RGB && alVal != eTextureFunc_Dot3RGBA) unit.mlAlphaFunc = alVal;
			break;
		case eTextureParam_ColorSource0:	unit.mvColorSource[0] = alVal; break;
		case eTextureParam_ColorSource1:	unit.mvColorSource[1] = alVal; break;
		case eTextureParam_ColorSource2:	unit.mvColorSource[2] = alVal; break;
		case eTextureParam_AlphaSource0:	unit.mvAlphaSource[0] = alVal; break;
		case eTextureParam_AlphaSource1:	unit.mvAlphaSource[1] = alVal; break;
		case eTextureParam_AlphaSource2:	unit.mvAlphaSource[2] = alVal; break;
		case eTextureParam_ColorOp0:		unit.mvColorOp[0] = alVal; break;
		case eTextureParam_ColorOp1:		unit.mvColorOp[1] = alVal; break;
		case eTextureParam_ColorOp2:		unit.mvColorOp[2] = alVal; break;
		case eTextureParam_AlphaOp0:		if(bAlphaOp) unit.mvAlphaOp[0] = alVal; break;
		case eTextureParam_AlphaOp1:		if(bAlphaOp) unit.mvAlphaOp[1] = alVal; break;
		case eTextureParam_AlphaOp2:		if(bAlphaOp) unit.mvAlphaOp[2] = alVal; break;
		case eTextureParam_ColorScale:		if(bScale) unit.mfColorScale = (float)alVal; break;
		case eTextureParam_AlphaScale:		if(bScale) unit.mfAlphaScale = (float)alVal; break;
		default: break;
		}
	}

	//-----------------------------------------------------------------------

	void cFixedFunctionShaders::SetConstantColor(int alUnit, const cColor &aColor)
	{
		if(alUnit < 0 || alUnit >= MAX_TEXTUREUNITS) return;
		// Clamped, as GL_TEXTURE_ENV_COLOR is
		float vColor[4] = {	cMath::Clamp(aColor.r, 0.0f, 1.0f), cMath::Clamp(aColor.g, 0.0f, 1.0f),
							cMath::Clamp(aColor.b, 0.0f, 1.0f), cMath::Clamp(aColor.a, 0.0f, 1.0f)};
		bool bSame = true;
		for(int i=0; i<4; ++i) bSame = bSame && vColor[i] == mvConstants[alUnit][i];
		if(bSame) return;
		memcpy(mvConstants[alUnit], vColor, sizeof(vColor));
		++mlConstantVersion;
	}

	//-----------------------------------------------------------------------

	void cFixedFunctionShaders::SetClipPlane(int alIdx, const cPlanef &aEyePlane)
	{
		if(alIdx < 0 || alIdx >= kMaxClipPlanes) return;
		mvClipPlanes[alIdx][0] = aEyePlane.a;
		mvClipPlanes[alIdx][1] = aEyePlane.b;
		mvClipPlanes[alIdx][2] = aEyePlane.c;
		mvClipPlanes[alIdx][3] = aEyePlane.d;
		mbClipPlanesChanged = true;
	}

	//-----------------------------------------------------------------------

	GLuint cFixedFunctionShaders::GetVertexStage(const cMatrixf &a_mtxModelView, const cMatrixf &a_mtxProjection,
												bool abMatricesChanged)
	{
		if(abMatricesChanged)
		{
			// cMatrixf is row-major
			cMatrixf mtxModelViewProj = cMath::MatrixMul(a_mtxProjection, a_mtxModelView);
			glProgramUniformMatrix4fv(mlVertexStage, mlModelViewLoc, 1, GL_TRUE, &a_mtxModelView.m[0][0]);
			glProgramUniformMatrix4fv(mlVertexStage, mlModelViewProjLoc, 1, GL_TRUE, &mtxModelViewProj.m[0][0]);
		}
		if(mbClipPlanesChanged)
		{
			glProgramUniform4fv(mlVertexStage, mlClipPlaneLoc, kMaxClipPlanes, &mvClipPlanes[0][0]);
			mbClipPlanesChanged = false;
		}
		return mlVertexStage;
	}

	//-----------------------------------------------------------------------

	GLuint cFixedFunctionShaders::GetFragmentStage()
	{
		if(mbKeyChanged)
		{
			mbKeyChanged = false;
			tString sKey = GetKey();
			auto it = m_mapFragmentStages.find(sKey);
			if(it == m_mapFragmentStages.end())
			{
				cFragmentStage stage;
				stage.mlProgram = CreateFragmentStage();
				stage.mlConstantLoc = stage.mlProgram ? glGetUniformLocation(stage.mlProgram, "hpl_ffConstant") : -1;
				stage.mlConstantVersion = 0;
				if(stage.mlProgram) cGpuProfiler::Label(GL_PROGRAM, stage.mlProgram, "fixed-function " + sKey);
				it = m_mapFragmentStages.insert(std::make_pair(sKey, stage)).first;
			}
			mpFragmentStage = &it->second;
		}

		if(mpFragmentStage->mlConstantLoc >= 0 && mpFragmentStage->mlConstantVersion != mlConstantVersion)
		{
			glProgramUniform4fv(mpFragmentStage->mlProgram, mpFragmentStage->mlConstantLoc, MAX_TEXTUREUNITS,
								&mvConstants[0][0]);
			mpFragmentStage->mlConstantVersion = mlConstantVersion;
		}
		return mpFragmentStage->mlProgram;
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	// Units with texturing off do not count: they pass the colour on.
	tString cFixedFunctionShaders::GetKey()
	{
		tString sKey;
		for(int i=0; i<MAX_TEXTUREUNITS; ++i)
		{
			const cUnit &unit = mvUnits[i];
			if(unit.mlTarget < 0) continue;

			char sUnit[64];
			if(unit.mbCombine)
			{
				snprintf(sUnit, sizeof(sUnit), "%d:%d:c%d%d%d%d%d%d%d%d%d%d%d%d%d%d%g%g ", i, unit.mlTarget,
						unit.mlColorFunc, unit.mlAlphaFunc,
						unit.mvColorSource[0], unit.mvColorSource[1], unit.mvColorSource[2],
						unit.mvAlphaSource[0], unit.mvAlphaSource[1], unit.mvAlphaSource[2],
						unit.mvColorOp[0], unit.mvColorOp[1], unit.mvColorOp[2],
						unit.mvAlphaOp[0], unit.mvAlphaOp[1], unit.mvAlphaOp[2],
						unit.mfColorScale, unit.mfAlphaScale);
			}
			else
			{
				snprintf(sUnit, sizeof(sUnit), "%d:%d:m ", i, unit.mlTarget);
			}
			sKey += sUnit;
		}
		return sKey;
	}

	//-----------------------------------------------------------------------

	tString cFixedFunctionShaders::GenerateFragmentSource()
	{
		tString sDecl, sMain;

		sDecl += "in vec4 hpl_ffColor;\n";
		sDecl += "uniform vec4 hpl_ffConstant[" + cString::ToString(MAX_TEXTUREUNITS) + "];\n";
		sDecl += "OUTPUT\n";

		sMain += "void main()\n{\n";
		sMain += "	vec4 prev = hpl_ffColor;\n";
		sMain += "	vec4 tex;\n";

		for(int i=0; i<MAX_TEXTUREUNITS; ++i)
		{
			const cUnit &unit = mvUnits[i];
			if(unit.mlTarget < 0) continue;

			tString sUnit = cString::ToString(i);
			tString sSampler = "hpl_ffTex" + sUnit;
			sDecl += tString("uniform ") + SamplerType(unit.mlTarget) + " " + sSampler + ";\n";

			// The coordinates are divided by q, as GL did; cube maps use s, t, r.
			tString sCoord = "vec4(0.0, 0.0, 0.0, 1.0)";
			if(i < kTexCoordUnits)
			{
				sCoord = "hpl_ffTexCoord" + sUnit;
				sDecl += "in vec4 " + sCoord + ";\n";
			}
			if(unit.mlTarget == eTextureTarget_CubeMap)
				sMain += "	tex = texture(" + sSampler + ", " + sCoord + ".xyz);\n";
			else
				sMain += "	tex = textureProj(" + sSampler + ", " + sCoord + ");\n";

			if(unit.mbCombine==false)
			{
				// GL_MODULATE. Textures are luminance, luminance-alpha, RGB or
				// RGBA, for which this is what GL did.
				sMain += "	prev = prev * tex;\n";
				continue;
			}

			tString vColorArgs[3], vAlphaArgs[3];
			for(int j=0; j<3; ++j)
			{
				vColorArgs[j] = ColorArg(unit.mvColorSource[j], unit.mvColorOp[j], i);
				vAlphaArgs[j] = AlphaArg(unit.mvAlphaSource[j], unit.mvAlphaOp[j], i);
			}

			tString sColor = CombineFunc(unit.mlColorFunc, vColorArgs[0], vColorArgs[1], vColorArgs[2], "vec3");
			tString sAlpha = CombineFunc(unit.mlAlphaFunc, vAlphaArgs[0], vAlphaArgs[1], vAlphaArgs[2], "float");

			char sScale[64];
			snprintf(sScale, sizeof(sScale), "%.1f", unit.mfColorScale);
			sMain += "	{\n";
			sMain += "		vec3 color = (" + sColor + ") * " + sScale + ";\n";
			if(unit.mlColorFunc == eTextureFunc_Dot3RGBA)
			{
				// The dot product goes to alpha too; the alpha function is not used.
				sMain += "		float alpha = color.r;\n";
			}
			else
			{
				snprintf(sScale, sizeof(sScale), "%.1f", unit.mfAlphaScale);
				sMain += "		float alpha = (" + sAlpha + ") * " + sScale + ";\n";
			}
			sMain += "		prev = clamp(vec4(color, alpha), 0.0, 1.0);\n";
			sMain += "	}\n";
		}

		sMain += "	outColor = prev;\n";
		sMain += "}\n";

		return sDecl + "\n" + sMain;
	}

	//-----------------------------------------------------------------------

	GLuint cFixedFunctionShaders::CreateFragmentStage()
	{
		tString sKey = GetKey();
		GLuint lProgram = cGLSLProgram::CompileSource(GL_FRAGMENT_SHADER, GenerateFragmentSource(),
														"fixed-function fragment (" + sKey + ")", true);
		if(lProgram==0) return 0;

		for(int i=0; i<MAX_TEXTUREUNITS; ++i)
		{
			if(mvUnits[i].mlTarget < 0) continue;
			tString sName = "hpl_ffTex" + cString::ToString(i);
			GLint lLoc = glGetUniformLocation(lProgram, sName.c_str());
			if(lLoc >= 0) glProgramUniform1i(lProgram, lLoc, i);
		}
		return lProgram;
	}

	//-----------------------------------------------------------------------

}
