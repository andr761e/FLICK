@echo off
setlocal

set "PROJECT_ROOT=%~dp0"
set "FLICK_PROJECT_PATH=%PROJECT_ROOT%FLICK.uproject"
set "FLICK_UNREAL_EDITOR_CMD=C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
set "FLICK_COORDINATOR_PUBLIC_URL=http://127.0.0.1:8090"
set "FLICK_BACKEND_SERVER_KEY=flick-local-development-key"

if not exist "%FLICK_PROJECT_PATH%" (
    echo FLICK.uproject was not found at %FLICK_PROJECT_PATH%
    exit /b 1
)
if not exist "%FLICK_UNREAL_EDITOR_CMD%" (
    echo UnrealEditor-Cmd.exe was not found at %FLICK_UNREAL_EDITOR_CMD%
    exit /b 1
)

echo Starting the FLICK local matchmaking coordinator on http://127.0.0.1:8090
echo It will allocate ports from 7780 and launch headless Editor server processes.
dotnet run --project "%PROJECT_ROOT%Tools\FlickCoordinator\FlickCoordinator.csproj" --configuration Release --no-launch-profile --urls http://127.0.0.1:8090

