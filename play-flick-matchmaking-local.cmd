@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "TEAM_SIZE=%~1"
if "%TEAM_SIZE%"=="" set "TEAM_SIZE=1"
if not "%TEAM_SIZE%"=="1" if not "%TEAM_SIZE%"=="2" if not "%TEAM_SIZE%"=="3" (
    echo Usage: play-flick-matchmaking-local.cmd [1^|2^|3]
    echo Example: play-flick-matchmaking-local.cmd 2
    exit /b 1
)

set /a TOTAL_PLAYERS=TEAM_SIZE*2
set "PROJECT=%~dp0FLICK.uproject"
set "UNREAL_EDITOR=C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe"
set "MAP=/Engine/Maps/Templates/OpenWorld"

if not exist "%UNREAL_EDITOR%" (
    echo Unreal Editor was not found at:
    echo   %UNREAL_EDITOR%
    exit /b 1
)

if not exist "%PROJECT%" (
    echo FLICK.uproject was not found beside this command.
    exit /b 1
)

echo Launching a local unranked %TEAM_SIZE%v%TEAM_SIZE% lifecycle test...
start "FLICK Matchmaking - Host" "%UNREAL_EDITOR%" "%PROJECT%" "%MAP%?listen?FlickNetworkMatch?FlickPlayersPerTeam=%TEAM_SIZE%?FlickMatchmaking" -game -Multiprocess -nosteam -windowed -ResX=640 -ResY=480 -WinX=0 -WinY=30 -NoSplash -FlickNetworkAutoReady -ExecCmds="t.MaxFPS 45"
timeout /t 3 /nobreak >nul

for /L %%I in (2,1,%TOTAL_PLAYERS%) do (
    set /a COLUMN=(%%I-1)%%3
    set /a ROW=(%%I-1)/3
    set /a POS_X=!COLUMN!*650
    set /a POS_Y=!ROW!*510+30
    start "FLICK Matchmaking - Player %%I" "%UNREAL_EDITOR%" "%PROJECT%" 127.0.0.1 -game -Multiprocess -nosteam -windowed -ResX=640 -ResY=480 -WinX=!POS_X! -WinY=!POS_Y! -NoSplash -FlickNetworkAutoReady -ExecCmds="t.MaxFPS 45"
)

echo.
echo This bypasses Steam discovery but exercises the matchmaking roster, lock,
echo ready confirmation, authoritative match start, and disconnect-forfeit path.
echo The host starts automatically when every local player has confirmed ready.
echo Close one non-host window during play to verify the server-side forfeit result.
