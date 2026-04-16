@echo off
setlocal
title GSMaiMai Uninstaller
echo ======================================
echo   GSMaiMai VST3 Plugin Uninstaller
echo ======================================
echo.

set "VST3_DST=%COMMONPROGRAMFILES%\VST3\GSMaiMai.vst3"

if exist "%VST3_DST%" (
    rmdir /s /q "%VST3_DST%"
    echo GSMaiMai has been removed from: %VST3_DST%
) else (
    echo GSMaiMai is not installed.
)

echo.
echo Uninstall complete. Restart your DAW.
pause
