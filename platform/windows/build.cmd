@echo off
rem Builds OpenHPL1 on Windows with Visual Studio (MSVC) and vcpkg.
rem Output: build-windows\overture.exe, with its DLLs, shaders and fonts.
rem The first run builds the vcpkg dependencies, which takes a while.
setlocal
cd /d "%~dp0\..\.."

if not defined VCPKG_ROOT set "VCPKG_ROOT=C:\vcpkg"
if not exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
	echo vcpkg not found in %VCPKG_ROOT%. Set VCPKG_ROOT.
	exit /b 1
)

if not defined VCINSTALLDIR (
	for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set "VSDIR=%%i"
)
if not defined VCINSTALLDIR (
	if not defined VSDIR (
		echo Visual Studio with the C++ workload not found.
		exit /b 1
	)
	call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
)

cmake --preset windows || exit /b 1
cmake --build --preset windows %* || exit /b 1
echo.
echo Built build-windows\overture.exe
