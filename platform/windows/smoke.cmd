@echo off
rem Smoke test, hidden and silent: loads every map (and the test maps), saves and reloads, then
rem exits with 1 if anything failed. Uses its own settings and saves in
rem build-windows\smoke-profile\, so yours are untouched.
rem Pass a filter to limit the maps, e.g. smoke.cmd level01
setlocal
set "HERE=%~dp0..\..\"
if not defined OPENHPL_GAME_DIR set "OPENHPL_GAME_DIR=%ProgramFiles(x86)%\Steam\steamapps\common\Penumbra Overture\redist"
set "OPENHPL_SMOKE=1"
rem Hidden window and OpenAL Soft's null backend: nothing is shown or heard
set "OPENHPL_HIDDEN=1"
set "ALSOFT_DRIVERS=null"
if not "%~1"=="" set "OPENHPL_SMOKE=%~1"
set "OPENHPL_PROFILE_ROOT=%HERE%build-windows\smoke-profile"
if not exist "%OPENHPL_PROFILE_ROOT%" mkdir "%OPENHPL_PROFILE_ROOT%"
cd /d "%OPENHPL_GAME_DIR%" || exit /b 1
start "" /wait "%HERE%build-windows\openhpl1.exe"
set "RESULT=%ERRORLEVEL%"
echo Smoke test exit code %RESULT%; log in %OPENHPL_PROFILE_ROOT%
exit /b %RESULT%
