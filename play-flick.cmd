@echo off
setlocal

set "PROJECT=%~dp0FLICK.uproject"
set "UNREAL_BUILD=C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\Build.bat"
set "UNREAL_EDITOR=C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe"

if not exist "%UNREAL_BUILD%" (
    echo Unreal Build Tool was not found at:
    echo   %UNREAL_BUILD%
    echo Update UNREAL_BUILD in play-flick.cmd if Unreal Engine moves.
    exit /b 1
)

if not exist "%UNREAL_EDITOR%" (
    echo Unreal Editor was not found at:
    echo   %UNREAL_EDITOR%
    echo Update UNREAL_EDITOR in play-flick.cmd if Unreal Engine moves.
    exit /b 1
)

if not exist "%PROJECT%" (
    echo FLICK.uproject was not found beside this command.
    exit /b 1
)

echo Building the latest FLICKEditor...
call "%UNREAL_BUILD%" FLICKEditor Win64 Development "%PROJECT%" -waitmutex -NoHotReload
if errorlevel 1 (
    echo.
    echo FLICKEditor build failed. The game was not launched.
    exit /b 1
)

echo.
echo Launching FLICK in a standalone game window...
if "%~1"=="" (
    start "FLICK" "%UNREAL_EDITOR%" "%PROJECT%" -game -windowed -ResX=1600 -ResY=900 -log
) else (
    start "FLICK" "%UNREAL_EDITOR%" "%PROJECT%" -game -log %*
)
