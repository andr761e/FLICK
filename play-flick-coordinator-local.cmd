@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "TEAM_SIZE=%~1"
if "%TEAM_SIZE%"=="" set "TEAM_SIZE=1"
if not "%TEAM_SIZE%"=="1" if not "%TEAM_SIZE%"=="2" if not "%TEAM_SIZE%"=="3" (
    echo Usage: play-flick-coordinator-local.cmd [1^|2^|3] [casual^|ranked] [0^|1^|2]
    exit /b 1
)
set "QUEUE_TYPE=%~2"
if "%QUEUE_TYPE%"=="" set "QUEUE_TYPE=casual"
set "VARIANT=%~3"
if "%VARIANT%"=="" set "VARIANT=0"
set "RANKED_ARG="
if /I "%QUEUE_TYPE%"=="ranked" set "RANKED_ARG=-FlickRanked"

set /a TOTAL_PLAYERS=TEAM_SIZE*2
set "PROJECT_ROOT=%~dp0"
set "PROJECT=%PROJECT_ROOT%FLICK.uproject"
set "UNREAL_EDITOR=C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe"
set "COORDINATOR_URL=http://127.0.0.1:8090"

if not exist "%UNREAL_EDITOR%" (
    echo Unreal Editor was not found at %UNREAL_EDITOR%
    exit /b 1
)

start "FLICK Local Coordinator" /min cmd /c ""%PROJECT_ROOT%run-flick-local-coordinator.cmd""
timeout /t 3 /nobreak >nul

echo Launching %TOTAL_PLAYERS% independent clients into the natural coordinator queue...
for /L %%I in (1,1,%TOTAL_PLAYERS%) do (
    set /a COLUMN=(%%I-1)%%3
    set /a ROW=(%%I-1)/3
    set /a POS_X=!COLUMN!*650
    set /a POS_Y=!ROW!*510+30
    start "FLICK Coordinator - Player %%I" "%UNREAL_EDITOR%" "%PROJECT%" /Engine/Maps/Templates/OpenWorld -game -Multiprocess -nosteam -windowed -ResX=640 -ResY=480 -WinX=!POS_X! -WinY=!POS_Y! -NoSplash -FlickCoordinatorUrl=%COORDINATOR_URL% -FlickCoordinatorAutoQueue -FlickCoordinatorTeamSize=%TEAM_SIZE% -FlickLocalAccountId=LocalPlayer%%I -FlickVariant=%VARIANT% %RANKED_ARG% -FlickNetworkAutoReady -ExecCmds="t.MaxFPS 45"
)

echo.
echo Each client queues independently. The coordinator forms two teams, launches a
echo headless authority, issues reservations, and moves the clients into its lobby.

