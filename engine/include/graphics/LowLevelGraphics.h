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
#ifndef HPL_LOWLEVELGRAPHICS_H
#define HPL_LOWLEVELGRAPHICS_H

#include <functional>

#include "math/MathTypes.h"
#include "system/SystemTypes.h"
#include "graphics/GraphicsTypes.h"
#include "graphics/Bitmap2D.h"
#include "graphics/Texture.h"
#include "graphics/GPUProgram.h"
#include "graphics/VertexBuffer.h"

namespace hpl {

	enum eBlendFunc
	{
		eBlendFunc_Zero,
		eBlendFunc_One,
		eBlendFunc_SrcColor,
		eBlendFunc_OneMinusSrcColor,
		eBlendFunc_DestColor,
		eBlendFunc_OneMinusDestColor,
		eBlendFunc_SrcAlpha,
		eBlendFunc_OneMinusSrcAlpha,
		eBlendFunc_DestAlpha,
		eBlendFunc_OneMinusDestAlpha,
		eBlendFunc_SrcAlphaSaturate,
		eBlendFunc_LastEnum
	};

	enum eDepthTestFunc
	{
		eDepthTestFunc_Never,
		eDepthTestFunc_Less,
		eDepthTestFunc_LessOrEqual,
		eDepthTestFunc_Greater,
		eDepthTestFunc_GreaterOrEqual,
		eDepthTestFunc_Equal,
		eDepthTestFunc_NotEqual,
		eDepthTestFunc_Always,
		eDepthTestFunc_LastEnum
	};

	enum eCullMode
	{
		eCullMode_Clockwise,
		eCullMode_CounterClockwise,
		eCullMode_LastEnum
	};

	enum eGraphicCaps
	{
		eGraphicCaps_MaxTextureImageUnits,

		eGraphicCaps_AnisotropicFiltering,
		eGraphicCaps_MaxAnisotropicFiltering,
		/// The most samples SetMultisampling() takes
		eGraphicCaps_MaxMultisampling,

		eGraphicCaps_LastEnum
	};

	typedef tFlag tVtxBatchFlag;

	#define eVtxBatchFlag_Normal		(0x00000001)
	#define eVtxBatchFlag_Position		(0x00000002)
	#define eVtxBatchFlag_Color0		(0x00000004)
	#define eVtxBatchFlag_Texture0		(0x00000008)
	#define eVtxBatchFlag_Texture1		(0x00000010)
	#define eVtxBatchFlag_Texture2		(0x00000020)

	class iFontData;

	/**
	 * The values linear lighting is tuned with. The 3D scene is added up
	 * and blended on values proportional to light, and converted for the
	 * screen when it is finished. The defaults were chosen by playing; 2.2,
	 * 1, 1, a blend match of 0 and a shoulder of 1 leave everything as the
	 * conversion gives it.
	 */
	class cLinearLightingTuning
	{
	public:
		/**
		 * The power the amount of light (N.L and the falloff ramp, which
		 * were made to look right on screen values) is raised to. 2.2 gives
		 * one light the look it had on screen values (the original); 1.0
		 * takes them as amounts of light, which is wider and flatter.
		 */
		float mfLightExp = 1.8f;
		/// Scales the light of the finished scene before it is converted for the screen.
		float mfExposure = 1.3f;
		/**
		 * Scales the light of what is added to the scene without being lit:
		 * halos, illumination maps, additive materials and particles. The
		 * full scale is for what is white; darker parts get less of it,
		 * so faint edges stay faint.
		 */
		float mfGlow = 1.45f;
		/**
		 * How far the alpha of what is blended over the scene (particles,
		 * alpha materials, fog) is changed to cover as much as it did on
		 * screen values (the original): 0 leaves it, 1 matches it.
		 */
		float mfBlendMatch = 1.0f;
		/**
		 * Tonemapping: the screen value (0 to 1) above which bright values
		 * are rolled off towards white instead of clipping at it. Below it
		 * the picture is untouched. 1 is no tonemapping.
		 */
		float mfShoulder = 1.0f;
		/// Tonemapping: the amount of light, in times white, that becomes white on screen.
		float mfWhite = 4.0f;
	};

	/**
	 * How surfaces answer light (Milestone 5): highlights as wide and
	 * strong as each surface is glossy, and ambient light that shows its
	 * relief.
	 *
	 * The game has no gloss maps: how much a surface reflects and how wide
	 * its highlight is are both read from what it has for specular (the
	 * specular colour map, or the normal map's alpha), bright meaning glossy.
	 */
	class cShadingSettings
	{
	public:
		/// Scales the highlights of lights.
		float mfSpecular = 1.0f;
		/// The share of light a surface with a white specular map mirrors when faced straight on.
		float mfReflectance = 0.1f;
		/**
		 * Roughness (0 mirror, 1 chalk) where the specular map is black,
		 * and where it is white. A third of the specular materials have no
		 * map at all and count as white: with these defaults their
		 * highlight is about as bright as the original's, which was the
		 * same for every material and as wide as a roughness of 0.6.
		 */
		float mfRoughDull = 0.6f;
		float mfRoughShiny = 0.45f;
		/// Ambient light: how much more of it comes from above than from below (0 = the same).
		float mfAmbientUp = 0.3f;
		/// Ambient light: how much more of it surfaces that face the viewer get (0 = the same).
		float mfAmbientFacing = 0.4f;
		/// Ambient light mirrored at grazing angles by what has specular.
		float mfAmbientSheen = 0.25f;
	};

	/**
	 * What glows in the air (Milestone 5, step 4): the halos of lamps, the
	 * shafts of light and the particles, each drawn by a shader of its own
	 * (a picture on a flat sheet in the original).
	 */
	class cEffectSettings
	{
	public:
		/**
		 * Halos (billboards that face the viewer): the glow is made from
		 * the picture's brightness by distance from its middle, so it has
		 * no steps or blocks, and it is hidden by what stands in front
		 * of the lamp as read from the depth buffer, in the same frame.
		 */
		float mfHaloBrightness = 1.0f;
		/// How much of what the picture has besides the round glow (rays of a star) is kept, 0 to 1.
		float mfHaloStreaks = 1.0f;
		/**
		 * Shafts (billboards that turn round an axis): the picture is spun
		 * round the axis into a body of lit air that the view goes through.
		 * It ends softly at what it meets and can be walked into.
		 */
		float mfShaftBrightness = 1.0f;
		/// How uneven the air is: streaks that drift along the shaft, 0 to 1.
		float mfShaftDust = 0.35f;
		/**
		 * Particles: they fade out where they come near what is behind
		 * them and near the viewer, and those blended by alpha (smoke,
		 * dust, mist, snow, water) are lit by the lights around them.
		 * Those added to the scene (fire, sparks) glow as they did.
		 */
		/// The distance the fade takes, in times the particle's size. 0: no fade.
		float mfParticleSoft = 1.0f;
		/// How much the light counts: 0 is the brightness the original gives them everywhere.
		float mfParticleLight = 1.0f;
		/// Their brightness with no light on them, where the original's is 1.
		float mfParticleDark = 0.55f;
	};

	enum eFilmGrain
	{
		eFilmGrain_Off,
		/// Noise added to the picture.
		eFilmGrain_Noise,
		/// The picture as made of grains that are developed or not.
		eFilmGrain_Film,
		eFilmGrain_LastEnum
	};

	/**
	 * The grain laid over the finished 3D scene (hpl1_output.fragment),
	 * under the 2D drawing.
	 */
	class cFilmGrainSettings
	{
	public:
		eFilmGrain mType = eFilmGrain_Off;
		/// How strong the grain is, 0 to 1. At 1 film grain is what its
		/// model of twelve crystals to a grain gives.
		float mfIntensity = 0.25f;
		/// The size of a grain in pixels of a picture 1080 high; it grows
		/// with the picture, and is never smaller than a pixel.
		float mfSize = 1.5f;
		/// 0: the grain is grey; 1: red, green and blue each have their own.
		float mfColor = 0.3f;
		/// How much of the grain the dark and the bright parts get, 0 to 1.
		float mfShadows = 1.0f;
		float mfHighlights = 1.0f;
		/// How often the grain changes, in times a second; 0 is every frame.
		float mfRate = 24.0f;
		/// Film only: how much the grains differ in size, 0 to 1.
		float mfDispersion = 0.5f;
	};

	/// Player feedback drawn over the scene, below the HUD. All amounts are 0..1.
	class cPlayerEffectState
	{
	public:
		float mfAdaptation = 0;
		float mfLowHealth = 0;
		/// Continuous low-health severity; independent of the heartbeat veil.
		float mfLowHealthVision = 0;
		float mfBloodHit = 0;
		/// A new pattern for each impact, held still while it fades.
		float mfBloodSeed = 17;
		float mfIceHit = 0;
		float mfPanic = 0;
		float mfPanicPulse = 0;
		float mfFlash = 0;
		float mfDeath = 0;
		float mfDeathBlack = 0;
		float mfSave = 0;
		float mfUnderwater = 0;
		cColor mWaterColor = cColor(0.5f, 0.65f, 1.0f);
	};

	class cPlayerEffectSettings
	{
	public:
		float mfAdaptationStrength = 0.65f;
		/// Seconds to reveal shadow detail; leaving darkness is faster.
		float mfAdaptationTime = 2.0f;
		float mfInjuryStrength = 0.8f;
		float mfPanicStrength = 0.65f;
		float mfWhiteoutGlow = 0.8f;
	};

	/// What SMAA takes for an edge between two pixels.
	enum eSmaaEdges
	{
		/// A difference in brightness.
		eSmaaEdges_Luma,
		/// The largest difference of red, green and blue.
		eSmaaEdges_Color,
		/// The differences of red, green and blue together, each counted
		/// by how bright it looks.
		eSmaaEdges_ColorWeighted,
		/// A difference in depth alone: the outlines of things, nothing on
		/// their surfaces.
		eSmaaEdges_Depth,
		eSmaaEdges_LastEnum
	};

	/**
	 * Anti-aliasing of the finished 3D scene by its edges (SMAA 1x,
	 * hpl1_SMAA.fragment): where neighbouring pixels differ, the shape of
	 * the edge is followed along and the pixels on it are blended by how
	 * much of each the true edge would cover. Works on what multisampling
	 * does not reach (alpha-tested textures, the edges of shadows) and
	 * along with it.
	 */
	class cSmaaSettings
	{
	public:
		bool mbActive = false;
		eSmaaEdges mEdges = eSmaaEdges_Color;
		/// How much two pixels differ where there is an edge, 0.05 to 0.2
		/// of the range from black to white. Lower finds more, and blurs
		/// more of what is detail and not an edge.
		float mfThreshold = 0.1f;
		/// How far an edge is followed along, in steps of two pixels, 1 to
		/// 112, and a diagonal one, in pixels, 0 (diagonals are not looked
		/// for) to 20.
		int mlSearchSteps = 16;
		int mlDiagonalSteps = 8;
		/// How much corners are rounded off, in percent; 100 is as much as
		/// any other part of an edge.
		int mlCornerRounding = 25;
		/**
		 * The depth helps: where it has an edge of its own (the distance
		 * differs by more than mfDepthThreshold of itself) the threshold is
		 * the one above times mfDepthScale times (1 - mfDepthStrength), and
		 * elsewhere times mfDepthScale. So outlines are found though the
		 * colours on both sides are close, as they are in the dark.
		 */
		bool mbDepthHelp = true;
		float mfDepthThreshold = 0.03f;
		float mfDepthScale = 1.0f;
		float mfDepthStrength = 0.5f;
		/// Shown in place of the picture: 1 the edges found, 2 the blend weights.
		int mlShow = 0;

		/// The settings of the reference implementation: low, medium, high, ultra.
		static const int klQualityNum = 4;
		/// The quality these settings are, klQualityNum if none.
		int GetQuality() const
		{
			for(int i=0; i<klQualityNum; ++i)
			{
				const cSmaaSettings quality = Quality(i);
				if(	quality.mfThreshold == mfThreshold && quality.mlSearchSteps == mlSearchSteps &&
					quality.mlDiagonalSteps == mlDiagonalSteps && quality.mlCornerRounding == mlCornerRounding)
					return i;
			}
			return klQualityNum;
		}
		void SetQuality(int alQuality)
		{
			const cSmaaSettings quality = Quality(alQuality);
			mfThreshold = quality.mfThreshold;
			mlSearchSteps = quality.mlSearchSteps;
			mlDiagonalSteps = quality.mlDiagonalSteps;
			mlCornerRounding = quality.mlCornerRounding;
		}

	private:
		static cSmaaSettings Quality(int alQuality)
		{
			static const float vThreshold[klQualityNum] = {0.15f, 0.1f, 0.1f, 0.05f};
			static const int vSteps[klQualityNum][3] = {{4,0,100}, {8,0,100}, {16,8,25}, {32,16,25}};
			cSmaaSettings quality;
			quality.mfThreshold = vThreshold[alQuality];
			quality.mlSearchSteps = vSteps[alQuality][0];
			quality.mlDiagonalSteps = vSteps[alQuality][1];
			quality.mlCornerRounding = vSteps[alQuality][2];
			return quality;
		}
	};

	/**
	 * What the light shaders need to read the shadow map of the light that
	 * is drawn. Lengths are in world units (metres); a blur is given as its
	 * radius at a distance of 1 from the light, and grows with the distance.
	 */
	class cShadowMapParams
	{
	public:
		/**
		 * 0: the light has no shadow map. 1: hard, one filtered sample.
		 * 2: blurred by mfSoftness. 3: blurred by how far what casts the
		 * shadow is from what it falls on, for a light of mfLightSize.
		 */
		int mlFilter = 0;
		/// The depth range the map was drawn with.
		float mfNear = 0.05f;
		float mfFar = 1;
		/// The width of one texel at a distance of 1 from the light.
		float mfTexelSize = 0;
		/// Spotlights: texture coordinates per unit of x/z and y/z in the light's view.
		cVector2f mvUvScale = cVector2f(1,1);

		float mfSoftness = 0.02f;
		/// Radius of the light source.
		float mfLightSize = 0.05f;
		/// The widest blur.
		float mfMaxBlur = 0.03f;
		/// Samples of the blur, and of the search for what casts the shadow (filter 3).
		int mlSamples = 16;
		int mlBlockerSamples = 12;
		/// The samples are turned by a different angle in every pixel.
		bool mbNoise = true;

		/// What a surface is moved towards the light by before it is compared.
		float mfBias = 0.01f;
		/// The same, in texels of the map, for a surface at 45 degrees to the light.
		float mfSlopeBias = 1.5f;

		/// The light passes draw how much of the light arrives instead of the lit surface.
		bool mbShowOnly = false;
	};

	class iLowLevelGraphics
	{
	public:
		virtual ~iLowLevelGraphics(){}
		/**
		 * Sets the video mode. Must only be called ONCE!
		 * \param alWidth
		 * \param alHeight
		 * \param alBpp
		 * \param abFullscreen
		 * \param alMultiSampling The amount of multisamplimg, 0 = off; see SetMultisampling().
		 * \param alDisplay The display to open on, see GetDisplayNum(). -1 = pick one.
		 * \return
		 */
		virtual bool Init(int alWidth, int alHeight, int alBpp, int abFullscreen, int alMultisampling,
							const tString& asWindowCaption, int alDisplay)=0;

		/**
		 * The displays (monitors) that are connected. Can be used before Init().
		 */
		virtual int GetDisplayNum()=0;
		virtual tString GetDisplayName(int alDisplay)=0;
		/**
		 * The size of the desktop on a display.
		 */
		virtual cVector2l GetDisplaySize(int alDisplay)=0;
		/**
		 * The fullscreen sizes of a display, smallest first.
		 */
		virtual void GetDisplayModes(int alDisplay, std::vector<cVector2l> &avModes)=0;
		/**
		 * The display the window was opened on.
		 */
		virtual int GetDisplay()=0;


		/**
		 * Get the capabilities of the graphics. Th return value depends on the capability
		 * \param aType
		 * \return
		 */
		virtual int GetCaps(eGraphicCaps aType)=0;
		/**
		 * Show the cursor or not. Default is false
		 * \param aX
		 */
		virtual void ShowCursor(bool abX)=0;

		virtual int GetMultisampling()=0;
		/**
		 * The number of samples a pixel of the 3D scene has, 0 or 1 for
		 * one. More than the graphics card takes (eGraphicCaps_MaxMultisampling)
		 * is that many. Not while the scene is drawn.
		 */
		virtual void SetMultisampling(int alSamples)=0;
		/**
		 * How every texture with mipmaps is filtered: between the two
		 * nearest mipmaps (trilinear) or from the nearest one alone, and
		 * with how many samples along a surface seen at a slant (1 is
		 * none, up to eGraphicCaps_MaxAnisotropicFiltering).
		 */
		virtual void SetTextureFilter(eTextureFilter aFilter, float afAnisotropy)=0;
		/**
		 * Get Size of screen
		 * \return
		 */
		/**
		 * Size of the window in pixels, which the 2D drawing covers.
		 */
		virtual cVector2f GetScreenSize()=0;
		/**
		 * Size in pixels the 3D scene is drawn at, between BeginScene() and
		 * EndScene(). Post effects and screen copies work at this size.
		 */
		virtual cVector2f GetRenderSize()=0;
		/**
		 * Sets the render size to the window size times afScale (0.25 to
		 * 2). Not while the scene is drawn.
		 */
		virtual void SetRenderScale(float afScale)=0;
		virtual float GetRenderScale()=0;
		/**
		 * How the lighting looks; see cLinearLightingTuning.
		 */
		virtual void SetLinearLightingTuning(const cLinearLightingTuning& aTuning)=0;
		virtual const cLinearLightingTuning& GetLinearLightingTuning()=0;
		/**
		 * How surfaces are shaded; see cShadingSettings. Takes effect with
		 * the next frame.
		 */
		virtual void SetShading(const cShadingSettings& aSettings)=0;
		virtual const cShadingSettings& GetShading()=0;
		/**
		 * How halos, shafts of light and particles are drawn; see
		 * cEffectSettings. Takes effect with the next frame.
		 */
		virtual void SetEffects(const cEffectSettings& aSettings)=0;
		virtual const cEffectSettings& GetEffects()=0;
		/**
		 * The grain over the 3D scene; see cFilmGrainSettings.
		 */
		virtual void SetFilmGrain(const cFilmGrainSettings& aSettings)=0;
		virtual const cFilmGrainSettings& GetFilmGrain()=0;
		virtual void SetPlayerEffectSettings(const cPlayerEffectSettings &aSettings) = 0;
		virtual const cPlayerEffectSettings &GetPlayerEffectSettings() = 0;
		/// Cleared before OnDraw each frame, then filled by the game's effect owners.
		virtual void SetPlayerEffectState(const cPlayerEffectState &aState) = 0;
		virtual const cPlayerEffectState &GetPlayerEffectState() = 0;
		/**
		 * Anti-aliasing of the finished 3D scene; see cSmaaSettings.
		 */
		virtual void SetSmaa(const cSmaaSettings& aSettings)=0;
		virtual const cSmaaSettings& GetSmaa()=0;
		/**
		 * Starts drawing the 3D scene.
		 */
		virtual void BeginScene()=0;
		/**
		 * With multisampling: from here to EndScene() the scene is drawn
		 * into with one sample a pixel, each the average of those it had.
		 * For what is drawn over the whole finished picture, which has
		 * nothing to gain from more. Screen copies and hpl_SceneDepth
		 * are of that picture then. EndScene() does it if nobody has.
		 */
		virtual void ResolveScene()=0;
		/**
		 * With multisampling, shaders read the scene's depth
		 * (hpl_SceneDepth) from a copy with one sample a pixel: this brings
		 * it up to date with what has been drawn. Not needed after
		 * ResolveScene().
		 */
		virtual void ResolveSceneDepth()=0;
		/**
		 * To be called before each draw over the 3D scene by a fragment
		 * program that reads the scene's colour through hpl_SceneColor (a
		 * sampler2DMS when the scene is multisampled, a sampler2D when
		 * not) at the pixel it is drawn at, and nowhere else.
		 * \return false if the scene can't be read while it is drawn
		 * into; the program must do with a copy then.
		 */
		virtual bool BeginSceneColorRead()=0;
		/**
		 * Ends drawing the 3D scene: it is scaled to the window, and the
		 * drawing that follows (HUD, menus) goes on top of it.
		 */
		virtual void EndScene()=0;
		virtual cVector2f GetVirtualSize()=0;
		/**
		 * Sets the virtual screen size. Default is 0-1
		 * \param avSize
		 */
		virtual void SetVirtualSize(cVector2f avSize)=0;
		/**
		 * How far the screen reaches past the virtual size on each side, in
		 * virtual units. The virtual canvas keeps its aspect ratio and is
		 * centred, so this is non-zero on one axis when the window has
		 * another aspect ratio: the screen spans -offset to size+offset.
		 */
		virtual cVector2f GetVirtualOffset()=0;

		/**
		 * Set if vsync should eb active
		 * \param abX
		 */
		virtual void SetVsyncActive(bool abX)=0;

		virtual void SetGammaCorrection(float afX)=0;
		virtual float GetGammaCorrection()=0;

		virtual iBitmap2D* CreateBitmap2D(const cVector2l &avSize, unsigned int alBpp)=0;
		virtual iFontData* CreateFontData(const tString &asName)=0;

		virtual iTexture* CreateTexture(bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget)=0;
		virtual iTexture* CreateTexture(const tString &asName,bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget)=0;
		virtual iTexture* CreateTexture(iBitmap2D* apBmp,bool abUseMipMaps, eTextureType aType,
										eTextureTarget aTarget)=0;
		virtual iTexture* CreateTexture(const cVector2l& avSize,int alBpp,cColor aFillCol,
									bool abUseMipMaps, eTextureType aType, eTextureTarget aTarget)=0;
		/**
		 * A texture of 16-bit floating point values to render into, with 1,
		 * 2 or 4 channels, filled with zeros.
		 */
		virtual iTexture* CreateFloatTexture(const cVector2l& avSize, int alChannels, eTextureTarget aTarget)=0;



		virtual iGpuProgram* CreateGpuProgram(const tString& asName, eGpuProgramType aType)=0;

		//TODO: Kinda quick and diry, better to have a screen to Bitmap.
		// and then a save as in the Bitmap.
		virtual void SaveScreenToBMP(const tString& asFile)=0;
		/**
		 * Saves the next frame as a PNG, as it is when it is shown: after
		 * everything has been drawn and just before the buffers are swapped.
		 */
		virtual void SaveNextFrameToPNG(const tString& asFile)=0;

		/////////// MATRIX METHODS //////////////////////////

		virtual void PushMatrix(eMatrix aMtxType)=0;
		virtual void PopMatrix(eMatrix aMtxType)=0;
		virtual void SetIdentityMatrix(eMatrix aMtxType)=0;

		virtual void SetMatrix(eMatrix aMtxType, const cMatrixf& a_mtxA)=0;


		/// The current matrix, kept on the CPU (no GL read back).
		virtual const cMatrixf& GetMatrix(eMatrix aMtxType)=0;

		/**
		 * Per-frame values for shaders, the hpl_Frame block in
		 * hpl1_frame.glsl; the render size is added. Set when the 3D scene
		 * starts. afTime is in seconds.
		 */
		virtual void SetFrameUniforms(const cMatrixf& a_mtxView, const cMatrixf& a_mtxProjection,
									const cVector3f& avEyePos, float afNear, float afFar,
									float afTime)=0;

		/////////// DEBUGGING //////////////////////////

		/**
		 * Starts a named section of the frame: a GL debug group (RenderDoc
		 * shows them), timed on the GPU while the debug overlay is shown.
		 * Sections nest; EndGpuSection() ends the last one begun. The name
		 * must stay valid until the frame is shown (use a string literal).
		 */
		virtual void BeginGpuSection(const char *asName)=0;
		virtual void EndGpuSection()=0;

		/// The overlay with frame times, GPU sections and draw counts.
		virtual void SetDebugOverlayActive(bool abX)=0;
		virtual bool GetDebugOverlayActive()=0;

		/**
		 * A function that draws Dear ImGui windows of the game's own, called
		 * every frame whether the overlay is shown or not; an empty function
		 * removes it.
		 */
		virtual void SetDebugPanel(std::function<void()> aDraw)=0;
		/// Gives ImGui the mouse and the keyboard, and frees the pointer.
		virtual void SetDebugPanelInput(bool abX)=0;

		virtual void SetOrthoProjection(const cVector2f& avSize, float afMin, float afMax)=0;
		/**
		 * Ortho projection of the whole screen in virtual coordinates,
		 * -GetVirtualOffset() to GetVirtualSize()+GetVirtualOffset().
		 */
		virtual void SetVirtualOrthoProjection(float afMin, float afMax)=0;

		/////////// DRAWING METHODS /////////////////////////

		// CLEARING THE FRAMEBUFFER
		virtual void ClearScreen()=0;

		virtual void SetClearColor(const cColor& aCol)=0;
		virtual void SetClearDepth(float afDepth)=0;

		virtual void SetClearColorActive(bool abX)=0;
		virtual void SetClearDepthActive(bool abX)=0;

		virtual void SetColorWriteActive(bool abR,bool abG,bool abB,bool abA)=0;
		virtual void SetDepthWriteActive(bool abX)=0;

		virtual void SetCullActive(bool abX)=0;
		virtual void SetCullMode(eCullMode aMode)=0;

		//DEPTH
		virtual void SetDepthTestActive(bool abX)=0;
		virtual void SetDepthTestFunc(eDepthTestFunc aFunc)=0;

		//ALPHA
		/**
		 * Fragments with an alpha under afMin are not drawn: what materials
		 * of eMaterialAlphaMode_Trans cut out. 0 draws them all.
		 */
		virtual void SetAlphaCutoff(float afMin)=0;

		//SCISSOR
		virtual void SetScissorActive(bool abX)=0;
		virtual void SetScissorRect(const cRect2l &aRect)=0;

		//TEXTURE
		virtual void SetTexture(unsigned int alUnit,iTexture* apTex)=0;

		//BLENDING
		virtual void SetBlendActive(bool abX)=0;
		virtual void SetBlendFunc(eBlendFunc aSrcFactor, eBlendFunc aDestFactor)=0;

		// POLYGONS
		virtual iVertexBuffer* CreateVertexBuffer(tVertexFlag aFlags, eVertexBufferDrawType aDrawType,
								eVertexBufferUsageType aUsageType,
								int alReserveVtxSize=0,int alReserveIdxSize=0)=0;



		virtual void DrawQuad(const tVertexVec &avVtx)=0;
		virtual void DrawQuad(const tVertexVec &avVtx, const cColor aCol)=0;
		virtual void DrawQuad(const tVertexVec &avVtx,const float afZ)=0;
		virtual void DrawQuad(const tVertexVec &avVtx,const float afZ,const cColor &aCol)=0;

		//VERTEX BATCHER
		virtual void AddVertexToBatch(const cVertex *apVtx, const cVector3f* avTransform)=0;

		virtual void AddVertexToBatch_Size2D(const cVertex *apVtx, const cVector3f* avTransform,
									const cColor* apCol,const float& mfW, const float& mfH)=0;

		virtual void AddVertexToBatch_Raw(	const cVector3f& avPos, const cColor &aColor,
											const cVector3f& avTex)=0;


		virtual void AddIndexToBatch(int alIndex)=0;


		//Add more ways to add Vertex to the batch?
		//Index array, vtxArray, etc perhaps?

		virtual void FlushQuadBatch(tVtxBatchFlag aTypeFlags, bool abAutoClear=true)=0;
		virtual void ClearBatch()=0;

		//some primitive:
		virtual void DrawLine(const cVector3f& avBegin, const cVector3f& avEnd, cColor aCol)=0;
		virtual void DrawBoxMaxMin(const cVector3f& avMax, const cVector3f& avMin, cColor aCol)=0;
		virtual void DrawSphere(const cVector3f& avPos, float afRadius, cColor aCol)=0;


		virtual void DrawLine2D(const cVector2f& avBegin, const cVector2f& avEnd, float afZ, cColor aCol)=0;

		// GENERAL
		/**
		 * All further drawing operations are rendered to this texture, which
		 * fills the viewport. There is no depth or stencil buffer.
		 * \param pTex Texture to render to. NULL = the screen (the scene
		 * framebuffer)
		 */
		virtual void SetRenderTarget(iTexture* pTex)=0;

		/**
		 * As SetRenderTarget(), to several textures of one size at once, up
		 * to eight: the fragment program writes to the first through
		 * outColor and to the others through outputs it declares itself,
		 * layout(location = 1) and up.
		 */
		virtual void SetRenderTargets(iTexture** apTextures, int alNum)=0;

		/**
		 * While set, the scene is drawn into without its depth and stencil
		 * buffer, so that a pass can read the depth (hpl_SceneDepth) of
		 * the scene it draws over. For the scene itself, not a render
		 * target; off again when the next scene begins.
		 */
		virtual void SetSceneColorOnly(bool abX)=0;

		/**
		 * All further drawing goes to the depth of a shadow map, cleared:
		 * the one of spotlights, or a face (0 to 5: +x -x +y -y +z -z) of
		 * the cube map of point lights. There is one of each, alSize pixels
		 * a side, and the light shaders read them. Nearer than the near
		 * plane counts as on it.
		 */
		virtual void BeginShadowMap(bool abCube, int alFace, int alSize)=0;
		/**
		 * Back to the target before BeginShadowMap().
		 */
		virtual void EndShadowMap()=0;
		/**
		 * For the light passes drawn from now on.
		 */
		virtual void SetShadowMapParams(const cShadowMapParams& aParams)=0;
		/**
		 * The look of the light whose passes are drawn next; the light
		 * shaders read it (hpl_Light0..2, hpl1_light_look.glsl).
		 */
		virtual void SetLightLook(const cLightLook& aLook)=0;

		/**
		 * Draws a part of the 3D scene as it is drawn so far into the same
		 * part of apTarget, a 2D texture of the render size: for what reads
		 * the scene while it draws over it. The target's format decides
		 * the precision: 8 bits clamp to 0..1 and round, as the copies of
		 * the screen in the original did.
		 * \param avPos The part's top left corner, in pixels from the top.
		 * \return false if nothing was drawn (no scene, or a render target is set)
		 */
		virtual bool DrawSceneCopy(iTexture* apTarget, const cVector2l &avPos, const cVector2l &avSize)=0;
		virtual void FlushRendering()=0;
		virtual void SwapBuffers()=0;
	};

	/// Begins a GPU section and ends it at the end of the scope.
	class cGpuSection
	{
	public:
		cGpuSection(iLowLevelGraphics *apLowLevel, const char *asName) : mpLowLevel(apLowLevel)
		{
			mpLowLevel->BeginGpuSection(asName);
		}
		~cGpuSection(){ mpLowLevel->EndGpuSection(); }

	private:
		iLowLevelGraphics *mpLowLevel;
	};
};
#endif // HPL_LOWLEVELGRAPHICS_H
