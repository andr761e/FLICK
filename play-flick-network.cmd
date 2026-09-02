@echo off
setlocal

set "PROJECT=%~dp0FLICK.uproject"
set "UNREAL_EDITOR=C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe"
set "MAP=/Engine/Maps/Templates/OpenWorld"

if not exist "%UNREAL_EDITOR%" (
    echo Unreal Editor was not found at:
    echo   %UNREAL_EDITOR%
    echo Update UNREAL_EDITOR in play-flick-network.cmd if Unreal Engine moves.
    exit /b 1
)

if not exist "%PROJECT%" (
    echo FLICK.uproject was not found beside this command.
    exit /b 1
)

echo Launching two independent FLICK instances...
start "FLICK - Player 1" "%UNREAL_EDITOR%" "%PROJECT%" "%MAP%" -game -Multiprocess -nosteam -windowed -ResX=960 -ResY=720 -WinX=20 -WinY=80 -NoSplash -ExecCmds="t.MaxFPS 60"
start "FLICK - Player 2" "%UNREAL_EDITOR%" "%PROJECT%" "%MAP%" -game -Multiprocess -nosteam -windowed -ResX=960 -ResY=720 -WinX=1000 -WinY=80 -NoSplash -ExecCmds="t.MaxFPS 60"

echo.
echo In Player 1: PLAY, choose a mode, then select HOST LOCAL.
echo In Player 2: PLAY, then select JOIN LOCALHOST.
echo The selected match begins after Player 2 joins.
