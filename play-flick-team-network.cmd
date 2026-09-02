@echo off
setlocal EnableExtensions EnableDelayedExpansion

set "TEAM_SIZE=%~1"
if "%TEAM_SIZE%"=="" set "TEAM_SIZE=2"
if not "%TEAM_SIZE%"=="2" if not "%TEAM_SIZE%"=="3" (
    echo Usage: play-flick-team-network.cmd [2^|3]
    echo Example: play-flick-team-network.cmd 3
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

echo Launching a local %TEAM_SIZE%v%TEAM_SIZE% lobby with %TOTAL_PLAYERS% independent clients...
start "FLICK - Host" "%UNREAL_EDITOR%" "%PROJECT%" "%MAP%?listen?FlickNetworkMatch?FlickPlayersPerTeam=%TEAM_SIZE%" -game -Multiprocess -nosteam -windowed -ResX=640 -ResY=480 -WinX=0 -WinY=30 -NoSplash -ExecCmds="t.MaxFPS 45"
timeout /t 3 /nobreak >nul

for /L %%I in (2,1,%TOTAL_PLAYERS%) do (
    set /a COLUMN=(%%I-1)%%3
    set /a ROW=(%%I-1)/3
    set /a POS_X=!COLUMN!*650
    set /a POS_Y=!ROW!*510+30
    start "FLICK - Player %%I" "%UNREAL_EDITOR%" "%PROJECT%" 127.0.0.1 -game -Multiprocess -nosteam -windowed -ResX=640 -ResY=480 -WinX=!POS_X! -WinY=!POS_Y! -NoSplash -ExecCmds="t.MaxFPS 45"
)

echo.
echo All clients join the team lobby without starting the match.
echo Ready every client, then press START in the host window.
echo Use  play-flick-team-network.cmd 3  for a six-client trios test.
