@echo off
rem Runs OpenHPL1 against the Penumbra: Overture game data.
rem Settings, saves and hpl.log: Documents\Penumbra Overture\OvertureRemade\
rem Arguments are passed to the game; set OVERTURE_GAME_DIR to use another copy.
setlocal
set "HERE=%~dp0..\..\"
if not defined OVERTURE_GAME_DIR set "OVERTURE_GAME_DIR=%ProgramFiles(x86)%\Steam\steamapps\common\Penumbra Overture\redist"
rem No ( ) blocks around the path: the ")" of "(x86)" would end them
if exist "%OVERTURE_GAME_DIR%\resources.cfg" goto found
echo Game data not found: "%OVERTURE_GAME_DIR%". Set OVERTURE_GAME_DIR.
exit /b 1
:found
cd /d "%OVERTURE_GAME_DIR%" || exit /b 1
start "" "%HERE%build-windows\openhpl1.exe" %*
