@echo off
rem Packages an existing build-windows\ build (run build.cmd first) as
rem build-windows\openhpl1-<version>-windows-x64.zip: the executable, its
rem DLLs and the Visual C++ runtime (app-local, so no redistributable needs
rem installing), shaders, fonts, HUD images and licences.
rem Usage: package.cmd <version>
setlocal
cd /d "%~dp0\..\.."
if "%~1"=="" (
	echo Usage: package.cmd ^<version^>
	exit /b 1
)
set "NAME=openhpl1-%~1-windows-x64"
set "BUILD=build-windows"
set "DIST=%BUILD%\%NAME%"
if not exist "%BUILD%\openhpl1.exe" (
	echo %BUILD%\openhpl1.exe not found. Run build.cmd first.
	exit /b 1
)

if not defined VCToolsRedistDir (
	for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
)
if not defined VCToolsRedistDir (
	if not defined VSDIR (
		echo Visual Studio with the C++ workload not found.
		exit /b 1
	)
	call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
)
set "CRT="
for /d %%d in ("%VCToolsRedistDir%x64\Microsoft.VC*.CRT") do set "CRT=%%d"
rem No ( ) block around the path: the ")" of "(x86)" would end it
if defined CRT goto crt_found
echo Visual C++ runtime not found in %VCToolsRedistDir%x64.
exit /b 1
:crt_found

if exist "%DIST%" rmdir /s /q "%DIST%"
if exist "%BUILD%\%NAME%.zip" del "%BUILD%\%NAME%.zip"
mkdir "%DIST%\licenses" || exit /b 1
copy /y "%BUILD%\openhpl1.exe" "%DIST%\" >nul || exit /b 1
copy /y "%BUILD%\*.dll" "%DIST%\" >nul || exit /b 1
copy /y "%CRT%\*.dll" "%DIST%\" >nul || exit /b 1
for %%d in (shaders fonts hud) do (
	xcopy /e /i /q /y "%BUILD%\%%d" "%DIST%\%%d" >nul || exit /b 1
)
for %%f in (COPYING README.md CHANGES.md) do copy /y "%%f" "%DIST%\" >nul || exit /b 1
for %%d in (angelscript glad imgui imgui_glad newton oalwrapper smaa tinyxml) do (
	if exist "vendor\%%d\LICENSE" copy /y "vendor\%%d\LICENSE" "%DIST%\licenses\%%d.txt" >nul
	if exist "vendor\%%d\LICENSE.txt" copy /y "vendor\%%d\LICENSE.txt" "%DIST%\licenses\%%d.txt" >nul
)
for %%p in (sdl3 sdl3-image openal-soft libogg libvorbis freetype libpng libjpeg-turbo zlib fmt) do (
	if exist "%BUILD%\vcpkg_installed\x64-windows\share\%%p\copyright" copy /y "%BUILD%\vcpkg_installed\x64-windows\share\%%p\copyright" "%DIST%\licenses\%%p.txt" >nul
)
(
	echo OpenHPL1 is GPLv3 ^(..\COPYING^). The fonts' licences are in ..\fonts\ and the
	echo HUD images' terms in ..\hud\README.md. The other DLLs come from vcpkg
	echo ^(https://github.com/microsoft/vcpkg^); their licences are in this folder.
	echo OpenAL32.dll is OpenAL Soft ^(LGPL-2.1, https://github.com/kcat/openal-soft^)
	echo and can be replaced by any compatible build. The msvcp140/vcruntime140
	echo DLLs are the Microsoft Visual C++ runtime, redistributed with the program.
) > "%DIST%\licenses\README.txt"

tar -a -c -f "%BUILD%\%NAME%.zip" -C "%BUILD%" "%NAME%" || exit /b 1
echo Packaged %BUILD%\%NAME%.zip
