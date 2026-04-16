@echo off
setlocal
title GSMaiMai Windows Build
echo ======================================
echo   GSMaiMai VST3 Plugin - Windows Build
echo ======================================
echo.

:: Check prerequisites
where cmake >nul 2>&1 || (
    echo [ERROR] CMake not found. Install from https://cmake.org/download/
    pause & exit /b 1
)
where git >nul 2>&1 || (
    echo [ERROR] Git not found. Install from https://git-scm.com/
    pause & exit /b 1
)

cd /d "%~dp0"

:: Clone JUCE if not present
if not exist "JUCE" (
    echo [1/4] Downloading JUCE...
    git clone --depth 1 --branch 8.0.4 https://github.com/juce-framework/JUCE.git
) else (
    echo [1/4] JUCE already present, skipping download.
)

:: Configure
echo [2/4] Configuring build...
cmake -B build_win -G "Visual Studio 17 2022" -A x64
if errorlevel 1 (
    echo Trying Visual Studio 16 2019...
    cmake -B build_win -G "Visual Studio 16 2019" -A x64
)
if errorlevel 1 (
    echo [ERROR] CMake configure failed. Make sure Visual Studio is installed.
    echo   Free: https://visualstudio.microsoft.com/downloads/ (Community Edition)
    echo   Workload: "Desktop development with C++"
    pause & exit /b 1
)

:: Build Release
echo [3/4] Building Release...
cmake --build build_win --config Release
if errorlevel 1 (
    echo [ERROR] Build failed.
    pause & exit /b 1
)

:: Copy to common VST3 folder
echo [4/4] Installing VST3...
set "VST3_SRC=%~dp0build_win\GSMaiMai_artefacts\Release\VST3\GSMaiMai.vst3"
set "VST3_DST=%COMMONPROGRAMFILES%\VST3\GSMaiMai.vst3"

if exist "%VST3_DST%" rmdir /s /q "%VST3_DST%"
xcopy "%VST3_SRC%" "%VST3_DST%" /e /i /y /q

echo.
echo ======================================
echo   Build complete!
echo   Installed to: %VST3_DST%
echo   Restart your DAW to load the plugin.
echo ======================================
pause
