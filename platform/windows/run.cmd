@echo off
rem Runs OpenHPL1 against the Penumbra: Overture game data.
rem Settings, saves and hpl.log: Documents\Penumbra Overture\OvertureRemade\
rem Arguments are passed to the game; set OPENHPL_GAME_DIR to use another copy.
rem Without the data there, openhpl1.exe finds the games itself or opens its chooser.
setlocal
set "HERE=%~dp0..\..\"
if not defined OPENHPL_GAME_DIR set "OPENHPL_GAME_DIR=%ProgramFiles(x86)%\Steam\steamapps\common\Penumbra Overture\redist"
rem No ( ) blocks around the path: the ")" of "(x86)" would end them
if exist "%OPENHPL_GAME_DIR%\resources.cfg" goto found
start "" "%HERE%build-windows\openhpl1.exe" %*
exit /b 0
:found
cd /d "%OPENHPL_GAME_DIR%" || exit /b 1
start "" "%HERE%build-windows\openhpl1.exe" %*
