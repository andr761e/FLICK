@echo off
setlocal

set "PROJECT_ROOT=%~dp0"
set "PROJECT=%PROJECT_ROOT%FLICK.uproject"
set "ENGINE_ROOT=%FLICK_UNREAL_ENGINE_ROOT%"
if not "%~1"=="" set "ENGINE_ROOT=%~1"
if "%ENGINE_ROOT%"=="" set "ENGINE_ROOT=C:\Program Files\Epic Games\UE_5.6"
set "RUN_UAT=%ENGINE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat"
set "OUTPUT=%PROJECT_ROOT%Builds\DedicatedServer"

if not exist "%RUN_UAT%" (
    echo Unreal Automation Tool was not found at:
    echo   %RUN_UAT%
    exit /b 1
)

echo Packaging the FLICK Win64 Development dedicated server...
call "%RUN_UAT%" BuildCookRun ^
    -project="%PROJECT%" ^
    -noP4 ^
    -server ^
    -noclient ^
    -serverplatform=Win64 ^
    -serverconfig=Development ^
    -build ^
    -cook ^
    -map=/Engine/Maps/Templates/OpenWorld ^
    -stage ^
    -pak ^
    -archive ^
    -archivedirectory="%OUTPUT%" ^
    -utf8output

if errorlevel 1 (
    echo.
    echo Dedicated-server packaging failed.
    echo Epic Launcher engine installs commonly omit dedicated-server target libraries.
    echo Set FLICK_UNREAL_ENGINE_ROOT to a source-built Unreal Engine 5.6 checkout,
    echo or pass the engine root as the first argument.
    exit /b 1
)

echo.
echo Dedicated server package ready:
echo   %OUTPUT%
exit /b 0
