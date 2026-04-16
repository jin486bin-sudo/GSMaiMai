@echo off
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

echo Installing to: %VST3_DST%
if exist "%VST3_DST%" rmdir /s /q "%VST3_DST%"
xcopy "%VST3_SRC%" "%VST3_DST%" /e /i /y /q

echo.
echo Installation complete. Restart your DAW.
pause
