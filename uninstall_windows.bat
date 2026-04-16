@echo off
>nul 2>&1 net session || (
    powershell -Command "Start-Process -Verb RunAs -FilePath '%~f0'"
    exit /b
)

setlocal
title GSMaiMai Uninstaller
echo ======================================
echo   GSMaiMai VST3 Plugin Uninstaller
echo ======================================
echo.

set "VST3_DST=%COMMONPROGRAMFILES%\VST3\GSMaiMai.vst3"

if exist "%VST3_DST%" (
    rmdir /s /q "%VST3_DST%"
    if exist "%VST3_DST%" (
        echo [ERROR] Could not remove %VST3_DST%
    ) else (
        echo Removed: %VST3_DST%
    )
) else (
    echo GSMaiMai is not installed at %VST3_DST%
)

echo.
pause
