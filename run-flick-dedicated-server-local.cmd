@echo off
setlocal

set "PROJECT_ROOT=%~dp0"
set "SERVER=%PROJECT_ROOT%Binaries\Win64\FLICKServer.exe"
set "TEAM_SIZE=%~1"
if "%TEAM_SIZE%"=="" set "TEAM_SIZE=1"
set "MAP=/Engine/Maps/Templates/OpenWorld?FlickNetworkMatch?FlickMatchmaking?FlickRanked?FlickPlayersPerTeam=%TEAM_SIZE%"

if not exist "%SERVER%" (
    echo FLICKServer.exe was not found. Run build-flick-dedicated-server.cmd first.
    exit /b 1
)

echo Starting the local ranked-development authority on port 7777...
echo This uses the server-owned LocalDevelopment ranking provider and does not require Steam.
"%SERVER%" "%MAP%" -log -nosteam -port=7777 -QueryPort=27015 -unattended -NoSplash

