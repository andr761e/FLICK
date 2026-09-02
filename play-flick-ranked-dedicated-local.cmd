@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "TEAM_SIZE=%~1"
if "%TEAM_SIZE%"=="" set "TEAM_SIZE=1"
if not "%TEAM_SIZE%"=="1" if not "%TEAM_SIZE%"=="2" if not "%TEAM_SIZE%"=="3" (
    echo Usage: play-flick-ranked-dedicated-local.cmd [1^|2^|3]
    exit /b 1
)

set /a TOTAL_PLAYERS=TEAM_SIZE*2
set "PROJECT_ROOT=%~dp0"
set "PROJECT=%PROJECT_ROOT%FLICK.uproject"
set "SERVER=%PROJECT_ROOT%Binaries\Win64\FLICKServer.exe"
set "UNREAL_EDITOR=C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe"
set "MAP=/Engine/Maps/Templates/OpenWorld?FlickNetworkMatch?FlickMatchmaking?FlickRanked?FlickPlayersPerTeam=%TEAM_SIZE%"

if not exist "%SERVER%" (
    echo FLICKServer.exe was not found. Run build-flick-dedicated-server.cmd first.
    exit /b 1
)
if not exist "%UNREAL_EDITOR%" (
    echo Unreal Editor was not found at:
    echo   %UNREAL_EDITOR%
    exit /b 1
)

echo Launching a headless authority and %TOTAL_PLAYERS% local clients...
start "FLICK Dedicated Server" /min "%SERVER%" "%MAP%" -log -nosteam -port=7777 -QueryPort=27015 -unattended -NoSplash
timeout /t 3 /nobreak >nul

for /L %%I in (1,1,%TOTAL_PLAYERS%) do (
    set /a COLUMN=(%%I-1)%%3
    set /a ROW=(%%I-1)/3
    set /a POS_X=!COLUMN!*650
    set /a POS_Y=!ROW!*510+30
    start "FLICK Ranked - Player %%I" "%UNREAL_EDITOR%" "%PROJECT%" 127.0.0.1 -game -Multiprocess -nosteam -windowed -ResX=640 -ResY=480 -WinX=!POS_X! -WinY=!POS_Y! -NoSplash -FlickNetworkAutoReady -ExecCmds="t.MaxFPS 45"
)

echo.
echo The server verifies local identities, registers one authoritative match,
echo and settles both teams through the server-owned development backend.

