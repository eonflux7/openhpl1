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
#include "impl/GLSLProgram.h"
#include "impl/SDLTexture.h"
#include "impl/GpuProfiler.h"
#include "graphics/LowLevelGraphics.h"
#include "system/LowLevelSystem.h"
#include "system/String.h"
#include "math/Math.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace hpl{

	//////////////////////////////////////////////////////////////////////////
	// STATIC DATA
	//////////////////////////////////////////////////////////////////////////

	tString cGLSLProgram::msShaderDir = "shaders/";
	GLuint cGLSLProgram::mlPipeline = 0;
	iLowLevelGraphics* cGLSLProgram::mpLowLevelGraphics = nullptr;
	bool cGLSLProgram::mbReloadEnabled = false;
	std::set<cGLSLProgram*> cGLSLProgram::msetPrograms;
	std::vector<cGLSLProgram::cSourceFile> cGLSLProgram::mvStageSources;
	cGLSLProgram* cGLSLProgram::mpBoundVertex = nullptr;
	cGLSLProgram* cGLSLProgram::mpBoundFragment = nullptr;
	GLuint cGLSLProgram::mlPipelineVertex = 0;
	GLuint cGLSLProgram::mlPipelineFragment = 0;

	// The vertex attributes, at the locations of cGLSLProgram::eVertexAttrib.
	static const char* kVertexPreamble =
		"#version 330 core\n"
		"#define VARYING out\n"
		"layout(location = 0) in vec4 hpl_Vertex;\n"
		"layout(location = 1) in vec3 hpl_Normal;\n"
		"layout(location = 2) in vec4 hpl_Color;\n"
		"layout(location = 4) in vec4 hpl_MultiTexCoord0;\n"
		"layout(location = 5) in vec4 hpl_MultiTexCoord1;\n"
		"layout(location = 6) in vec4 hpl_MultiTexCoord2;\n"
		"layout(location = 7) in vec4 hpl_MultiTexCoord3;\n"
		"layout(location = 8) in vec4 hpl_MultiTexCoord4;\n";

	// The shader's main() is called by the one in kFragmentEpilogue.
	static const char* kFragmentPreamble =
		"#version 330 core\n"
		"#define VARYING in\n"
		"#define OUTPUT layout(location = 0) out vec4 outColor;\n"
		"#define main hpl_main\n";

	// For the programs the renderer draws with (not the full-screen passes):
	// the state set through iLowLevelGraphics that fragment shaders act on
	// (uniform buffer binding kStateBlockBinding), and the colour
	// conversions for linear lighting. They return the colour as it is
	// when the target does not hold linear values.
	static const char* kFragmentStatePreamble =
		"#define HPL_STATE\n"
		"layout(std140) uniform hpl_State\n"
		"{\n"
		"	vec4 hpl_AlphaTest; // x: eAlphaTestFunc + 1, 0 for off, 9 for \"at least\n"
		"						// the reference\" as coverage; y: reference;\n"
		"						// z: how far alpha is matched to the original look\n"
		"						// when it blends into a linear scene, else 0;\n"
		"						// w: scale for the light of what is added to a\n"
		"						// linear scene, at full brightness\n"
		"	vec4 hpl_ColorSpace; // x: 1 when the target holds linear values;\n"
		"						// y: scale for the converted colour;\n"
		"						// z: power for amounts of light;\n"
		"						// w: 1 when a copy of a linear scene is drawn into it\n"
		"	vec4 hpl_ShadowMap0; // the shadow map of the light that is drawn, see\n"
		"	vec4 hpl_ShadowMap1; // cShadowMapParams and hpl1_shadow.glsl\n"
		"	vec4 hpl_ShadowMap2;\n"
		"	vec4 hpl_ShadowMap3;\n"
		"	vec4 hpl_Light0; // the look of the light that is drawn, see cLightLook\n"
		"	vec4 hpl_Light1; // and hpl1_light_look.glsl\n"
		"	vec4 hpl_Light2;\n"
		"};\n"
		"vec3 hpl_ToLinear(vec3 c)\n"
		"{\n"
		"	return hpl_ColorSpace.x > 0.5 ? pow(max(c, 0.0), vec3(2.2)) : c;\n"
		"}\n"
		"vec3 hpl_ToEncoded(vec3 c)\n"
		"{\n"
		"	return hpl_ColorSpace.x > 0.5 ? pow(max(c, 0.0), vec3(1.0 / 2.2)) : c;\n"
		"}\n";

	// After every fragment shader: the colour is clamped to 0..1, as the
	// fixed-function pipeline did, and alpha tested as set with
	// iLowLevelGraphics::SetAlphaTestActive/Func.
	//
	// In the multisampled scene the test is made for each sample instead
	// (kAlphaToCoverage, with GL_SAMPLE_ALPHA_TO_COVERAGE): the alpha that
	// is written is how far the fragment is past the reference, over the
	// width of one pixel, and as many of the pixel's samples are drawn. The
	// outline of what the texture cuts out is anti-aliased as the edge of a
	// triangle is.
	//
	// Shaders work on encoded (screen) colours, as the original ones do.
	// With linear lighting the result is converted here, so each draw gives
	// the colour it always did and only the blending with what is in the
	// target happens on linear values. A shader that reads or writes linear
	// values itself defines HPL_LINEAR_COLOR: its result is used as it is,
	// and not clamped at 1. Neither is a copy of the linear scene drawn back
	// into it (iLowLevelGraphics::SetLinearSource). HPL_NO_CLAMP leaves the
	// result unclamped at 1 without linear lighting too.
	//
	// What is added to the scene (halos, lit parts, additive particles) is
	// scaled by the glow value in proportion to its own brightness: the
	// bright middle of a halo goes past white, its faint edge stays as
	// faint as it was.
	//
	// Alpha blending is the one mix that the conversion cannot keep: the
	// same alpha covers more on linear values when something light is
	// blended over something dark (smoke, snow, fog), and less the other
	// way round. The amounts were made by eye on screen values, so the
	// alpha is replaced by the one that gives the original result over a
	// backdrop of an assumed, dark, screen value. Over that backdrop the
	// match is exact; over others it is closer than the alpha as it is.
	static const char* kFragmentEpilogue =
		"#undef main\n"
		"void main()\n"
		"{\n"
		"	hpl_main();\n"
		"#if defined(HPL_NO_CLAMP)\n"
		"	outColor = max(outColor, 0.0);\n"
		"#elif defined(HPL_STATE) && defined(HPL_LINEAR_COLOR)\n"
		"	if(hpl_ColorSpace.x > 0.5 || hpl_ColorSpace.w > 0.5)	outColor = max(outColor, 0.0);\n"
		"	else													outColor = clamp(outColor, 0.0, 1.0);\n"
		"#elif defined(HPL_STATE)\n"
		"	if(hpl_ColorSpace.w > 0.5)	outColor = max(outColor, 0.0);\n"
		"	else						outColor = clamp(outColor, 0.0, 1.0);\n"
		"#else\n"
		"	outColor = clamp(outColor, 0.0, 1.0);\n"
		"#endif\n"
		"#if defined(HPL_STATE) && !defined(HPL_LINEAR_COLOR)\n"
		"	float hpl_encodedLuma = dot(outColor.rgb, vec3(0.3, 0.58, 0.12));\n"
		"	if(hpl_ColorSpace.x > 0.5)\n"
		"		outColor.rgb = pow(outColor.rgb, vec3(2.2)) * hpl_ColorSpace.y *\n"
		"						mix(1.0, hpl_AlphaTest.w, hpl_encodedLuma);\n"
		"#endif\n"
		"#ifdef HPL_STATE\n"
		"	int func = int(hpl_AlphaTest.x);\n"
		"	float alpha = outColor.a, ref = hpl_AlphaTest.y;\n"
		"	if(func == 9)\n"
		"	{\n"
		"		outColor.a = clamp((alpha - ref) / max(fwidth(alpha), 0.0001) + 0.5, 0.0, 1.0);\n"
		"		if(outColor.a <= 0.0) discard;\n"
		"	}\n"
		"	else if(func == 1 ||\n"
		"		(func == 2 && !(alpha < ref)) || (func == 3 && !(alpha <= ref)) ||\n"
		"		(func == 4 && !(alpha > ref)) || (func == 5 && !(alpha >= ref)) ||\n"
		"		(func == 6 && !(alpha == ref)) || (func == 7 && !(alpha != ref)))\n"
		"		discard;\n"
		"#endif\n"
		"#if defined(HPL_STATE) && !defined(HPL_LINEAR_COLOR)\n"
		"	if(hpl_AlphaTest.z > 0.0)\n"
		"	{\n"
		"		const float backdrop = 0.15;\n"
		"		float span = pow(hpl_encodedLuma, 2.2) - pow(backdrop, 2.2);\n"
		"		if(abs(span) > 0.001)\n"
		"		{\n"
		"			float matched = (pow(mix(backdrop, hpl_encodedLuma, alpha), 2.2) - pow(backdrop, 2.2)) / span;\n"
		"			outColor.a = mix(alpha, clamp(matched, 0.0, 1.0), hpl_AlphaTest.z);\n"
		"		}\n"
		"	}\n"
		"#endif\n"
		"}\n";

	// Programs that are variants of a shared file: the engine's program name
	// (without "_vp"/"_fp"), the file (without "hpl1_" and the extension) and
	// the #defines it is compiled with. Other names load the file of the
	// same name.
	struct cShaderVariant
	{
		const char *msName;
		const char *msFile;
		const char *msDefines;
	};

	static const cShaderVariant kShaderVariants[] = {
		{"Diffuse_Light",				"light",	""},
		{"Diffuse_Light_Spot",			"light",	"SPOT"},
		{"DiffuseSpec_Light",			"light",	"SPECULAR"},
		{"DiffuseSpec_Light_Spot",		"light",	"SPECULAR SPOT"},
		{"Bump_Light",					"light",	"NORMAL_MAP"},
		{"Bump_Light_Spot",				"light",	"NORMAL_MAP SPOT"},
		{"BumpSpec_Light",				"light",	"NORMAL_MAP SPECULAR"},
		{"BumpSpec_Light_Spot",			"light",	"NORMAL_MAP SPECULAR SPOT"},
		{"BumpColorSpec_Light",			"light",	"NORMAL_MAP SPECULAR SPECULAR_MAP"},
		{"BumpColorSpec_Light_Spot",	"light",	"NORMAL_MAP SPECULAR SPECULAR_MAP SPOT"},
		{"Ambient_Shaded",				"ambient",	""},
		{"Ambient_Shaded_Spec",			"ambient",	"SPECULAR"},
		{"Ambient_Shaded_Bump",			"ambient",	"NORMAL_MAP"},
		{"Ambient_Shaded_Bump_Spec",	"ambient",	"NORMAL_MAP SPECULAR"},
		{"Ambient_Shaded_Bump_ColorSpec",	"ambient",	"NORMAL_MAP SPECULAR SPECULAR_MAP"},
		{"Fog_Trans_Alpha",				"Fog_Trans",	""},
		{"Fog_Trans_Mod",				"Fog_Trans",	"MOD"},
		{"Fog_Trans_ModX2",				"Fog_Trans",	"MODX2"},
		{"PostEffect_DoF_Sky",			"PostEffect_DoF",	"SKY"},
		{"PostEffect_BloomChain_Prefilter",	"PostEffect_BloomChain",	"PREFILTER"},
		{"PostEffect_BloomChain_Down",		"PostEffect_BloomChain",	"DOWN"},
		{"PostEffect_BloomChain_Up",		"PostEffect_BloomChain",	"UP"},
		{"PostEffect_BloomChain_Composite",	"PostEffect_BloomChain",	"COMPOSITE"},
		{"PostEffect_AO_Deinterleave",	"PostEffect_AO",	"DEINTERLEAVE"},
		{"PostEffect_AO_Occlusion",		"PostEffect_AO",	"OCCLUSION"},
		{"PostEffect_AO_Occlusion_2x2",	"PostEffect_AO",	"OCCLUSION SAMPLES_2X2"},
		{"PostEffect_AO_Occlusion_2x4",	"PostEffect_AO",	"OCCLUSION SAMPLES_2X4"},
		{"PostEffect_AO_Occlusion_3x6",	"PostEffect_AO",	"OCCLUSION SAMPLES_3X6"},
		{"PostEffect_AO_Occlusion_4x10",	"PostEffect_AO",	"OCCLUSION SAMPLES_4X10"},
		{"PostEffect_AO_Occlusion_6x16",	"PostEffect_AO",	"OCCLUSION SAMPLES_6X16"},
		{"PostEffect_AO_Filter",		"PostEffect_AO",	"FILTER"},
		{"PostEffect_AO_Composite",		"PostEffect_AO",	"COMPOSITE"},
		{"PostEffect_AO_Composite_Scene",	"PostEffect_AO",	"COMPOSITE SCENE"},
		{"PostEffect_AO_Composite_SceneMS",	"PostEffect_AO",	"COMPOSITE SCENE_MULTISAMPLE"},
		{"PostEffect_Bokeh_Prefilter",	"PostEffect_Bokeh",	"PREFILTER"},
		{"PostEffect_Bokeh_Gather",		"PostEffect_Bokeh",	"GATHER"},
		{"PostEffect_Bokeh_Fill",		"PostEffect_Bokeh",	"FILL"},
		{"PostEffect_Bokeh_Composite",	"PostEffect_Bokeh",	"COMPOSITE"},
	};

	static const int kMaxIncludeDepth = 8;

	static const int kMaxSamplers = 8;

	//-----------------------------------------------------------------------

	static bool ReadTextFile(const tString& asPath, tString& asOut)
	{
		FILE *pFile = fopen(asPath.c_str(), "rb");
		if(pFile==nullptr) return false;

		fseek(pFile,0,SEEK_END);
		long lSize = ftell(pFile);
		fseek(pFile,0,SEEK_SET);

		asOut.resize(lSize);
		size_t lRead = fread(&asOut[0],1,lSize,pFile);
		fclose(pFile);

		return lRead == (size_t)lSize;
	}

	//-----------------------------------------------------------------------

	// "core/programs/Diffuse_Light_Spot_fp.cg" -> "hpl1_light", "SPOT"
	static tString CgNameToShaderBase(const tString& asCgFile, tString& asDefines)
	{
		tString sName = cString::GetFileName(asCgFile);
		sName = cString::SetFileExt(sName,"");

		size_t lPos = sName.find("_vp");
		if(lPos == tString::npos) lPos = sName.find("_fp");
		if(lPos != tString::npos) sName.erase(lPos,3);

		asDefines = "";
		for(const cShaderVariant& variant : kShaderVariants)
		{
			if(sName == variant.msName)
			{
				sName = variant.msFile;
				asDefines = variant.msDefines;
				break;
			}
		}

		return "hpl1_" + sName;
	}

	//-----------------------------------------------------------------------

	// Appends asFile with its #include "name" lines replaced by the named
	// files from the shader directory, each file once. Every file gets its
	// own source number in #line, so compile errors read "<number>(<line>)"
	// with the number indexing avFiles.
	static bool ExpandIncludes(const tString& asDir, const tString& asFile,
								std::vector<tString>& avFiles, tString& asOut, int alDepth)
	{
		tString sSource;
		if(ReadTextFile(asDir + asFile, sSource)==false)
		{
			Error("GLSL: Couldn't read '%s'\n",(asDir + asFile).c_str());
			return false;
		}

		int lFileNum = (int)avFiles.size();
		avFiles.push_back(asFile);

		char sLine[64];
		snprintf(sLine, sizeof(sLine), "#line 1 %d\n", lFileNum);
		asOut += sLine;

		size_t lStart = 0;
		int lLineNum = 1;
		while(lStart < sSource.size())
		{
			size_t lEnd = sSource.find('\n', lStart);
			if(lEnd == tString::npos) lEnd = sSource.size();
			tString sText = sSource.substr(lStart, lEnd - lStart);
			lStart = lEnd + 1;

			size_t lFirst = sText.find_first_not_of(" \t");
			if(lFirst == tString::npos || sText.compare(lFirst, 8, "#include") != 0)
			{
				asOut += sText;
				asOut += "\n";
				++lLineNum;
				continue;
			}

			size_t lOpen = sText.find('"', lFirst);
			size_t lClose = lOpen == tString::npos ? tString::npos : sText.find('"', lOpen + 1);
			if(lClose == tString::npos)
			{
				Error("GLSL: %s(%d): expected #include \"file\"\n", asFile.c_str(), lLineNum);
				return false;
			}
			tString sInclude = sText.substr(lOpen + 1, lClose - lOpen - 1);

			bool bIncluded = false;
			for(const tString& sFile : avFiles) bIncluded = bIncluded || sFile == sInclude;
			if(!bIncluded)
			{
				if(alDepth >= kMaxIncludeDepth)
				{
					Error("GLSL: %s(%d): includes nested too deep\n", asFile.c_str(), lLineNum);
					return false;
				}
				if(!ExpandIncludes(asDir, sInclude, avFiles, asOut, alDepth + 1)) return false;
			}

			++lLineNum;
			snprintf(sLine, sizeof(sLine), "#line %d %d\n", lLineNum, lFileNum);
			asOut += sLine;
		}

		return true;
	}

	//////////////////////////////////////////////////////////////////////////
	// CONSTRUCTORS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	cGLSLProgram::cGLSLProgram(tString asName,eGpuProgramType aType)
	: iGpuProgram(asName, aType)
	{
		msName = asName;
		mlProgram = 0;
		mlCompanionFragment = 0;
		msetPrograms.insert(this);
	}

	cGLSLProgram::~cGLSLProgram()
	{
		if(mpBoundVertex == this) mpBoundVertex = nullptr;
		if(mpBoundFragment == this) mpBoundFragment = nullptr;
		msetPrograms.erase(this);

		if(mlProgram) glDeleteProgram(mlProgram);
		if(mlCompanionFragment) glDeleteProgram(mlCompanionFragment);
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	bool cGLSLProgram::InitPipeline(iLowLevelGraphics *apLowLevelGraphics)
	{
		mpLowLevelGraphics = apLowLevelGraphics;

		const char *pReload = getenv("OVERTURE_SHADER_RELOAD");
		mbReloadEnabled = pReload && pReload[0] && tString(pReload) != "0";
		if(mbReloadEnabled) Log("  Shaders are reloaded when their files change (%s)\n", msShaderDir.c_str());

		if(!GLAD_GL_VERSION_4_1 && !GLAD_GL_ARB_separate_shader_objects)
		{
			Error("OpenGL separate shader objects are not supported!\n");
			return false;
		}

		// Bound from now on; draws that put their own programs in use put
		// it back.
		glGenProgramPipelines(1,&mlPipeline);
		glBindProgramPipeline(mlPipeline);
		cGpuProfiler::Label(GL_PROGRAM_PIPELINE, mlPipeline, "Engine programs");
		glUseProgram(0);
		mlPipelineVertex = mlPipelineFragment = 0;
		return true;
	}

	void cGLSLProgram::ExitPipeline()
	{
		if(mlPipeline) glDeleteProgramPipelines(1,&mlPipeline);
		mlPipeline = 0;
	}

	//-----------------------------------------------------------------------

	GLuint cGLSLProgram::CreateStage(GLenum aStage, const tString& asName, const tString& asDefines)
	{
		std::vector<tString> vFiles;
		GLuint lProgram = CompileStage(aStage, "hpl1_" + asName +
									(aStage == GL_VERTEX_SHADER ? ".vertex" : ".fragment"), asDefines, false, &vFiles);
		WatchFiles(mvStageSources, vFiles);
		return lProgram;
	}

	//-----------------------------------------------------------------------

	GLuint cGLSLProgram::CompileSource(GLenum aStage, const tString& asSource, const tString& asName,
										bool abState)
	{
		tString sSource = aStage == GL_VERTEX_SHADER ? kVertexPreamble : kFragmentPreamble;
		if(aStage == GL_FRAGMENT_SHADER && abState) sSource += kFragmentStatePreamble;
		sSource += "#line 1 1\n";
		sSource += asSource;

		std::vector<tString> vFiles;
		vFiles.push_back("(preamble)");
		vFiles.push_back(asName);
		return LinkStage(aStage, sSource, asName, vFiles);
	}

	//-----------------------------------------------------------------------

	GLuint cGLSLProgram::GetVertexStage()
	{
		return mpBoundVertex ? mpBoundVertex->mlProgram : 0;
	}

	GLuint cGLSLProgram::GetFragmentStage()
	{
		if(mpBoundFragment) return mpBoundFragment->mlProgram;
		if(mpBoundVertex)	return mpBoundVertex->mlCompanionFragment;
		return 0;
	}

	//-----------------------------------------------------------------------

	void cGLSLProgram::SetPipelineStages(GLuint alVertex, GLuint alFragment)
	{
		if(alVertex == mlPipelineVertex && alFragment == mlPipelineFragment) return;

		if(alVertex != mlPipelineVertex) glUseProgramStages(mlPipeline, GL_VERTEX_SHADER_BIT, alVertex);
		if(alFragment != mlPipelineFragment) glUseProgramStages(mlPipeline, GL_FRAGMENT_SHADER_BIT, alFragment);
		mlPipelineVertex = alVertex;
		mlPipelineFragment = alFragment;

		if(mbDebugInfo)
		{
			static int lReported = 0;
			glValidateProgramPipeline(mlPipeline);
			GLint lValid = GL_TRUE;
			glGetProgramPipelineiv(mlPipeline, GL_VALIDATE_STATUS, &lValid);
			if(!lValid && lReported < 10)
			{
				++lReported;
				char sLog[2048] = "";
				glGetProgramPipelineInfoLog(mlPipeline, sizeof(sLog), nullptr, sLog);
				Log("GLSL: invalid pipeline (vertex '%s', fragment '%s'): %s\n",
					mpBoundVertex ? mpBoundVertex->msName.c_str() : "-",
					mpBoundFragment ? mpBoundFragment->msName.c_str() : "-", sLog);
			}
		}
	}

	//-----------------------------------------------------------------------

	bool cGLSLProgram::ReloadChangedFiles()
	{
		if(!mbReloadEnabled) return false;

		static std::chrono::steady_clock::time_point lastCheck;
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		if(now - lastCheck < std::chrono::milliseconds(500)) return false;
		lastCheck = now;

		bool bChanged = FilesChanged(mvStageSources);
		for(cGLSLProgram *pProgram : msetPrograms)
		{
			if(!FilesChanged(pProgram->mvSources)) continue;
			bChanged = true;
			if(pProgram->CompileProgram())
				Log("GLSL: reloaded '%s'\n", pProgram->msName.c_str());
		}
		return bChanged;
	}

	//-----------------------------------------------------------------------

	bool cGLSLProgram::Reload()
	{
		return false;
	}

	void cGLSLProgram::Unload()
	{
	}

	void cGLSLProgram::Destroy()
	{
	}

	//-----------------------------------------------------------------------

	bool cGLSLProgram::CreateFromFile(const tString &asFile, const tString &asEntry)
	{
		msBase = CgNameToShaderBase(asFile, msDefines);

		if(CompileProgram()==false)
		{
			Error("GLSL: Couldn't create program for '%s'\n",asFile.c_str());
			return false;
		}

		if(mbDebugInfo) Log("GLSL: '%s' -> '%s' (%s)\n",msName.c_str(),msBase.c_str(),msDefines.c_str());

		return true;
	}

	//-----------------------------------------------------------------------

	// Compiles msBase with msDefines. On failure the current version, if
	// any, is kept.
	bool cGLSLProgram::CompileProgram()
	{
		std::vector<tString> vFiles;
		GLuint lProgram = 0, lCompanion = 0;
		bool bOk;

		if(mProgramType == eGpuProgramType_Vertex)
		{
			lProgram = CompileStage(GL_VERTEX_SHADER, msBase + ".vertex", msDefines, true, &vFiles);
			// Optional: not every vertex shader has a companion.
			bool bHasCompanion = std::filesystem::exists(msShaderDir + msBase + ".fragment");
			if(bHasCompanion)
				lCompanion = CompileStage(GL_FRAGMENT_SHADER, msBase + ".fragment", msDefines, true, &vFiles);
			bOk = lProgram != 0 && (lCompanion != 0 || !bHasCompanion);
		}
		else
		{
			lProgram = CompileStage(GL_FRAGMENT_SHADER, msBase + ".fragment", msDefines, true, &vFiles);
			bOk = lProgram != 0;
		}

		WatchFiles(mvSources, vFiles);

		if(!bOk)
		{
			if(lProgram) glDeleteProgram(lProgram);
			if(lCompanion) glDeleteProgram(lCompanion);
			return false;
		}

		if(mlProgram) glDeleteProgram(mlProgram);
		if(mlCompanionFragment) glDeleteProgram(mlCompanionFragment);
		mlProgram = lProgram;
		mlCompanionFragment = lCompanion;
		// Uniform values are lost with the old program; most are set again
		// for every draw. The next draw puts the new one in the pipeline.
		m_mapUniforms.clear();

		return true;
	}

	//-----------------------------------------------------------------------

	// Adds avFiles (relative to the shader directory) to avWatched with
	// their current times.
	void cGLSLProgram::WatchFiles(std::vector<cSourceFile>& avWatched, const std::vector<tString>& avFiles)
	{
		if(!mbReloadEnabled) return;

		for(const tString& sFile : avFiles)
		{
			tString sPath = msShaderDir + sFile;
			bool bFound = false;
			for(const cSourceFile& watched : avWatched) bFound = bFound || watched.msPath == sPath;
			if(bFound) continue;

			std::error_code err;
			avWatched.push_back({sPath, std::filesystem::last_write_time(sPath, err)});
		}
	}

	// True if a file's time changed since the last call; updates the times.
	bool cGLSLProgram::FilesChanged(std::vector<cSourceFile>& avWatched)
	{
		bool bChanged = false;
		for(cSourceFile& watched : avWatched)
		{
			std::error_code err;
			std::filesystem::file_time_type time = std::filesystem::last_write_time(watched.msPath, err);
			if(err) continue; // mid-save, or deleted
			if(time != watched.mTime) bChanged = true;
			watched.mTime = time;
		}
		return bChanged;
	}

	//-----------------------------------------------------------------------

	// The pipeline gets the stages at the next draw, see SetPipelineStages().
	void cGLSLProgram::Bind()
	{
		if(mProgramType == eGpuProgramType_Vertex)	mpBoundVertex = this;
		else										mpBoundFragment = this;
	}

	//-----------------------------------------------------------------------

	void cGLSLProgram::UnBind()
	{
		if(mProgramType == eGpuProgramType_Vertex)
			mpBoundVertex = nullptr;
		else if(mpBoundFragment == this)
			mpBoundFragment = nullptr;
	}

	//-----------------------------------------------------------------------

	bool cGLSLProgram::SetFloat(const tString& asName, float afX)
	{
		GLint lLoc = GetUniform(asName);
		if(lLoc < 0) return false;
		glProgramUniform1f(mlProgram, lLoc, afX);
		return true;
	}

	bool cGLSLProgram::SetVec2f(const tString& asName, float afX,float afY)
	{
		GLint lLoc = GetUniform(asName);
		if(lLoc < 0) return false;
		glProgramUniform2f(mlProgram, lLoc, afX, afY);
		return true;
	}

	bool cGLSLProgram::SetVec3f(const tString& asName, float afX,float afY,float afZ)
	{
		GLint lLoc = GetUniform(asName);
		if(lLoc < 0) return false;
		glProgramUniform3f(mlProgram, lLoc, afX, afY, afZ);
		return true;
	}

	bool cGLSLProgram::SetVec4f(const tString& asName, float afX,float afY,float afZ, float afW)
	{
		GLint lLoc = GetUniform(asName);
		if(lLoc < 0) return false;
		glProgramUniform4f(mlProgram, lLoc, afX, afY, afZ, afW);
		return true;
	}

	//-----------------------------------------------------------------------

	bool cGLSLProgram::SetMatrixf(const tString& asName, const cMatrixf& mMtx)
	{
		GLint lLoc = GetUniform(asName);
		if(lLoc < 0) return false;
		// cMatrixf is row-major
		glProgramUniformMatrix4fv(mlProgram, lLoc, 1, GL_TRUE, &mMtx.m[0][0]);
		return true;
	}

	//-----------------------------------------------------------------------

	bool cGLSLProgram::SetMatrixf(const tString& asName, eGpuProgramMatrix mType,
									eGpuProgramMatrixOp mOp)
	{
		GLint lLoc = GetUniform(asName);
		if(lLoc < 0) return false;

		// Like cgGLSetStateMatrixParameter: the current matrices, as kept
		// by the low-level graphics.
		cMatrixf mtxResult;
		switch(mType)
		{
		case eGpuProgramMatrix_View:
			mtxResult = mpLowLevelGraphics->GetMatrix(eMatrix_ModelView);
			break;
		case eGpuProgramMatrix_Projection:
			mtxResult = mpLowLevelGraphics->GetMatrix(eMatrix_Projection);
			break;
		case eGpuProgramMatrix_Texture:
			mtxResult = mpLowLevelGraphics->GetMatrix(eMatrix_Texture);
			break;
		case eGpuProgramMatrix_ViewProjection:
			mtxResult = cMath::MatrixMul(mpLowLevelGraphics->GetMatrix(eMatrix_Projection),
										mpLowLevelGraphics->GetMatrix(eMatrix_ModelView));
			break;
		default:
			return false;
		}

		switch(mOp)
		{
		case eGpuProgramMatrixOp_Identity:			break;
		case eGpuProgramMatrixOp_Inverse:			mtxResult = cMath::MatrixInverse(mtxResult); break;
		case eGpuProgramMatrixOp_Transpose:			mtxResult = mtxResult.GetTranspose(); break;
		case eGpuProgramMatrixOp_InverseTranspose:	mtxResult = cMath::MatrixInverse(mtxResult).GetTranspose(); break;
		default:									break;
		}

		glProgramUniformMatrix4fv(mlProgram, lLoc, 1, GL_TRUE, &mtxResult.m[0][0]);
		return true;
	}

	//-----------------------------------------------------------------------

	bool cGLSLProgram::SetTexture(const tString& asName,iTexture* apTexture, bool abAutoDisable)
	{
		// Samplers are named texN and fixed to unit N.
		if(asName.size() < 4 || asName.compare(0,3,"tex") != 0) return false;
		int lUnit = cString::ToInt(asName.substr(3).c_str(),-1);
		if(lUnit < 0) return false;

		return SetTextureToUnit(lUnit, apTexture);
	}

	//-----------------------------------------------------------------------

	bool cGLSLProgram::SetTextureToUnit(int alUnit, iTexture* apTexture)
	{
		if(alUnit < 0 || alUnit >= kMaxSamplers) return false;
		if(apTexture==nullptr) return true;

		GLenum Target = GL_TEXTURE_2D;
		switch(apTexture->GetTarget())
		{
		case eTextureTarget_1D:			Target = GL_TEXTURE_1D; break;
		case eTextureTarget_2D:			Target = GL_TEXTURE_2D; break;
		case eTextureTarget_CubeMap:	Target = GL_TEXTURE_CUBE_MAP; break;
		case eTextureTarget_3D:			Target = GL_TEXTURE_3D; break;
		default: break;
		}

		GLint lPrevUnit;
		glGetIntegerv(GL_ACTIVE_TEXTURE, &lPrevUnit);
		glActiveTexture(GL_TEXTURE0 + alUnit);
		glBindTexture(Target, static_cast<cSDLTexture*>(apTexture)->GetTextureHandle());
		glActiveTexture(lPrevUnit);
		if(mpLowLevelGraphics)
			static_cast<cLowLevelGraphicsSDL*>(mpLowLevelGraphics)->BindSampler(alUnit,
												static_cast<cSDLTexture*>(apTexture)->GetSampler());

		return true;
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	GLint cGLSLProgram::GetUniform(const tString& asName)
	{
		if(mlProgram==0) return -1;

		std::map<tString,GLint>::iterator it = m_mapUniforms.find(asName);
		if(it != m_mapUniforms.end()) return it->second;

		GLint lLoc = glGetUniformLocation(mlProgram, asName.c_str());
		m_mapUniforms.insert(std::map<tString,GLint>::value_type(asName,lLoc));
		return lLoc;
	}

	//-----------------------------------------------------------------------

	GLuint cGLSLProgram::CompileStage(GLenum aStage, const tString& asFile, const tString& asDefines,
										bool abState, std::vector<tString> *apFiles)
	{
		tString sSource = aStage == GL_VERTEX_SHADER ? kVertexPreamble : kFragmentPreamble;
		if(aStage == GL_FRAGMENT_SHADER && abState) sSource += kFragmentStatePreamble;

		tStringVec vDefines;
		cString::GetStringVec(asDefines, vDefines, nullptr);
		for(const tString& sDefine : vDefines) sSource += "#define " + sDefine + "\n";

		// Source number 0 is the preamble above.
		std::vector<tString> vFiles(1, "(preamble)");
		bool bRead = ExpandIncludes(msShaderDir, asFile, vFiles, sSource, 0);
		if(apFiles) apFiles->insert(apFiles->end(), vFiles.begin() + 1, vFiles.end());
		if(!bRead) return 0;

		tString sName = asFile;
		if(asDefines != "") sName += " (" + asDefines + ")";

		return LinkStage(aStage, sSource, sName, vFiles);
	}

	//-----------------------------------------------------------------------

	GLuint cGLSLProgram::LinkStage(GLenum aStage, tString asSource, const tString& asName,
									const std::vector<tString>& avFiles)
	{
		if(aStage == GL_FRAGMENT_SHADER)
		{
			// Source number 0 is the preamble
			asSource += "#line 1 0\n";
			asSource += kFragmentEpilogue;
		}

		const tString &sName = asName;
		const std::vector<tString> &vFiles = avFiles;
		const char* pSource = asSource.c_str();
		GLuint lShader = glCreateShader(aStage);
		glShaderSource(lShader, 1, &pSource, nullptr);
		glCompileShader(lShader);

		GLint lOk = GL_FALSE;
		glGetShaderiv(lShader, GL_COMPILE_STATUS, &lOk);
		if(!lOk)
		{
			char sLog[4096];
			glGetShaderInfoLog(lShader, sizeof(sLog), nullptr, sLog);
			tString sFiles;
			for(size_t i=0; i<vFiles.size(); ++i)
				sFiles += "  " + cString::ToString((int)i) + " = " + vFiles[i] + "\n";
			Error("GLSL: Compiling '%s' failed:\n%s\nSource numbers:\n%s",sName.c_str(),sLog,sFiles.c_str());
			glDeleteShader(lShader);
			return 0;
		}

		GLuint lProgram = glCreateProgram();
		glProgramParameteri(lProgram, GL_PROGRAM_SEPARABLE, GL_TRUE);
		glAttachShader(lProgram, lShader);
		glLinkProgram(lProgram);
		glDetachShader(lProgram, lShader);
		glDeleteShader(lShader);

		glGetProgramiv(lProgram, GL_LINK_STATUS, &lOk);
		if(!lOk)
		{
			char sLog[4096];
			glGetProgramInfoLog(lProgram, sizeof(sLog), nullptr, sLog);
			Error("GLSL: Linking '%s' failed:\n%s\n",sName.c_str(),sLog);
			glDeleteProgram(lProgram);
			return 0;
		}

		cGpuProfiler::Label(GL_PROGRAM, lProgram, sName);

		GLuint lFrameBlock = glGetUniformBlockIndex(lProgram, "hpl_Frame");
		if(lFrameBlock != GL_INVALID_INDEX) glUniformBlockBinding(lProgram, lFrameBlock, kFrameBlockBinding);
		GLuint lStateBlock = glGetUniformBlockIndex(lProgram, "hpl_State");
		if(lStateBlock != GL_INVALID_INDEX) glUniformBlockBinding(lProgram, lStateBlock, kStateBlockBinding);

		// texN samplers always read texture unit N
		for(int i=0; i<kMaxSamplers; ++i)
		{
			char sName[8];
			snprintf(sName, sizeof(sName), "tex%d", i);
			GLint lLoc = glGetUniformLocation(lProgram, sName);
			if(lLoc >= 0) glProgramUniform1i(lProgram, lLoc, i);
		}
		// The shadow map of the light that is drawn, as a shadow sampler and
		// as its depth: on units of their own (hpl1_shadow.glsl)
		GLint lShadowLoc = glGetUniformLocation(lProgram, "hpl_ShadowMap");
		if(lShadowLoc >= 0) glProgramUniform1i(lProgram, lShadowLoc, kShadowMapUnit);
		lShadowLoc = glGetUniformLocation(lProgram, "hpl_ShadowMapDepth");
		if(lShadowLoc >= 0) glProgramUniform1i(lProgram, lShadowLoc, kShadowMapUnit + 1);
		GLint lDepthLoc = glGetUniformLocation(lProgram, "hpl_SceneDepth");
		if(lDepthLoc >= 0) glProgramUniform1i(lProgram, lDepthLoc, kSceneDepthUnit);
		GLint lColorLoc = glGetUniformLocation(lProgram, "hpl_SceneColor");
		if(lColorLoc >= 0) glProgramUniform1i(lProgram, lColorLoc, kSceneColorUnit);

		return lProgram;
	}

	//-----------------------------------------------------------------------
}
