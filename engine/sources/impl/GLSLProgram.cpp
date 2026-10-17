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
	iLowLevelGraphics* cGLSLProgram::mpLowLevelGraphics = nullptr;
	bool cGLSLProgram::mbReloadEnabled = false;
	std::set<cGLSLProgram*> cGLSLProgram::msetPrograms;
	cGLSLProgram* cGLSLProgram::mpBoundProgram = nullptr;

	// The vertex attributes, at the locations of cGLSLProgram::eVertexAttrib.
	static const char* kVertexPreamble =
		"#version 330 core\n"
		"#define VARYING out\n"
		"layout(location = 0) in vec4 hpl_Vertex;\n"
		"layout(location = 1) in vec3 hpl_Normal;\n"
		"layout(location = 2) in vec4 hpl_Color;\n"
		"layout(location = 4) in vec4 hpl_TexCoord;\n"
		"layout(location = 5) in vec4 hpl_Tangent;\n";

	// The shader's main() is called by the one in kFragmentEpilogue.
	static const char* kFragmentPreamble =
		"#version 330 core\n"
		"#define VARYING in\n"
		"#define OUTPUT layout(location = 0) out vec4 outColor;\n"
		"#define main hpl_main\n";

	// For the programs the renderer draws with (not the full-screen passes):
	// the state set through iLowLevelGraphics that fragment shaders act on
	// (uniform buffer binding kStateBlockBinding), and the colour
	// conversions for the linear scene. They return the colour as it is
	// when the target does not hold linear values (outside the scene).
	static const char* kFragmentStatePreamble =
		"#define HPL_STATE\n"
		"layout(std140) uniform hpl_State\n"
		"{\n"
		"	vec4 hpl_AlphaTest; // x: 1 to discard what has an alpha under y,\n"
		"						// 2 to do that as coverage, 0 for off;\n"
		"						// z: how far alpha is matched to the original look\n"
		"						// when it blends into the scene, else 0;\n"
		"						// w: scale for the light of what is added to the\n"
		"						// scene, at full brightness\n"
		"	vec4 hpl_ColorSpace; // x: 1 when the target holds linear values (the scene);\n"
		"						// y: scale for the converted colour;\n"
		"						// z: power for amounts of light; w: unused\n"
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
	// iLowLevelGraphics::SetAlphaCutoff.
	//
	// In the multisampled scene the test is made for each sample instead
	// (kAlphaToCoverage, with GL_SAMPLE_ALPHA_TO_COVERAGE): the alpha that
	// is written is how far the fragment is past the reference, over the
	// width of one pixel, and as many of the pixel's samples are drawn. The
	// outline of what the texture cuts out is anti-aliased as the edge of a
	// triangle is.
	//
	// Shaders work on encoded (screen) colours, as the original ones do.
	// In the scene, which holds linear values, the result is converted
	// here, so each draw gives the colour it always did and only the
	// blending with what is in the target happens on linear values. A
	// shader that reads or writes linear values itself defines
	// HPL_LINEAR_COLOR: its result is used as it is, and not clamped at 1.
	// HPL_NO_CLAMP leaves the result unclamped at 1 too.
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
		"#if defined(HPL_NO_CLAMP) || (defined(HPL_STATE) && defined(HPL_LINEAR_COLOR))\n"
		"	outColor = max(outColor, 0.0);\n"
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
		"	if(func == 2)\n"
		"	{\n"
		"		outColor.a = clamp((alpha - ref) / max(fwidth(alpha), 0.0001) + 0.5, 0.0, 1.0);\n"
		"		if(outColor.a <= 0.0) discard;\n"
		"	}\n"
		"	else if(func == 1 && !(alpha >= ref))\n"
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

	cGLSLProgram::cGLSLProgram(tString asName, bool abState)
	: iGpuProgram(asName)
	{
		msName = asName;
		mbState = abState;
		mlProgram = 0;
		msetPrograms.insert(this);
	}

	cGLSLProgram::~cGLSLProgram()
	{
		if(mpBoundProgram == this) mpBoundProgram = nullptr;
		msetPrograms.erase(this);

		GLint current = 0;
		glGetIntegerv(GL_CURRENT_PROGRAM, &current);
		for(auto& entry : mTextureVariants) {
			if(current == (GLint)entry.second.handle) glUseProgram(0);
			glDeleteProgram(entry.second.handle);
		}

		if(mlMaterialBuffer) glDeleteBuffers(1, &mlMaterialBuffer);
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PUBLIC METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	bool cGLSLProgram::InitPrograms(iLowLevelGraphics *apLowLevelGraphics)
	{
		mpLowLevelGraphics = apLowLevelGraphics;
		const char *reload = getenv("OVERTURE_SHADER_RELOAD");
		mbReloadEnabled = reload && reload[0] && tString(reload) != "0";
		return true;
	}

	void cGLSLProgram::ExitPrograms()
	{
		glUseProgram(0);
		mpBoundProgram = nullptr;

	}

	bool cGLSLProgram::Apply(unsigned int alArrayMask, const float* apLayers)
	{
		alArrayMask &= mlSampler2DMask;
		auto variant = mTextureVariants.find(alArrayMask);
		if(variant == mTextureVariants.end()) {
			std::vector<tString> files;
			auto linked = LinkTextureVariant(alArrayMask, files);
			WatchFiles(mvSources, files);
			if(!linked.handle) return false;
			variant = mTextureVariants.emplace(alArrayMask, linked).first;
		}
		if(!variant->second.handle) return false;
		glUseProgram(variant->second.handle);
		if(apLayers) for(int i=0; i<8; ++i)
			if(variant->second.layers[i] >= 0) glUniform1f(variant->second.layers[i], apLayers[i]);
		glBindBufferBase(GL_UNIFORM_BUFFER, 2, mlMaterialBuffer);
		if(ParametersChanged()) {
			glBindBuffer(GL_UNIFORM_BUFFER, mlMaterialBuffer);
			glBufferSubData(GL_UNIFORM_BUFFER, 0, static_cast<unsigned>(eProgramParam::Count) * 16, GetParameterData());
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
			ParametersUploaded();
		}
		return true;
	}

	bool cGLSLProgram::ReloadChangedFiles()
	{
		if(!mbReloadEnabled) return false;

		static std::chrono::steady_clock::time_point lastCheck;
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		if(now - lastCheck < std::chrono::milliseconds(500)) return false;
		lastCheck = now;

		bool bChanged = false;
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
		return CompileProgram();
	}

	void cGLSLProgram::Unload()
	{
	}

	void cGLSLProgram::Destroy()
	{
	}

	//-----------------------------------------------------------------------

	bool cGLSLProgram::CreateFromFile(const tString &asFile, const tString &asDefines)
	{
		const size_t separator = asFile.find(':');
		msVertex = asFile.substr(0, separator);
		msBase = separator == tString::npos ? msVertex : asFile.substr(separator + 1);
		msDefines = asDefines;

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
		// Reload every used sampler layout as one transaction. A failed
		// stage/include keeps the complete working set and material values.
		std::map<unsigned int, cTextureVariant> replacements;
		std::vector<tString> files;
		replacements[0] = LinkTextureVariant(0, files);
		for(const auto& entry : mTextureVariants)
			if(entry.first) replacements[entry.first] = LinkTextureVariant(entry.first, files);
		WatchFiles(mvSources, files);
		bool valid = true;
		for(const auto& entry : replacements) valid &= entry.second.handle != 0;
		if(!valid) {
			for(const auto& entry : replacements) if(entry.second.handle) glDeleteProgram(entry.second.handle);
			return false;
		}
		GLint current = 0;
		glGetIntegerv(GL_CURRENT_PROGRAM, &current);
		for(const auto& entry : mTextureVariants) {
			if(current == (GLint)entry.second.handle) glUseProgram(replacements[entry.first].handle);
			glDeleteProgram(entry.second.handle);
		}
		mTextureVariants.swap(replacements);
		mlProgram = mTextureVariants[0].handle;
		mlSampler2DMask = 0;
		GLint count = 0;
		glGetProgramiv(mlProgram, GL_ACTIVE_UNIFORMS, &count);
		for(GLint i=0; i<count; ++i) {
			char name[128]; GLsizei length; GLint size; GLenum type;
			glGetActiveUniform(mlProgram, i, sizeof(name), &length, &size, &type, name);
			if(type == GL_SAMPLER_2D && length == 4 && tString(name, 3) == "tex" && name[3] >= '0' && name[3] <= '7')
				mlSampler2DMask |= 1u << (name[3]-'0');
		}

		if(!mlMaterialBuffer){
			glGenBuffers(1, &mlMaterialBuffer);
			glBindBuffer(GL_UNIFORM_BUFFER, mlMaterialBuffer);
			glBufferData(GL_UNIFORM_BUFFER, sizeof(mvParameters), GetParameterData(), GL_DYNAMIC_DRAW);
			glBindBuffer(GL_UNIFORM_BUFFER, 0);
		}
		return true;
	}

	cGLSLProgram::cTextureVariant cGLSLProgram::LinkTextureVariant(unsigned int mask, std::vector<tString>& files)
	{
		tString defines = msDefines;
		for(int i=0; i<8; ++i) if(mask & (1u << i)) defines += " HPL_ARRAY_" + std::to_string(i);
		cTextureVariant variant;
		std::vector<tString> sources;
		variant.handle = CreateLinked(msBase, defines, mbState, msVertex, &sources);
		files.insert(files.end(), sources.begin(), sources.end());
		variant.layers.fill(-1);
		if(variant.handle) for(int i=0; i<8; ++i)
			variant.layers[i] = glGetUniformLocation(variant.handle, ("hpl_TextureLayer" + std::to_string(i)).c_str());
		return variant;
	}

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

	void cGLSLProgram::Bind(){ mpBoundProgram = this; }
	void cGLSLProgram::UnBind(){ if(mpBoundProgram == this) mpBoundProgram = nullptr; }

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

		if(mpLowLevelGraphics) mpLowLevelGraphics->SetTexture(alUnit, apTexture);

		return true;
	}

	//-----------------------------------------------------------------------

	//////////////////////////////////////////////////////////////////////////
	// PRIVATE METHODS
	//////////////////////////////////////////////////////////////////////////

	//-----------------------------------------------------------------------

	GLuint cGLSLProgram::CompileShader(GLenum aStage, const tString& asFile, const tString& asDefines,
										bool abState, std::vector<tString> *apFiles)
	{
		tString sSource = aStage == GL_VERTEX_SHADER ? kVertexPreamble : kFragmentPreamble;
		if(aStage == GL_FRAGMENT_SHADER && abState) sSource += kFragmentStatePreamble;

		tStringVec vDefines;
		cString::GetStringVec(asDefines, vDefines, nullptr);
		for(const tString& sDefine : vDefines) sSource += "#define " + sDefine + "\n";

		// Source number 0 is the preamble above.
		std::vector<tString> vFiles(1, "(preamble)");
		bool bRead = (aStage != GL_FRAGMENT_SHADER || ExpandIncludes(msShaderDir, "hpl1_texture.glsl", vFiles, sSource, 0)) &&
			ExpandIncludes(msShaderDir, "hpl1_material.glsl", vFiles, sSource, 0) &&
			ExpandIncludes(msShaderDir, asFile, vFiles, sSource, 0);
		if(apFiles) apFiles->insert(apFiles->end(), vFiles.begin() + 1, vFiles.end());
		if(!bRead) return 0;

		tString sName = asFile;
		if(asDefines != "") sName += " (" + asDefines + ")";

		return CompileSource(aStage, sSource, sName, vFiles);
	}

	//-----------------------------------------------------------------------

	GLuint cGLSLProgram::CompileSource(GLenum aStage, tString asSource, const tString& asName,
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

		cGpuProfiler::Label(GL_SHADER, lShader, sName);
		return lShader;
	}

	GLuint cGLSLProgram::CreateLinked(const tString& name, const tString& defines, bool state,
		const tString& vertex, std::vector<tString>* files)
	{
		std::vector<tString> sources;
		GLuint vs = CompileShader(GL_VERTEX_SHADER, "hpl1_" + (vertex.empty() ? name : vertex) + ".vertex", defines, state, &sources);
		GLuint fs = CompileShader(GL_FRAGMENT_SHADER, "hpl1_" + name + ".fragment", defines, state, &sources);
		if(files) *files = sources;
		if(!vs || !fs){
			if(vs) glDeleteShader(vs);
			if(fs) glDeleteShader(fs);
			return 0;
		}
		GLuint lProgram = glCreateProgram();
		glAttachShader(lProgram, vs);
		glAttachShader(lProgram, fs);
		glLinkProgram(lProgram);
		glDetachShader(lProgram, vs); glDeleteShader(vs);
		glDetachShader(lProgram, fs); glDeleteShader(fs);
		GLint lOk = GL_FALSE;
		glGetProgramiv(lProgram, GL_LINK_STATUS, &lOk);
		if(!lOk){
			char log[4096]; glGetProgramInfoLog(lProgram, sizeof(log), nullptr, log);
			Error("GLSL: Linking '%s' (%s) failed: %s\n", name.c_str(), defines.c_str(), log);
			glDeleteProgram(lProgram); return 0;
		}
		cGpuProfiler::Label(GL_PROGRAM, lProgram, name + " " + defines);
		GLuint material = glGetUniformBlockIndex(lProgram, "hpl_Material");
		if(material != GL_INVALID_INDEX){
			GLint size = 0;
			glGetActiveUniformBlockiv(lProgram, material, GL_UNIFORM_BLOCK_DATA_SIZE, &size);
			if(size != static_cast<GLint>(static_cast<unsigned>(eProgramParam::Count) * 16)){
				Error("GLSL: Material block layout mismatch for '%s': %d bytes\n", name.c_str(), size);
				glDeleteProgram(lProgram); return 0;
			}
			glUniformBlockBinding(lProgram, material, 2);
		}

		GLuint lFrameBlock = glGetUniformBlockIndex(lProgram, "hpl_Frame");
		if(lFrameBlock != GL_INVALID_INDEX) glUniformBlockBinding(lProgram, lFrameBlock, kFrameBlockBinding);
		GLuint lStateBlock = glGetUniformBlockIndex(lProgram, "hpl_State");
		if(lStateBlock != GL_INVALID_INDEX) glUniformBlockBinding(lProgram, lStateBlock, kStateBlockBinding);

		GLint previous; glGetIntegerv(GL_CURRENT_PROGRAM, &previous);
		glUseProgram(lProgram);
		// texN samplers always read texture unit N
		for(int i=0; i<kMaxSamplers; ++i)
		{
			char sName[8];
			snprintf(sName, sizeof(sName), "tex%d", i);
			GLint lLoc = glGetUniformLocation(lProgram, sName);
			if(lLoc >= 0) glUniform1i(lLoc, i);
		}
		// The shadow map of the light that is drawn, as a shadow sampler and
		// as its depth: on units of their own (hpl1_shadow.glsl)
		GLint lShadowLoc = glGetUniformLocation(lProgram, "hpl_ShadowMap");
		if(lShadowLoc >= 0) glUniform1i(lShadowLoc, kShadowMapUnit);
		lShadowLoc = glGetUniformLocation(lProgram, "hpl_ShadowMapDepth");
		if(lShadowLoc >= 0) glUniform1i(lShadowLoc, kShadowMapUnit + 1);
		GLint lDepthLoc = glGetUniformLocation(lProgram, "hpl_SceneDepth");
		if(lDepthLoc >= 0) glUniform1i(lDepthLoc, kSceneDepthUnit);
		GLint lColorLoc = glGetUniformLocation(lProgram, "hpl_SceneColor");
		if(lColorLoc >= 0) glUniform1i(lColorLoc, kSceneColorUnit);

		glUseProgram(previous);
		return lProgram;
	}

	//-----------------------------------------------------------------------
}
