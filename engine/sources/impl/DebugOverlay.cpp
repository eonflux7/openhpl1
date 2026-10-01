/*
 * This file is part of Overture Remade, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#include "impl/DebugOverlay.h"
#include "impl/GpuProfiler.h"
#include "system/LowLevelSystem.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>

namespace hpl {

	//-----------------------------------------------------------------------

	cDebugOverlay::cDebugOverlay()
	{
		const char *pActive = getenv("OVERTURE_DEBUG_OVERLAY");
		mbActive = pActive && pActive[0] && pActive[0] != '0';
		mbInitialized = false;
		mbFailed = false;
		mfFrameMs = 0;
	}

	cDebugOverlay::~cDebugOverlay()
	{
	}

	//-----------------------------------------------------------------------

	bool cDebugOverlay::Init(SDL_Window *apWindow)
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		// The working directory is the game's data directory: keep ImGui's
		// files out of it.
		io.IniFilename = nullptr;
		io.LogFilename = nullptr;
		io.ConfigFlags |= ImGuiConfigFlags_NoMouse | ImGuiConfigFlags_NoMouseCursorChange |
							ImGuiConfigFlags_NoKeyboard;

		// Sized for 1080 lines, larger above that.
		int lW = 0, lH = 0;
		SDL_GetWindowSizeInPixels(apWindow, &lW, &lH);
		float fScale = std::max(1.0f, lH / 1080.0f);

		ImGui::StyleColorsDark();
		ImGuiStyle& style = ImGui::GetStyle();
		style.ScaleAllSizes(fScale);
		style.FontSizeBase = std::round(15.0f * fScale);
		style.WindowRounding = 0;

		std::string sFont = std::string(SDL_GetBasePath() ? SDL_GetBasePath() : "") +
							"fonts/Cousine-Regular.ttf";
		if(io.Fonts->AddFontFromFileTTF(sFont.c_str()) == nullptr)
			io.Fonts->AddFontDefault();

		if(!ImGui_ImplSDL3_InitForOpenGL(apWindow, SDL_GL_GetCurrentContext()) ||
			!ImGui_ImplOpenGL3_Init("#version 330"))
		{
			Error("Couldn't set up the debug overlay (ImGui)\n");
			ImGui::DestroyContext();
			return false;
		}

		mbInitialized = true;
		return true;
	}

	//-----------------------------------------------------------------------

	void cDebugOverlay::Exit()
	{
		if(!mbInitialized) return;
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL3_Shutdown();
		ImGui::DestroyContext();
		mbInitialized = false;
	}

	//-----------------------------------------------------------------------

	void cDebugOverlay::Draw(SDL_Window *apWindow, cGpuProfiler *apProfiler, const cVector2l& avRenderSize,
							float afRenderScale)
	{
		std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
		float fFrameMs = std::chrono::duration<float, std::milli>(now - mLastFrame).count();
		mLastFrame = now;
		mfFrameMs = mfFrameMs + (fFrameMs - mfFrameMs) * 0.05f;

		apProfiler->SetTiming(mbActive);
		if(!mbActive || mbFailed) return;
		if(!mbInitialized && !Init(apWindow))
		{
			mbFailed = true;
			return;
		}

		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		ImGui::SetNextWindowPos(ImVec2(8, 8), ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.7f);
		ImGui::Begin("Renderer", nullptr, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoInputs |
					ImGuiWindowFlags_NoSavedSettings);

		ImGui::Text("Frame %6.2f ms (%4.0f fps)", mfFrameMs, mfFrameMs > 0 ? 1000.0f / mfFrameMs : 0.0f);
		ImGui::Text("GPU   %6.2f ms", apProfiler->GetGpuFrameMs());
		ImGui::Text("Draw calls %d, triangles %d", apProfiler->GetDrawCalls(), apProfiler->GetTriangles());
		ImGui::Text("Scene %dx%d (%.0f%%)", avRenderSize.x, avRenderSize.y, afRenderScale * 100.0f);

		if(ImGui::BeginTable("sections", 3, ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_RowBg))
		{
			ImGui::TableSetupColumn("GPU section");
			ImGui::TableSetupColumn("ms");
			ImGui::TableSetupColumn("count");
			ImGui::TableHeadersRow();
			for(const cGpuProfiler::cSectionResult& section : apProfiler->GetSections())
			{
				ImGui::TableNextRow();
				ImGui::TableNextColumn();
				ImGui::Text("%*s%s", section.mlDepth * 2, "", section.msName.c_str());
				ImGui::TableNextColumn();
				ImGui::Text("%6.2f", section.mfAverageMs);
				ImGui::TableNextColumn();
				if(section.mlCount > 1) ImGui::Text("%d", section.mlCount);
			}
			ImGui::EndTable();
		}

		ImGui::End();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	//-----------------------------------------------------------------------

}
