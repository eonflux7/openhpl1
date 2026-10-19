/*
 * This file is part of OpenHPL1, licensed under the GNU GPL v3 or
 * later. See COPYING.
 */
#include "core/GameLauncher.h"

#include "core/SmokeTest.h"
#include "core/RefShots.h"

#include <SDL3/SDL.h>

#include <imgui.h>
#include <imgui_impl_sdl3.h>
#include <imgui_impl_sdlrenderer3.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>

#if defined(WIN32)
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#ifndef NOMINMAX
		#define NOMINMAX
	#endif
	#include <windows.h>
	#include <shellapi.h>
#endif

//-----------------------------------------------------------------------

static const char *gvGameKeys[eGameId_LastEnum] = {"overture", "blackplague", "requiem"};

const char* cGameLauncher::GetGameKey(eGameId aId)
{
	return gvGameKeys[aId];
}

static eGameId GameFromKey(const tString& asKey)
{
	tString sKey = cString::ToLowerCase(asKey);
	for(int i=0; i<eGameId_LastEnum; ++i)
		if(sKey == gvGameKeys[i]) return (eGameId)i;
	return eGameId_LastEnum;
}

//-----------------------------------------------------------------------

// GAMES.CFG: one "key=value" a line

static std::filesystem::path GamesFile()
{
	tString sDir = cString::To8Char(cInit::GetPersonalDir());
	if(sDir != "" && sDir.back() != '/' && sDir.back() != '\\') sDir += "/";
	return std::filesystem::path(sDir + cString::To8Char(tWString(PERSONAL_RELATIVEROOT PERSONAL_RELATIVEGAME_PARENT)) +
								 "games.cfg");
}

static std::vector<std::string> ReadLines()
{
	std::vector<std::string> vLines;
	std::ifstream file(GamesFile());
	std::string sLine;
	while(std::getline(file, sLine))
	{
		if(sLine != "" && sLine.back() == '\r') sLine.pop_back();
		if(sLine != "") vLines.push_back(sLine);
	}
	return vLines;
}

static std::string ReadValue(const std::string& asKey)
{
	std::string sKey = asKey + "=";
	for(const std::string& sLine : ReadLines())
		if(sLine.compare(0, sKey.size(), sKey) == 0) return sLine.substr(sKey.size());
	return "";
}

static void WriteValue(const std::string& asKey, const std::string& asValue)
{
	if(ReadValue(asKey) == asValue) return;

	std::vector<std::string> vLines;
	std::string sKey = asKey + "=";
	for(const std::string& sLine : ReadLines())
		if(sLine.compare(0, sKey.size(), sKey) != 0) vLines.push_back(sLine);
	vLines.push_back(sKey + asValue);

	std::filesystem::path file = GamesFile();
	std::error_code err;
	std::filesystem::create_directories(file.parent_path(), err);
	std::ofstream out(file, std::ios::trunc);
	for(const std::string& sLine : vLines) out << sLine << "\n";
}

void cGameLauncher::RememberGame(eGameId aId, const tString& asFolder)
{
	WriteValue(gvGameKeys[aId], asFolder);
	WriteValue("last", gvGameKeys[aId]);
}

bool cGameLauncher::GetAskAtStart()
{
	return ReadValue("ask") == "1";
}

void cGameLauncher::SetAskAtStart(bool abX)
{
	WriteValue("ask", abX ? "1" : "0");
}

//-----------------------------------------------------------------------

// FINDING THE GAMES

// A folder holds the game when its first map is there
static bool HasGameData(eGameId aId, const std::string& asFolder)
{
	if(asFolder.empty()) return false;
	std::error_code err;
	return std::filesystem::exists(std::filesystem::path(asFolder) / cInit::GetGameStartFile(aId), err);
}

// The folder itself, or redist in it (Overture on Steam for Windows); "" if neither has the game
static std::string GameDataIn(eGameId aId, const std::string& asFolder)
{
	if(HasGameData(aId, asFolder)) return asFolder;
	std::string sRedist = (std::filesystem::path(asFolder) / "redist").string();
	if(HasGameData(aId, sRedist)) return sRedist;
	return "";
}

// A Steam installation and the other libraries its libraryfolders.vdf lists:
//   "path"		"D:\\SteamLibrary"
static void AddSteamLibraries(const std::string& asSteam, std::vector<std::string>& avLibraries)
{
	if(asSteam.empty()) return;
	avLibraries.push_back(asSteam);

	std::ifstream file(std::filesystem::path(asSteam) / "steamapps" / "libraryfolders.vdf");
	std::stringstream text;
	text << file.rdbuf();
	const std::string sText = text.str();

	size_t lPos = 0;
	while((lPos = sText.find("\"path\"", lPos)) != std::string::npos)
	{
		lPos = sText.find('"', lPos + 6);
		if(lPos == std::string::npos) break;
		std::string sPath;
		for(++lPos; lPos < sText.size() && sText[lPos] != '"'; ++lPos)
		{
			if(sText[lPos] == '\\' && lPos + 1 < sText.size()) ++lPos;
			sPath += sText[lPos];
		}
		if(sPath != "") avLibraries.push_back(sPath);
	}
}

#if defined(WIN32)
static std::string RegistryString(HKEY aRoot, const wchar_t *asKey, const wchar_t *asValue)
{
	wchar_t sBuffer[MAX_PATH * 4];
	DWORD lSize = sizeof(sBuffer);
	if(RegGetValueW(aRoot, asKey, asValue, RRF_RT_REG_SZ, nullptr, sBuffer, &lSize) != ERROR_SUCCESS) return "";
	return cString::To8Char(tWString(sBuffer));
}
#endif

// Where a game may be, most likely first
static std::vector<std::string> CandidateFolders(eGameId aId)
{
	std::vector<std::string> vFolders;

	// Black Plague and Requiem are installed together
	vFolders.push_back(ReadValue(gvGameKeys[aId]));
	if(aId == eGameId_BlackPlague) vFolders.push_back(ReadValue(gvGameKeys[eGameId_Requiem]));
	if(aId == eGameId_Requiem) vFolders.push_back(ReadValue(gvGameKeys[eGameId_BlackPlague]));

	const char *pGameDir = SDL_getenv("OVERTURE_GAME_DIR");
	if(pGameDir && pGameDir[0]) vFolders.push_back(pGameDir);
	{
		std::error_code err;
		vFolders.push_back(std::filesystem::current_path(err).string());
	}
	if(SDL_GetBasePath()) vFolders.push_back(SDL_GetBasePath());

	std::vector<std::string> vSteamLibraries;
#if defined(WIN32)
	AddSteamLibraries(RegistryString(HKEY_CURRENT_USER, L"Software\\Valve\\Steam", L"SteamPath"), vSteamLibraries);
	AddSteamLibraries(RegistryString(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\Valve\\Steam", L"InstallPath"),
					  vSteamLibraries);
	const char *pProgramFiles = SDL_getenv("ProgramFiles(x86)");
	if(pProgramFiles) AddSteamLibraries(std::string(pProgramFiles) + "\\Steam", vSteamLibraries);
#else
	const char *pHome = SDL_getenv("HOME");
	if(pHome)
	{
		for(const char *pSteam : {"/.local/share/Steam", "/.steam/steam", "/.var/app/com.valvesoftware.Steam/.local/share/Steam",
								  "/snap/steam/common/.local/share/Steam"})
			AddSteamLibraries(std::string(pHome) + pSteam, vSteamLibraries);
	}
#endif
	std::vector<const char*> vSteamNames;
	if(aId == eGameId_Overture) vSteamNames = {"Penumbra Overture"};
	else vSteamNames = {"Penumbra Black Plague", "Penumbra Requiem"};
	for(const std::string& sLibrary : vSteamLibraries)
		for(const char *pName : vSteamNames)
			vFolders.push_back((std::filesystem::path(sLibrary) / "steamapps" / "common" / pName).string());

#if defined(WIN32)
	// GOG's games each have a key with their folder
	HKEY hGames;
	if(RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\WOW6432Node\\GOG.com\\Games", 0, KEY_READ, &hGames) == ERROR_SUCCESS)
	{
		wchar_t sName[256];
		for(DWORD i=0; ; ++i)
		{
			DWORD lNameSize = 256;
			if(RegEnumKeyExW(hGames, i, sName, &lNameSize, nullptr, nullptr, nullptr, nullptr) != ERROR_SUCCESS) break;
			vFolders.push_back(RegistryString(hGames, sName, L"path"));
		}
		RegCloseKey(hGames);
	}

	// Retail installs
	for(const char *pVar : {"ProgramFiles(x86)", "ProgramFiles"})
	{
		const char *pDir = SDL_getenv(pVar);
		if(pDir == nullptr) continue;
		for(const char *pName : vSteamNames) vFolders.push_back(std::string(pDir) + "\\" + pName);
	}
#endif
	return vFolders;
}

tString cGameLauncher::FindGameFolder(eGameId aId)
{
	for(const std::string& sFolder : CandidateFolders(aId))
	{
		std::string sData = GameDataIn(aId, sFolder);
		if(sData != "") return sData;
	}
	return "";
}

//-----------------------------------------------------------------------

// THE CHOOSER

// A folder dialog's answer, which may come on another thread. It outlives
// the chooser, in case the dialog is still open when the chooser closes.
struct cFolderRequest
{
	std::mutex mMutex;
	bool mbPending = false;
	bool mbDone = false;
	eGameId mGame = eGameId_LastEnum;
	std::string msFolder;
	std::string msError;
};
static cFolderRequest gFolderRequest;

static void SDLCALL OnFolderChosen(void *apUserData, const char * const *apFileList, int alFilter)
{
	cFolderRequest *pRequest = (cFolderRequest*)apUserData;
	std::lock_guard<std::mutex> lock(pRequest->mMutex);
	pRequest->mbDone = true;
	// Nothing in the list: cancelled
	if(apFileList == nullptr) pRequest->msError = SDL_GetError();
	else if(apFileList[0]) pRequest->msFolder = apFileList[0];
}

struct cChoice
{
	std::string msFolder;
	// Why a folder didn't do
	std::string msNote;
	// No folder dialog: the folder is typed
	bool mbTyping = false;
	char msTyped[1024] = {};
};

static std::string NotFoundText(eGameId aId, const cChoice *apChoices)
{
	if(aId == eGameId_Requiem && apChoices[eGameId_BlackPlague].msFolder != "")
		return "Not in Black Plague's folder: Requiem is installed there as an expansion.";
	if(aId == eGameId_Requiem)
		return "Not found. Pick the folder of Penumbra: Black Plague with Requiem installed.";
	return std::string("Not found. Pick the folder of ") + cInit::GetGameName(aId) + ".";
}

static void UseFolder(eGameId aId, const std::string& asFolder, cChoice *apChoices)
{
	cChoice& choice = apChoices[aId];
	std::string sData = GameDataIn(aId, asFolder);
	if(sData == "")
	{
		choice.msNote = std::string("No ") + cInit::GetGameName(aId) + " in " + asFolder;
		return;
	}
	WriteValue(gvGameKeys[aId], sData);
	choice.msNote = "";
	choice.mbTyping = false;
	// Black Plague's folder may have Requiem too
	for(int i=0; i<eGameId_LastEnum; ++i) apChoices[i].msFolder = cGameLauncher::FindGameFolder((eGameId)i);
}

// When there is no window: buttons for the games that were found
static eGameId ChooseWithMessageBox(const std::string& asProblem)
{
	std::vector<SDL_MessageBoxButtonData> vButtons;
	for(int i=0; i<eGameId_LastEnum; ++i)
		if(cGameLauncher::FindGameFolder((eGameId)i) != "")
			vButtons.push_back({0, i, cInit::GetGameName((eGameId)i)});
	std::string sText = asProblem == "" ? "" : asProblem + "\n\n";
	sText += vButtons.empty() ? "No Penumbra game was found. Set OVERTURE_GAME_DIR to the game's folder."
							  : "Which game?";
	vButtons.push_back({SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, -1, "Quit"});

	SDL_MessageBoxData data = {};
	data.flags = SDL_MESSAGEBOX_INFORMATION | SDL_MESSAGEBOX_BUTTONS_LEFT_TO_RIGHT;
	data.title = "OpenHPL1";
	data.message = sText.c_str();
	data.numbuttons = (int)vButtons.size();
	data.buttons = vButtons.data();
	int lButton = -1;
	if(!SDL_ShowMessageBox(&data, &lButton) || lButton < 0) return eGameId_LastEnum;
	return (eGameId)lButton;
}

static eGameId Choose(eGameId aPreferred, const std::string& asProblem)
{
	if(!SDL_InitSubSystem(SDL_INIT_VIDEO))
	{
		SDL_Log("Game chooser: no video (%s)", SDL_GetError());
		return ChooseWithMessageBox(asProblem);
	}

	// Sized as Dear ImGui's SDL3 example does for the display's scale
	float fScale = SDL_GetDisplayContentScale(SDL_GetPrimaryDisplay());
	if(fScale <= 0) fScale = 1;
	SDL_Window *pWindow = nullptr;
	SDL_Renderer *pRenderer = nullptr;
	if(!SDL_CreateWindowAndRenderer("OpenHPL1", (int)(620 * fScale), (int)(480 * fScale),
									SDL_WINDOW_HIDDEN | SDL_WINDOW_HIGH_PIXEL_DENSITY, &pWindow, &pRenderer))
	{
		SDL_Log("Game chooser: no window (%s)", SDL_GetError());
		SDL_QuitSubSystem(SDL_INIT_VIDEO);
		return ChooseWithMessageBox(asProblem);
	}
	SDL_SetRenderVSync(pRenderer, 1);
	SDL_SetWindowPosition(pWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_ShowWindow(pWindow);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	// Nothing written to the working directory
	io.IniFilename = nullptr;
	io.LogFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;

	ImGui::StyleColorsDark();
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowPadding = ImVec2(24, 20);
	style.FramePadding = ImVec2(10, 6);
	style.ItemSpacing = ImVec2(10, 6);
	style.FrameRounding = 4;
	style.ScaleAllSizes(fScale);
	style.FontScaleDpi = fScale;
	style.FontSizeBase = 17;

	// The fonts next to the executable; ImGui's own without them
	std::string sFonts = std::string(SDL_GetBasePath() ? SDL_GetBasePath() : "") + "fonts/";
	ImFontConfig fontConfig;
	fontConfig.Flags |= ImFontFlags_NoLoadError;
	ImFont *pFont = io.Fonts->AddFontFromFileTTF((sFonts + "Arimo-Regular.ttf").c_str(), 0, &fontConfig);
	if(pFont == nullptr) pFont = io.Fonts->AddFontDefault();
	ImFont *pBoldFont = io.Fonts->AddFontFromFileTTF((sFonts + "Arimo-Bold.ttf").c_str(), 0, &fontConfig);
	if(pBoldFont == nullptr) pBoldFont = pFont;

	ImGui_ImplSDL3_InitForSDLRenderer(pWindow, pRenderer);
	ImGui_ImplSDLRenderer3_Init(pRenderer);

	cChoice vChoices[eGameId_LastEnum];
	for(int i=0; i<eGameId_LastEnum; ++i) vChoices[i].msFolder = cGameLauncher::FindGameFolder((eGameId)i);
	bool bAskAtStart = cGameLauncher::GetAskAtStart();

	const ImVec4 colorDim(0.62f, 0.62f, 0.66f, 1);
	const ImVec4 colorWarning(0.95f, 0.72f, 0.35f, 1);

	eGameId chosen = eGameId_LastEnum;
	bool bQuit = false;
	while(chosen == eGameId_LastEnum && bQuit == false)
	{
		// Idle while nothing happens, but often enough to see the dialog's answer
		SDL_Event event;
		if(SDL_WaitEventTimeout(&event, 100))
		{
			do
			{
				ImGui_ImplSDL3_ProcessEvent(&event);
				if(event.type == SDL_EVENT_QUIT) bQuit = true;
				if(event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(pWindow))
					bQuit = true;
			}
			while(SDL_PollEvent(&event));
		}

		bool bPending;
		{
			std::lock_guard<std::mutex> lock(gFolderRequest.mMutex);
			if(gFolderRequest.mbDone)
			{
				cChoice& choice = vChoices[gFolderRequest.mGame];
				if(gFolderRequest.msError != "")
				{
					choice.mbTyping = true;
					choice.msNote = "No folder dialog could be opened (" + gFolderRequest.msError + "). Type the folder:";
				}
				else if(gFolderRequest.msFolder != "")
				{
					UseFolder(gFolderRequest.mGame, gFolderRequest.msFolder, vChoices);
				}
				gFolderRequest.mbDone = false;
				gFolderRequest.mbPending = false;
			}
			bPending = gFolderRequest.mbPending;
		}

		ImGui_ImplSDLRenderer3_NewFrame();
		ImGui_ImplSDL3_NewFrame();
		ImGui::NewFrame();

		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::SetNextWindowSize(io.DisplaySize);
		ImGui::Begin("##chooser", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
						ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

		ImGui::PushFont(pBoldFont, 26);
		ImGui::TextUnformatted("OpenHPL1");
		ImGui::PopFont();
		ImGui::TextColored(colorDim, "Choose a game");
		if(asProblem != "")
		{
			ImGui::PushStyleColor(ImGuiCol_Text, colorWarning);
			ImGui::TextWrapped("%s", asProblem.c_str());
			ImGui::PopStyleColor();
		}
		ImGui::Spacing();
		ImGui::Spacing();

		// Nothing else while a folder dialog is open
		ImGui::BeginDisabled(bPending);
		const float fBrowseWidth = ImGui::CalcTextSize("Browse...").x + style.FramePadding.x * 4;
		const float fRowHeight = ImGui::GetFrameHeight() * 1.5f;
		for(int i=0; i<eGameId_LastEnum; ++i)
		{
			eGameId game = (eGameId)i;
			cChoice& choice = vChoices[i];
			ImGui::PushID(i);

			ImGui::BeginDisabled(choice.msFolder == "");
			ImGui::PushFont(pBoldFont, 0);
			if(ImGui::Button(cInit::GetGameName(game), ImVec2(ImGui::GetContentRegionAvail().x - fBrowseWidth - style.ItemSpacing.x, fRowHeight)))
				chosen = game;
			ImGui::PopFont();
			if(game == aPreferred) ImGui::SetItemDefaultFocus();
			ImGui::EndDisabled();

			ImGui::SameLine();
			if(ImGui::Button("Browse...", ImVec2(fBrowseWidth, fRowHeight)))
			{
				{
					std::lock_guard<std::mutex> lock(gFolderRequest.mMutex);
					gFolderRequest.mbPending = true;
					gFolderRequest.mGame = game;
					gFolderRequest.msFolder = "";
					gFolderRequest.msError = "";
				}
				// Not locked: the dialog may answer at once, on this thread
				SDL_ShowOpenFolderDialog(OnFolderChosen, &gFolderRequest, pWindow,
										 choice.msFolder != "" ? choice.msFolder.c_str() : nullptr, false);
			}

			ImGui::PushStyleColor(ImGuiCol_Text, colorDim);
			ImGui::TextWrapped("%s", choice.msFolder != "" ? choice.msFolder.c_str() : NotFoundText(game, vChoices).c_str());
			ImGui::PopStyleColor();
			if(choice.msNote != "")
			{
				ImGui::PushStyleColor(ImGuiCol_Text, colorWarning);
				ImGui::TextWrapped("%s", choice.msNote.c_str());
				ImGui::PopStyleColor();
			}
			if(choice.mbTyping)
			{
				const float fUseWidth = ImGui::CalcTextSize("Use").x + style.FramePadding.x * 4;
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - fUseWidth - style.ItemSpacing.x);
				bool bEntered = ImGui::InputText("##folder", choice.msTyped, sizeof(choice.msTyped),
												 ImGuiInputTextFlags_EnterReturnsTrue);
				ImGui::SameLine();
				if((ImGui::Button("Use", ImVec2(fUseWidth, 0)) || bEntered) && choice.msTyped[0])
					UseFolder(game, choice.msTyped, vChoices);
			}

			ImGui::PopID();
			ImGui::Spacing();
			ImGui::Spacing();
		}
		ImGui::EndDisabled();

		// At the bottom
		const float fQuitWidth = ImGui::CalcTextSize("Quit").x + style.FramePadding.x * 6;
		ImGui::SetCursorPosY(std::max(ImGui::GetCursorPosY(),
									  ImGui::GetWindowHeight() - style.WindowPadding.y - ImGui::GetFrameHeight()));
		if(ImGui::Checkbox("Show this at every start", &bAskAtStart)) cGameLauncher::SetAskAtStart(bAskAtStart);
		ImGui::SameLine(ImGui::GetWindowWidth() - style.WindowPadding.x - fQuitWidth);
		if(ImGui::Button("Quit", ImVec2(fQuitWidth, 0))) bQuit = true;
		if(ImGui::IsKeyPressed(ImGuiKey_Escape) && bPending == false) bQuit = true;

		ImGui::End();

		ImGui::Render();
		SDL_SetRenderScale(pRenderer, io.DisplayFramebufferScale.x, io.DisplayFramebufferScale.y);
		SDL_SetRenderDrawColor(pRenderer, 15, 15, 17, 255);
		SDL_RenderClear(pRenderer);
		ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), pRenderer);
		SDL_RenderPresent(pRenderer);
	}

	ImGui_ImplSDLRenderer3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
	SDL_DestroyRenderer(pRenderer);
	SDL_DestroyWindow(pWindow);
	SDL_QuitSubSystem(SDL_INIT_VIDEO);

	return chosen;
}

//-----------------------------------------------------------------------

// Smoke tests, captures and checks set the game and folder themselves: they
// never wait for a chooser
static bool IsAutomated()
{
	if(cSmokeTest::IsRequested() || cRefShots::IsRequested()) return true;
	for(const char *pVar : {"OVERTURE_CHECKS", "OVERTURE_EFFECTS_CAPTURE", "OVERTURE_DOG_REGRESSIONS", "OVERTURE_HIDDEN"})
	{
		const char *pValue = SDL_getenv(pVar);
		if(pValue && pValue[0] && pValue[0] != '0') return true;
	}
	return false;
}

bool cGameLauncher::Run(const tString& asCommandLine)
{
	// The game asked for
	eGameId asked = eGameId_LastEnum;
	bool bChoose = false;
	{
		std::istringstream args(asCommandLine);
		tString sArg;
		while(args >> sArg)
		{
			if(sArg.compare(0, 7, "--game=") == 0)
			{
				asked = GameFromKey(sArg.substr(7));
				if(asked == eGameId_LastEnum) SDL_Log("--game: '%s' is not overture, blackplague or requiem", sArg.c_str() + 7);
			}
			if(sArg == "--choose-game") bChoose = true;
		}
	}
	const char *pGame = SDL_getenv("OVERTURE_GAME");
	if(asked == eGameId_LastEnum && pGame && pGame[0])
	{
		asked = GameFromKey(pGame);
		if(asked == eGameId_LastEnum) SDL_Log("OVERTURE_GAME '%s' is not overture, blackplague or requiem", pGame);
	}

	std::string sCurrent;
	{
		std::error_code err;
		sCurrent = std::filesystem::current_path(err).string();
	}

	const eGameId last = GameFromKey(ReadValue("last"));
	eGameId game = eGameId_LastEnum;
	std::string sFolder, sProblem;
	if(asked != eGameId_LastEnum)
	{
		sFolder = GameDataIn(asked, sCurrent);
		if(sFolder == "") sFolder = FindGameFolder(asked);
		if(sFolder != "") game = asked;
		else sProblem = std::string(cInit::GetGameName(asked)) + " wasn't found. Pick its folder, or another game.";
	}
	else if(bChoose == false)
	{
		// The data in the working directory: Black Plague's folder has its
		// first map (Requiem, in the same folder, has to be asked for)
		for(eGameId id : {eGameId_BlackPlague, eGameId_Overture})
		{
			sFolder = GameDataIn(id, sCurrent);
			if(sFolder != "") { game = id; break; }
		}
		if(game == eGameId_LastEnum && last != eGameId_LastEnum && GetAskAtStart() == false)
		{
			sFolder = FindGameFolder(last);
			if(sFolder != "") game = last;
			else sProblem = std::string(cInit::GetGameName(last)) + " is no longer where it was.";
		}
	}

	if(game == eGameId_LastEnum)
	{
		// As before there was a chooser: the game asked for, or Overture, in
		// the working directory (and its errors)
		if(IsAutomated())
		{
			cInit::SetGameId(asked != eGameId_LastEnum ? asked : eGameId_Overture);
			return true;
		}

		game = Choose(asked != eGameId_LastEnum ? asked : last, sProblem);
		if(game == eGameId_LastEnum) return false;
		sFolder = FindGameFolder(game);
	}

	std::error_code err;
	std::filesystem::current_path(std::filesystem::path(sFolder), err);
	if(err)
	{
		std::string sMessage = std::string("Couldn't go to the folder of ") + cInit::GetGameName(game) + ":\n" + sFolder;
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "OpenHPL1", sMessage.c_str(), nullptr);
		return false;
	}
	cInit::SetGameId(game);
	return true;
}

//-----------------------------------------------------------------------

#if defined(WIN32)
void cGameLauncher::StartAgain(eGameId aId)
{
	// The test hooks are for the game they were set for; the new process
	// inherits this environment
	for(const wchar_t *pVar : {L"OVERTURE_GAME", L"OVERTURE_MAP", L"OVERTURE_SCRIPT", L"OVERTURE_SMOKE"})
		SetEnvironmentVariableW(pVar, nullptr);

	// The same arguments as this run had, but for --game, each quoted
	std::wstring sCommandLine;
	int lArgs = 0;
	LPWSTR *pArgs = CommandLineToArgvW(GetCommandLineW(), &lArgs);
	for(int i=0; pArgs && i<lArgs; ++i)
	{
		std::wstring sArg = pArgs[i];
		if(i > 0 && (sArg.compare(0, 7, L"--game=") == 0 || sArg == L"--choose-game")) continue;

		std::wstring sQuoted = L"\"";
		size_t lSlashes = 0;
		for(wchar_t c : sArg)
		{
			if(c == L'\\') { ++lSlashes; continue; }
			// Backslashes are literal unless a quote follows them
			sQuoted.append(c == L'"' ? lSlashes * 2 + 1 : lSlashes, L'\\');
			lSlashes = 0;
			sQuoted += c;
		}
		sQuoted.append(lSlashes * 2, L'\\');
		sQuoted += L"\"";
		if(sCommandLine != L"") sCommandLine += L" ";
		sCommandLine += sQuoted;
	}
	if(pArgs) LocalFree(pArgs);
	sCommandLine += L" --game=" + cString::To16Char(gvGameKeys[aId]);

	wchar_t sExe[MAX_PATH * 4];
	DWORD lExeLength = GetModuleFileNameW(nullptr, sExe, MAX_PATH * 4);
	STARTUPINFOW startInfo = {};
	startInfo.cb = sizeof(startInfo);
	PROCESS_INFORMATION processInfo = {};
	fflush(nullptr);
	if(lExeLength == 0 || lExeLength >= MAX_PATH * 4 ||
		!CreateProcessW(sExe, sCommandLine.data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startInfo, &processInfo))
	{
		Error("Couldn't start %s: error %lu\n", cInit::GetGameName(aId), GetLastError());
		return;
	}
	CloseHandle(processInfo.hThread);
	CloseHandle(processInfo.hProcess);
}
#endif
