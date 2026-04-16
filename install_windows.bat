@echo off
:: Self-elevate to admin (VST3 system folder requires it)
>nul 2>&1 net session || (
    echo Requesting administrator privileges...
    powershell -Command "Start-Process -Verb RunAs -FilePath '%~f0'"
    exit /b
)

setlocal
title GSMaiMai Installer
echo ======================================
echo   GSMaiMai VST3 Plugin Installer
echo ======================================
echo.

set "VST3_SRC=%~dp0GSMaiMai.vst3"
set "VST3_DST=%COMMONPROGRAMFILES%\VST3\GSMaiMai.vst3"

if not exist "%VST3_SRC%" (
    echo [ERROR] GSMaiMai.vst3 not found next to this script.
    echo Run build_windows.bat first, or place GSMaiMai.vst3 here.
    pause & exit /b 1
)

echo Removing previous version if present...
if exist "%VST3_DST%" rmdir /s /q "%VST3_DST%"

echo Installing to: %VST3_DST%
if not exist "%COMMONPROGRAMFILES%\VST3" mkdir "%COMMONPROGRAMFILES%\VST3"
xcopy "%VST3_SRC%" "%VST3_DST%\" /e /i /y /q
if errorlevel 1 (
    echo.
    echo [ERROR] Copy failed. Try right-click this file and "Run as administrator".
    pause & exit /b 1
)

:: Verify
if exist "%VST3_DST%\Contents\x86_64-win\GSMaiMai.vst3" (
    echo.
    echo ======================================
    echo   SUCCESS - installed.
    echo   Installed file:
    dir /tc "%VST3_DST%\Contents\x86_64-win\GSMaiMai.vst3" | findstr "GSMaiMai"
    echo ======================================
    echo.
    echo Now in your DAW:
    echo   - Fully quit and restart
    echo   - Rescan plugins (Cubase: Studio ^> VST Plug-in Manager ^> Update/Reset)
    echo   - Ableton: Preferences ^> Plug-Ins ^> Rescan
) else (
    echo [ERROR] Installation verification failed.
)

echo.
pause
