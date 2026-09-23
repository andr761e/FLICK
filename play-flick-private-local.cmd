@echo off
setlocal EnableExtensions

set "PROJECT=%~dp0FLICK.uproject"
set "ENGINE_ROOT=%FLICK_UNREAL_ENGINE_ROOT%"
if "%ENGINE_ROOT%"=="" set "ENGINE_ROOT=C:\Program Files\Epic Games\UE_5.8"
set "UNREAL_EDITOR=%ENGINE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe"
set "MAP=/Engine/Maps/Templates/OpenWorld"
set "PARTY_ID=FLICK-LOCAL-PRIVATE-PARTY"

if not exist "%UNREAL_EDITOR%" (
    echo Unreal Editor was not found at:
    echo   %UNREAL_EDITOR%
    exit /b 1
)

echo Launching a local two-player private party without Steam...
start "FLICK - Party Leader" "%UNREAL_EDITOR%" "%PROJECT%" "%MAP%?listen?FlickParty?FlickPartyId=%PARTY_ID%?FlickPartySlot=0?FlickPartySize=2?FlickPartyLeader=1" -game -Multiprocess -nosteam -windowed -ResX=960 -ResY=720 -WinX=20 -WinY=80 -NoSplash -ExecCmds="t.MaxFPS 60"
timeout /t 3 /nobreak >nul
start "FLICK - Party Guest" "%UNREAL_EDITOR%" "%PROJECT%" "127.0.0.1?FlickPartyId=%PARTY_ID%?FlickPartySlot=1?FlickPartySize=2?FlickPartyLeader=0" -game -Multiprocess -nosteam -windowed -ResX=960 -ResY=720 -WinX=1000 -WinY=80 -NoSplash -ExecCmds="t.MaxFPS 60"

echo.
echo In the leader window: PLAY, then PRIVATE MATCH.
echo Only the leader configures and launches the match. Then both players choose a team or spectate.
echo Confirm a class after joining a team; empty seats become bots when the match starts.
echo This validates gameplay transport and replication. Steam invites still require two Steam accounts.
