@echo off
setlocal

set "PROJECT_ROOT=%~dp0"
set "PROJECT=%PROJECT_ROOT%FLICK.uproject"
set "ENGINE_ROOT=%FLICK_UNREAL_ENGINE_ROOT%"
if not "%~1"=="" set "ENGINE_ROOT=%~1"
if "%ENGINE_ROOT%"=="" set "ENGINE_ROOT=C:\Program Files\Epic Games\UE_5.6"
set "BUILD_BAT=%ENGINE_ROOT%\Engine\Build\BatchFiles\Build.bat"

if not exist "%BUILD_BAT%" (
    echo Unreal Build Tool was not found at:
    echo   %BUILD_BAT%
    exit /b 1
)

echo Building the FLICK Win64 Development dedicated server target...
call "%BUILD_BAT%" FLICKServer Win64 Development "%PROJECT%" -waitmutex -NoHotReload
if errorlevel 1 (
    echo.
    echo The dedicated-server build failed.
    echo Epic Launcher engine installs may not include the server target libraries.
    echo Set FLICK_UNREAL_ENGINE_ROOT to a source-built Unreal Engine 5.6 checkout,
    echo or pass the engine root as the first argument.
    exit /b 1
)

echo.
echo Dedicated server build ready:
echo   %PROJECT_ROOT%Binaries\Win64\FLICKServer.exe
exit /b 0
