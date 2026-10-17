/*
 * This file is part of Overture Remade, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#include "core/StdAfx.h"
#include "core/RefView.h"
#include "core/Init.h"
#include "core/MapHandler.h"
#include "core/GraphicsHelper.h"
#include "entities/GameSaveArea.h"
#include "player/Player.h"
#include "player/PlayerHelper.h"
#include "systems/EffectHandler.h"
#include "systems/FadeHandler.h"

#include <imgui.h>
#include <cmath>

static const char *gvEffectNames[] = {
	"None", "Night adaptation", "Low health", "Double vision only", "Blood hit", "Ice hit",
	"Panic", "White flash", "Save-point haze", "Death", "Underwater", "Combined"};
static const char *gvEffectFiles[] = {
	"neutral", "adaptation", "low-health", "double-vision", "blood", "ice", "panic",
	"flash", "save", "death", "underwater", "combined"};
enum eEffectAction
{
	eAction_Blood = 1,
	eAction_Ice,
	eAction_Flash,
	eAction_Shake,
	eAction_EarRing,
	eAction_Fear,
	eAction_LowHealth,
	eAction_Heal,
	eAction_BlackFade,
	eAction_Rock
};

static unsigned int NextBloodPreviewSeed(unsigned int alSeed)
{
	return (alSeed * 25173u + 13849u) & 65535u;
}

void cRefView::DrawEffectsLab()
{
	const float fEm = ImGui::GetFontSize();
	const ImGuiViewport *pViewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(ImVec2(pViewport->WorkPos.x + fEm, pViewport->WorkPos.y + fEm), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSizeConstraints(ImVec2(fEm * 27, 0), ImVec2(fEm * 27, pViewport->WorkSize.y - fEm * 2));
	ImGui::SetNextWindowBgAlpha(0.9f);
	bool bOpen = true;
	if(ImGui::Begin("Effects lab (F2)", &bOpen, ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Checkbox("Full debug panels", &mbEffectsLabFullPanel);
		bool bFlashlight = mpInit->mpPlayer->GetFlashLight()->IsActive();
		if(ImGui::Checkbox("Flashlight", &bFlashlight))
		{
			mbFlashLight = bFlashlight;
			mbSetFlashLight = true;
			mbUsed = true;
		}
		DrawPlayerEffects();
	}
	ImGui::End();
	if(!bOpen) SetOpen(false);
}

void cRefView::DrawPlayerEffects()
{
	iLowLevelGraphics *pLowLevel = mpInit->mpGame->GetGraphics()->GetLowLevel();
	cPlayerEffectSettings settings = pLowLevel->GetPlayerEffectSettings();
	bool bChanged = false;
	int lMode = settings.mbImproved ? 1 : 0;
	if(ImGui::Combo("Treatment", &lMode, "Original\0Improved\0"))
	{
		settings.mbImproved = lMode == 1;
		bChanged = true;
	}
	ImGui::SameLine();
	if(ImGui::Button("Reset##player-effects"))
	{
		settings = cPlayerEffectSettings();
		bChanged = true;
	}
	ImGui::PushItemWidth(ImGui::GetFontSize() * 10);
	ImGui::BeginDisabled(!settings.mbImproved);
	bChanged |= ImGui::SliderFloat("Adaptation", &settings.mfAdaptationStrength, 0, 1, "%.2f");
	bChanged |= ImGui::SliderFloat("Adaptation time", &settings.mfAdaptationTime, 0.3f, 5.0f, "%.1f s");
	bChanged |= ImGui::SliderFloat("Injury", &settings.mfInjuryStrength, 0, 1, "%.2f");
	bChanged |= ImGui::SliderFloat("Panic", &settings.mfPanicStrength, 0, 1, "%.2f");
	bChanged |= ImGui::SliderFloat("Whiteout glow", &settings.mfWhiteoutGlow, 0, 1, "%.2f");
	ImGui::EndDisabled();
	if(bChanged) pLowLevel->SetPlayerEffectSettings(settings);

	ImGui::Separator();
	bool bInGame = mpInit->mpGame->GetUpdater()->GetCurrentContainerName() == "Default" &&
		mpInit->mpGame->GetScene()->GetWorld3D() && !mpInit->mpMapHandler->IsChangingMap();
	ImGui::BeginDisabled(!bInGame);
	if(ImGui::BeginCombo("Preview", gvEffectNames[mlEffectPreview]))
	{
		for(int i = 0; i < ePreview_Last; ++i)
			if(ImGui::Selectable(gvEffectNames[i], mlEffectPreview == i))
			{
				mlEffectPreview = i;
				if(i == ePreview_Blood) mlEffectPreviewBloodSeed = NextBloodPreviewSeed(mlEffectPreviewBloodSeed);
				mfEffectPreviewClock = 0;
				mbEffectTour = false;
			}
		ImGui::EndCombo();
	}
	ImGui::SliderFloat("Amount", &mfEffectPreviewAmount, 0, 1, "%.2f");
	if(mlEffectPreview == ePreview_Blood && ImGui::Button("New splash"))
		mlEffectPreviewBloodSeed = NextBloodPreviewSeed(mlEffectPreviewBloodSeed);
	ImGui::Checkbox("Animate", &mbEffectPreviewAnimate);
	ImGui::SameLine();
	if(ImGui::Checkbox("Tour all", &mbEffectTour))
	{
		mfEffectPreviewClock = 0;
		if(mbEffectTour)
		{
			mlEffectPreview = ePreview_Adaptation;
			mbEffectPreviewAnimate = true;
		}
	}
	if(ImGui::Button("Stop preview"))
	{
		mlEffectPreview = ePreview_None;
		mbEffectTour = false;
	}
	ImGui::TextWrapped("Previews change the picture, without death, saves or enemy alerts. "
					   "F2 closes the panel; the preview continues while you walk.");

	ImGui::Separator();
	ImGui::TextUnformatted("Live feedback");
	if(ImGui::Button("Blood hit")) mlEffectAction = eAction_Blood;
	ImGui::SameLine();
	if(ImGui::Button("Ice hit")) mlEffectAction = eAction_Ice;
	if(ImGui::Button("Flash")) mlEffectAction = eAction_Flash;
	ImGui::SameLine();
	if(ImGui::Button("Shake")) mlEffectAction = eAction_Shake;
	ImGui::SameLine();
	if(ImGui::Button("Ear ringing")) mlEffectAction = eAction_EarRing;
	if(ImGui::Button("Toggle scripted fear")) mlEffectAction = eAction_Fear;
	if(ImGui::Button("Set health to 25")) mlEffectAction = eAction_LowHealth;
	ImGui::SameLine();
	if(ImGui::Button("Restore health")) mlEffectAction = eAction_Heal;
	if(ImGui::Button("Black fade")) mlEffectAction = eAction_BlackFade;
	cDepthOfFieldSettings dof = mpInit->mpGame->GetGraphics()->GetRendererPostEffects()->GetDepthOfFieldSettings();
	bool bDofChanged = ImGui::Checkbox("Preview focus blur", &dof.mbTest);
	if(dof.mbTest)
	{
		bDofChanged |= ImGui::SliderFloat("Focus at", &dof.mfTestFocus, 0.2f, 15, "%.1f m");
		bDofChanged |= ImGui::SliderFloat("Focus range", &dof.mfTestRange, 0.1f, 5, "%.1f m");
	}
	if(bDofChanged)
		mpInit->mpGame->GetGraphics()->GetRendererPostEffects()->SetDepthOfFieldSettings(dof);
	cPlayerNoiseFilter *pNoise = mpInit->mpPlayer->GetNoiseFilter();
	int lGrain = pNoise->IsActive() ? 1 + (int)pNoise->GetType() : 0;
	if(ImGui::Combo("Grain", &lGrain, "Off\0Original tiles\0Shader noise\0Film\0"))
	{
		if(lGrain > 0) pNoise->SetType((eNoiseFilterType)(lGrain - 1));
		pNoise->SetActive(lGrain > 0);
	}
	ImGui::TextWrapped("Hits use the real impact animation without taking health. "
					   "Health buttons affect movement and heartbeat.");
	bool bLab = mpInit->mpMapHandler->GetCurrentMapName() == "effectslab";
	ImGui::BeginDisabled(!bLab);
	if(ImGui::Button("Toggle boat rocking (lab)")) mlEffectAction = eAction_Rock;
	ImGui::EndDisabled();
	ImGui::EndDisabled();
	ImGui::PopItemWidth();

	ImGui::Separator();
	if(ImGui::Button("Load effects lab")) msLoadMap = "effectslab.dae";
	ImGui::TextWrapped("Dark alcove on the left: crouch and stay still. Lamps and props ahead; "
					   "water on the right. Use the preview for panic and death.");
}

cPlayerEffectState cRefView::EffectPreviewState() const
{
	cPlayerEffectState state;
	state.mfBloodSeed = (float)mlEffectPreviewBloodSeed;
	state.mfPanicPulse = 0.5f - 0.5f * cosf(mfEffectPreviewClock * 4.6f);
	float fAmount = cMath::Clamp(mfEffectPreviewAmount, 0.0f, 1.0f);
	if(mbEffectPreviewAnimate)
		fAmount *= 0.5f - 0.5f * cosf(mfEffectPreviewClock * k2Pif / 3.0f);
	switch(mlEffectPreview)
	{
		case ePreview_Adaptation:
			state.mfAdaptation = fAmount;
			break;
		case ePreview_Health:
			state.mfLowHealth = fAmount;
			state.mfLowHealthVision = fAmount;
			break;
		case ePreview_DoubleVision:
			state.mfLowHealthVision = fAmount;
			break;
		case ePreview_Blood:
			state.mfBloodHit = fAmount;
			break;
		case ePreview_Ice:
			state.mfIceHit = fAmount;
			break;
		case ePreview_Panic:
			state.mfPanic = fAmount;
			break;
		case ePreview_Flash:
			state.mfFlash = fAmount;
			break;
		case ePreview_Save:
			state.mfSave = fAmount;
			break;
		case ePreview_Death:
			state.mfDeath = fAmount;
			state.mfDeathBlack = cMath::Clamp((fAmount - 0.55f) / 0.45f, 0.0f, 1.0f);
			break;
		case ePreview_Underwater:
			state.mfUnderwater = fAmount;
			break;
		case ePreview_Combined:
			state.mfAdaptation = fAmount;
			state.mfLowHealth = fAmount;
			state.mfLowHealthVision = fAmount;
			state.mfPanic = fAmount;
			break;
		default:
			break;
	}
	return state;
}

void cRefView::OnDraw()
{
	HoldCompareCamera();
	if(mlEffectPreview == ePreview_None ||
		mpInit->mpGame->GetUpdater()->GetCurrentContainerName() != "Default" ||
		mpInit->mpGame->GetGraphics()->GetDrawer()->GetSkipDrawing()) return;
	iLowLevelGraphics *pLowLevel = mpInit->mpGame->GetGraphics()->GetLowLevel();
	cPlayerEffectState state = EffectPreviewState();
	pLowLevel->SetPlayerEffectState(state);
	if(!pLowLevel->GetPlayerEffectSettings().mbImproved)
		DrawLegacyEffectPreview(state);
}

void cRefView::DrawLegacyEffectPreview(const cPlayerEffectState &state)
{
	static const char *vFiles[] = {"player_in_shadow.jpg", "player_hurt.bmp", "player_hit_blood_splash.bmp",
		"player_hit_ice.bmp", "player_death_fade.bmp", "player_death_black.bmp",
		"effect_white.jpg", "effect_white.jpg", "effect_white.jpg"};
	static const char *vMaterials[] = {"diffadditive2d", "diffalpha2d", "diffalpha2d", "diffalpha2d",
		"smoke2d", "smoke2d", "diffalpha2d", "diffadditive2d", "smoke2d"};
	cGraphicsDrawer *pDrawer = mpInit->mpGame->GetGraphics()->GetDrawer();
	const cVector3f vPos = mpInit->mpGraphicsHelper->GetScreenPos();
	const cVector2f vSize = mpInit->mpGraphicsHelper->GetScreenSize();
	auto Draw = [&](int i, const cColor &color)
	{
		if(!mvLegacyPreviewGfx[i]) mvLegacyPreviewGfx[i] = pDrawer->CreateGfxObject(vFiles[i], vMaterials[i]);
		if(mvLegacyPreviewGfx[i]) pDrawer->DrawGfxObject(mvLegacyPreviewGfx[i], vPos, vSize, color);
	};
	if(state.mfAdaptation > 0) Draw(0, cColor(state.mfAdaptation * 0.85f, state.mfAdaptation * 0.85f));
	if(state.mfLowHealth > 0) Draw(1, cColor(1, state.mfLowHealth));
	if(state.mfBloodHit > 0) Draw(2, cColor(1, state.mfBloodHit * 0.6f));
	if(state.mfIceHit > 0) Draw(3, cColor(1, state.mfIceHit * 0.6f));
	if(state.mfDeath > 0) Draw(4, cColor(state.mfDeath, 0));
	if(state.mfDeathBlack > 0) Draw(5, cColor(state.mfDeathBlack, 0));
	if(state.mfSave > 0) Draw(6, cColor(1, 1) * state.mfSave + cColor(0.847f, 0.333f, 0.02f, 0) * (1 - state.mfSave));
	if(state.mfFlash > 0) Draw(7, cColor(1, state.mfFlash));
	if(state.mfUnderwater > 0)
		Draw(8, cColor(1 - state.mWaterColor.r, 1 - state.mWaterColor.g, 1 - state.mWaterColor.b, 0) * state.mfUnderwater);
}

void cRefView::UpdatePlayerEffects(float afTimeStep)
{
	if(mpInit->mpGame->GetUpdater()->GetCurrentContainerName() != "Default" ||
		!mpInit->mpGame->GetScene()->GetWorld3D() || mpInit->mpMapHandler->IsChangingMap()) return;
	iLowLevelGraphics *pLowLevel = mpInit->mpGame->GetGraphics()->GetLowLevel();
	cPlayer *pPlayer = mpInit->mpPlayer;
	cEffectHandler *pEffects = mpInit->mpEffectHandler;
	mfEffectPreviewClock += afTimeStep;
	if(!msEffectsCapture.empty() && mlEffectsCaptureIndex >= ePreview_Last * 2 + 4 && mlEffectsCaptureIndex < ePreview_Last * 2 + 6)
		mfEffectPreviewClock = mlEffectsCaptureIndex == ePreview_Last * 2 + 4 ? 0.0f : kPif / 4.6f;
	if(!msEffectsCapture.empty() && mlEffectsCaptureIndex >= ePreview_Last * 2 + 9 && mlEffectsCaptureIndex < ePreview_Last * 2 + 11)
		mfEffectPreviewClock = mlEffectsCaptureIndex == ePreview_Last * 2 + 9 ? 0.0f : kPif / 4.6f;
	if(mlEffectAction != 0)
	{
		mlEffectPreview = ePreview_None;
		mbEffectTour = false;
	}
	if(mbEffectTour && mfEffectPreviewClock >= 3.0f)
	{
		mfEffectPreviewClock = 0;
		mlEffectPreview = mlEffectPreview + 1 < ePreview_Last ? mlEffectPreview + 1 : ePreview_Adaptation;
		if(mlEffectPreview == ePreview_Blood) mlEffectPreviewBloodSeed = NextBloodPreviewSeed(mlEffectPreviewBloodSeed);
	}
	switch(mlEffectAction)
	{
		case eAction_Blood:
			pPlayer->GetDamage()->Start(1.0f, ePlayerDamageType_BloodSplash);
			break;
		case eAction_Ice:
			pPlayer->GetDamage()->Start(1.0f, ePlayerDamageType_Ice);
			break;
		case eAction_Flash:
			pEffects->GetFlash()->Start(0.25f, 0.4f, 1.5f);
			break;
		case eAction_Shake:
			pEffects->GetShakeScreen()->Start(0.025f, 0.4f, 0.1f, 0.6f);
			break;
		case eAction_EarRing:
			pPlayer->GetEarRing()->Start(4.0f);
			break;
		case eAction_Fear:
			pPlayer->GetFearFilter()->SetActive(!pPlayer->GetFearFilter()->IsActive());
			break;
		case eAction_LowHealth:
			pPlayer->SetHealth(25);
			break;
		case eAction_Heal:
			pPlayer->SetHealth(100);
			break;
		case eAction_BlackFade:
			mpInit->mpFadeHandler->FadeOut(0.6f);
			mfEffectBlackFade = 1.2f;
			break;
		case eAction_Rock:
			if(mpInit->mpMapHandler->GetCurrentMapName() == "effectslab")
			{
				bool bOn = !pEffects->GetWaveGravity()->IsActive();
				pEffects->GetWaveGravity()->Setup(cMath::ToRad(6), 8, 9.8f, 0);
				pEffects->GetWaveGravity()->SetActive(bOn);
				if(!bOn) mpInit->mpGame->GetScene()->GetWorld3D()->GetPhysicsWorld()->SetGravity(cVector3f(0, -9.8f, 0));
			}
			break;
		default:
			break;
	}
	mlEffectAction = 0;
	if(mfEffectBlackFade > 0)
	{
		mfEffectBlackFade -= afTimeStep;
		if(mfEffectBlackFade <= 0) mpInit->mpFadeHandler->FadeIn(0.6f);
	}

	// The original previews use the same projection changes and history
	// pass as the game. Leaving a preview releases that history immediately.
	cPlayerEffectState preview = EffectPreviewState();
	cRendererPostEffects *pPostEffects = mpInit->mpGame->GetGraphics()->GetRendererPostEffects();
	float fTrail = cMath::Max(preview.mfPanic * 0.8f, preview.mfDeath * 0.7f);
	fTrail = cMath::Max(fTrail, cMath::Max(preview.mfBloodHit, preview.mfIceHit) * 0.92f);
	fTrail = cMath::Max(fTrail, preview.mfSave * 0.8f);
	bool bTrail = !pLowLevel->GetPlayerEffectSettings().mbImproved && mlEffectPreview != ePreview_None && fTrail > 0;
	if(bTrail)
	{
		pPostEffects->SetPlayerImageTrailActive(true);
		pPostEffects->SetPlayerImageTrailAmount(fTrail);
	}
	else if(mbLegacyPreviewTrail)
		pPostEffects->SetPlayerImageTrailActive(false);
	mbLegacyPreviewTrail = bTrail;
	if(mlEffectPreview != ePreview_None)
	{
		cVector2f vScreen = pLowLevel->GetScreenSize();
		bool bImproved = pLowLevel->GetPlayerEffectSettings().mbImproved;
		pPlayer->GetCamera()->SetAspect(vScreen.x / vScreen.y * (bImproved ? 1.0f : 1.0f - preview.mfAdaptation * 0.18f));
		pPlayer->GetCamera()->SetFOV(bImproved ? pPlayer->GetHidden()->GetFeedbackFOV(preview.mfAdaptation, preview.mfPanic, preview.mfPanicPulse) : cMath::ToRad(70 + preview.mfAdaptation * 8 - preview.mfPanic * 8));
	}

	if(msEffectsCapture.empty()) return;
	if(mfEffectsCaptureHealth < 0) mfEffectsCaptureHealth = pPlayer->GetHealth();
	// Fixed snapshots of every preview in each treatment, plus opaque
	// endpoints and zero strength. No gameplay actions or save files.
	const int lCases = ePreview_Last * 2 + 11;
	if(mlEffectsCaptureIndex >= lCases)
	{
		UpdateEffectsChecks();
		return;
	}
	cPlayerEffectSettings settings = pLowLevel->GetPlayerEffectSettings();
	settings.mbImproved = mlEffectsCaptureIndex >= ePreview_Last;
	mlEffectPreview = mlEffectsCaptureIndex % ePreview_Last;
	mfEffectPreviewAmount = 0.75f;
	if(mlEffectsCaptureIndex >= ePreview_Last * 2)
	{
		int lEndpoint = mlEffectsCaptureIndex - ePreview_Last * 2;
		const int vEndpoints[] = {ePreview_Flash, ePreview_Save, ePreview_Death, ePreview_Combined, ePreview_Panic, ePreview_Panic,
			ePreview_Blood, ePreview_Blood, ePreview_Blood, ePreview_DoubleVision, ePreview_DoubleVision};
		mlEffectPreview = vEndpoints[lEndpoint];
		if(lEndpoint >= 6) mlEffectPreviewBloodSeed = lEndpoint == 7 ? NextBloodPreviewSeed(17) : 17;
		if(lEndpoint < 3)
			mfEffectPreviewAmount = 1.0f;
		else if(lEndpoint == 3)
			mfEffectPreviewAmount = 0.0f;
	}
	pLowLevel->SetPlayerEffectSettings(settings);
	if(++mlEffectsCaptureFrame < 30) return;
	if(mlEffectsCaptureFrame == 30)
	{
		if(settings.mbImproved)
		{
			cVector2f vSize = pLowLevel->GetScreenSize();
			float fFov = pPlayer->GetCamera()->GetFOV();
			CheckEffectsCapture(fabsf(pPlayer->GetCamera()->GetAspect() - vSize.x / vSize.y) < 0.0001f, "preview keeps aspect");
			CheckEffectsCapture(fFov >= cMath::ToRad(63) && fFov <= cMath::ToRad(70.001f), "preview zoom remains subtle");
			if(preview.mfAdaptation > 0 && settings.mfAdaptationStrength > 0)
				CheckEffectsCapture(fFov < cMath::ToRad(70), "adaptation preview zooms in");
			if(mlEffectsCaptureIndex == ePreview_Last * 2 + 4) mfEffectsCapturePanicFov = fFov;
			if(mlEffectsCaptureIndex == ePreview_Last * 2 + 5)
				CheckEffectsCapture(mfEffectsCapturePanicFov - fFov > cMath::ToRad(0.5f), "panic preview pulses FOV");
		}
		CheckEffectsCapture(pPlayer->GetHealth() == mfEffectsCaptureHealth, "preview leaves health alone");
		tString sName = settings.mbImproved ? "improved-" : "original-";
		sName += gvEffectFiles[mlEffectPreview];
		if(mlEffectsCaptureIndex >= ePreview_Last * 2)
		{
			int lEndpoint = mlEffectsCaptureIndex - ePreview_Last * 2;
			if(lEndpoint == 4)
				sName += "-trough";
			else if(lEndpoint == 5)
				sName += "-peak";
			else if(lEndpoint >= 6 && lEndpoint <= 8)
				sName += lEndpoint == 6 ? "-seed-a" : (lEndpoint == 7 ? "-seed-b" : "-seed-a-repeat");
			else if(lEndpoint >= 9)
				sName += lEndpoint == 9 ? "-trough" : "-peak";
			else
				sName += mfEffectPreviewAmount == 0 ? "-zero" : "-full";
		}
		pLowLevel->SaveNextFrameToPNG(msEffectsCapture + "/" + sName + ".png");
		Log("EFFECTSHOTS: %s\n", sName.c_str());
	}
	if(mlEffectsCaptureFrame >= 34)
	{
		mlEffectsCaptureFrame = 0;
		++mlEffectsCaptureIndex;
	}
}

void cRefView::CheckEffectsCapture(bool abCondition, const char *asMessage)
{
	if(abCondition) return;
	mbEffectsCaptureFailed = true;
	Log("EFFECTSHOTS: FAIL %s\n", asMessage);
}

// Exercise real owners after the visual gallery: crouch in darkness,
// switch treatments, take a harmless hit, and fade scripted fear in/out.
void cRefView::UpdateEffectsChecks()
{
	mlEffectPreview = ePreview_None;
	iLowLevelGraphics *pLowLevel = mpInit->mpGame->GetGraphics()->GetLowLevel();
	cPlayer *pPlayer = mpInit->mpPlayer;
	cPlayerEffectSettings settings = pLowLevel->GetPlayerEffectSettings();
	float fAspect = pLowLevel->GetScreenSize().x / pLowLevel->GetScreenSize().y;
	int lFrame = ++mlEffectsCheckFrame;
	const cPlayerEffectState &state = pLowLevel->GetPlayerEffectState();
	if(lFrame >= 640 && lFrame < 680 && !mbEffectsCaptureHealthGap && state.mfLowHealth == 0)
	{
		CheckEffectsCapture(state.mfLowHealthVision > 0.65f, "double vision persists between heartbeats");
		pLowLevel->SaveNextFrameToPNG(msEffectsCapture + "/actual-health-between-beats.png");
		mbEffectsCaptureHealthGap = true;
	}
	switch(lFrame)
	{
		case 1:
			settings.mbImproved = true;
			pLowLevel->SetPlayerEffectSettings(settings);
			pPlayer->GetCharacterBody()->SetFeetPosition(cVector3f(-4.5f, 0, -4.5f));
			pPlayer->ChangeMoveState(ePlayerMoveState_Crouch);
			break;
		case 210:
			CheckEffectsCapture(pPlayer->GetHidden()->IsHidden(), "real hiding trigger in dark alcove");
			CheckEffectsCapture(pPlayer->GetHidden()->GetAdaptationAmount() > 0.7f, "real adaptation eases in");
			CheckEffectsCapture(fabsf(pPlayer->GetCamera()->GetAspect() - fAspect) < 0.0001f, "adaptation keeps aspect");
			CheckEffectsCapture(pPlayer->GetCamera()->GetFOV() < cMath::ToRad(70) &&
					pPlayer->GetCamera()->GetFOV() > cMath::ToRad(65),
				"real adaptation uses subtle zoom");
			pLowLevel->SaveNextFrameToPNG(msEffectsCapture + "/actual-adaptation.png");
			break;
		case 215:
			settings.mbImproved = false;
			pLowLevel->SetPlayerEffectSettings(settings);
			break;
		case 245:
			CheckEffectsCapture(pPlayer->GetCamera()->GetAspect() < fAspect * 0.9f, "original adaptation retained");
			settings.mbImproved = true;
			pLowLevel->SetPlayerEffectSettings(settings);
			break;
		case 247:
			CheckEffectsCapture(fabsf(pPlayer->GetCamera()->GetAspect() - fAspect) < 0.0001f &&
					pPlayer->GetCamera()->GetFOV() < cMath::ToRad(70),
				"treatment switch keeps aspect and adapted zoom");
			pPlayer->GetDamage()->Start(1.0f, ePlayerDamageType_BloodSplash);
			mfEffectsCaptureBloodSeed = pPlayer->GetDamage()->GetBloodSeed();
			break;
		case 257:
			CheckEffectsCapture(pPlayer->GetDamage()->GetVisualAmount() > 0.5f, "real hit appears");
			CheckEffectsCapture(pLowLevel->GetPlayerEffectState().mfBloodHit > 0, "hit reaches shader");
			CheckEffectsCapture(pLowLevel->GetPlayerEffectState().mfBloodSeed == mfEffectsCaptureBloodSeed, "blood pattern stays still during impact");
			pPlayer->GetDamage()->Start(1.0f, ePlayerDamageType_BloodSplash);
			CheckEffectsCapture(pPlayer->GetDamage()->GetBloodSeed() == mfEffectsCaptureBloodSeed, "ignored overlapping hit keeps pattern");
			mpInit->mpEffectHandler->GetFlash()->Start(0, 0, 0);
			break;
		case 265:
			CheckEffectsCapture(mpInit->mpEffectHandler->GetFlash()->GetVisualAmount() == 0, "zero-time flash completes");
			break;
		case 290:
			CheckEffectsCapture(pPlayer->GetDamage()->GetVisualAmount() == 0, "real hit clears");
			pPlayer->GetDamage()->Start(1.0f, ePlayerDamageType_BloodSplash);
			CheckEffectsCapture(pPlayer->GetDamage()->GetBloodSeed() != mfEffectsCaptureBloodSeed, "next hit has a new blood pattern");
			pPlayer->GetFearFilter()->SetActive(true);
			break;
		case 365:
			CheckEffectsCapture(pPlayer->GetFearFilter()->GetVisualAmount() > 0.95f, "real fear eases in");
			pPlayer->GetFearFilter()->SetActive(false);
			break;
		case 405:
			CheckEffectsCapture(pPlayer->GetFearFilter()->GetVisualAmount() > 0 &&
					pPlayer->GetFearFilter()->GetVisualAmount() < 0.95f,
				"fear eases out instead of disappearing");
			break;
		case 605:
			CheckEffectsCapture(pPlayer->GetFearFilter()->GetVisualAmount() == 0, "fear clears");
			pPlayer->SetHealth(25);
			break;
		case 620:
			CheckEffectsCapture(pLowLevel->GetPlayerEffectState().mfLowHealth > 0, "heartbeat reaches shader");
			CheckEffectsCapture(state.mfLowHealthVision > 0.5f, "real low health drives double vision");
			pPlayer->ChangeMoveState(ePlayerMoveState_Walk);
			pPlayer->SetStartPos("link01");
			break;
		case 635:
			pLowLevel->SaveNextFrameToPNG(msEffectsCapture + "/actual-low-health.png");
			break;
		case 680:
			CheckEffectsCapture(mbEffectsCaptureHealthGap, "live checks include a gap between heartbeats");
			pPlayer->SetHealth(100);
			pPlayer->GetCharacterBody()->SetFeetPosition(cVector3f(4.5f, -2.2f, 2.5f));
			break;
		case 705:
			CheckEffectsCapture(mpInit->mpEffectHandler->GetUnderwater()->IsActive(), "real underwater trigger");
			CheckEffectsCapture(pLowLevel->GetPlayerEffectState().mfUnderwater > 0, "underwater reaches shader");
			pLowLevel->SaveNextFrameToPNG(msEffectsCapture + "/actual-underwater.png");
			mpInit->mpEffectHandler->GetShakeScreen()->Start(0.01f, 0.1f, 0.1f, 0.1f);
			pPlayer->GetEarRing()->Start(1.0f);
			break;
		case 730:
			CheckEffectsCapture(mpInit->mpEffectHandler->GetShakeScreen()->GetScreenAdd() == cVector3f(0, 0, 0), "shake clears");
			pPlayer->GetEarRing()->Stop(true);
			pPlayer->SetStartPos("link01");
			SetOpen(true);
			break;
		case 735:
			pLowLevel->SaveNextFrameToPNG(msEffectsCapture + "/effects-panel.png");
			break;
		case 740:
			SetOpen(false);
			break;
		case 745:
			CheckEffectsCapture(pPlayer->GetHealth() == 100, "live tests restore health");
			CheckEffectsCapture(state.mfLowHealth == 0 && state.mfLowHealthVision == 0, "healing clears the red veil and double vision");
			CheckEffectsCapture(!mpInit->mpEffectHandler->GetUnderwater()->IsActive(), "underwater clears on leaving pool");
			break;
		case 750:
		{
			iGameEntity *pSave = mpInit->mpMapHandler->GetGameEntity("savepoint");
			CheckEffectsCapture(pSave && pSave->GetType() == eGameEntityType_SaveArea, "lab save point exists");
			if(pSave && pSave->GetType() == eGameEntityType_SaveArea)
				mpInit->mpEffectHandler->GetSaveEffect()->NormalSave(cVector3f(-1.3f, 1.05f, -4.9f), static_cast<cGameSaveArea *>(pSave));
			break;
		}
		case 805:
			CheckEffectsCapture(pPlayer->GetCamera()->GetFOV() > cMath::ToRad(65), "improved save keeps a comfortable FOV");
			CheckEffectsCapture(!mpInit->mpGame->GetGraphics()->GetRendererPostEffects()->GetImageTrailActive(), "improved save replaces image trail with scene glow");
			CheckEffectsCapture(pLowLevel->GetPlayerEffectState().mfSave > 0, "save haze reaches shader");
			pLowLevel->SaveNextFrameToPNG(msEffectsCapture + "/actual-save.png");
			break;
		case 806:
			settings.mbImproved = false;
			pLowLevel->SetPlayerEffectSettings(settings);
			break;
		case 807:
			CheckEffectsCapture(pPlayer->GetCamera()->GetFOV() < cMath::ToRad(60), "original save zoom retained");
			CheckEffectsCapture(mpInit->mpGame->GetGraphics()->GetRendererPostEffects()->GetImageTrailActive(), "original save trail retained");
			settings.mbImproved = true;
			pLowLevel->SetPlayerEffectSettings(settings);
			break;
		case 809:
			CheckEffectsCapture(pPlayer->GetCamera()->GetFOV() > cMath::ToRad(65) &&
					!mpInit->mpGame->GetGraphics()->GetRendererPostEffects()->GetImageTrailActive(),
				"switching back during a save restores improved treatment");
			break;
		case 1070:
		{
			iGameEntity *pSave = mpInit->mpMapHandler->GetGameEntity("savepoint");
			CheckEffectsCapture(pSave && pSave->GetType() == eGameEntityType_SaveArea &&
					static_cast<cGameSaveArea *>(pSave)->GetHasBeenUsed() && pPlayer->mlStat_NumOfSaves > 0,
				"normal save completes and records the used save point");
			CheckEffectsCapture(mpInit->mpEffectHandler->GetSaveEffect()->GetVisualAmount() == 0 &&
					!mpInit->mpGame->GetGraphics()->GetRendererPostEffects()->GetImageTrailActive(),
				"normal save clears its haze and trail");
			mpInit->mpEffectHandler->GetSaveEffect()->AutoSave();
			break;
		}
		case 1105:
			CheckEffectsCapture(mpInit->mpEffectHandler->GetSaveEffect()->GetVisualAmount() > 0.5f, "automatic save haze appears");
			break;
		case 1210:
			CheckEffectsCapture(mpInit->mpEffectHandler->GetSaveEffect()->GetVisualAmount() == 0, "automatic save haze clears");
			{
				settings.mbImproved = false;
				pLowLevel->SetPlayerEffectSettings(settings);
				iGameEntity *pSave = mpInit->mpMapHandler->GetGameEntity("savepoint");
				if(pSave && pSave->GetType() == eGameEntityType_SaveArea)
					mpInit->mpEffectHandler->GetSaveEffect()->NormalSave(cVector3f(-1.3f, 1.05f, -4.9f), static_cast<cGameSaveArea *>(pSave));
				break;
			}
		case 1220:
			CheckEffectsCapture(mpInit->mpGame->GetGraphics()->GetRendererPostEffects()->GetImageTrailActive() &&
					mpInit->mpEffectHandler->GetSaveEffect()->GetVisualAmount() > 0,
				"interrupted original save starts its trail and overlay");
			mpInit->mpEffectHandler->GetSaveEffect()->Reset();
			CheckEffectsCapture(!mpInit->mpGame->GetGraphics()->GetRendererPostEffects()->GetImageTrailActive() &&
					mpInit->mpEffectHandler->GetSaveEffect()->GetVisualAmount() == 0,
				"save reset clears its trail and overlay");
			pPlayer->SetActive(true);
			pPlayer->GetLookAt()->SetActive(false);
			settings.mbImproved = true;
			pLowLevel->SetPlayerEffectSettings(settings);
			break;
		case 1225:
			Log("EFFECTSHOTS: done, 41 frames and live effect checks, %d failure(s)\n", mbEffectsCaptureFailed ? 1 : 0);
			mpInit->mpGame->Exit();
			break;
		default:
			break;
	}
}
