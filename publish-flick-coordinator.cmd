@echo off
setlocal

set "PROJECT_ROOT=%~dp0"
set "PROJECT=%PROJECT_ROOT%Tools\FlickCoordinator\FlickCoordinator.csproj"
set "OUTPUT=%PROJECT_ROOT%Builds\OnlineStack\Coordinator"

echo Publishing the FLICK coordinator for Windows x64...
dotnet publish "%PROJECT%" --configuration Release --runtime win-x64 --self-contained false --output "%OUTPUT%"
if errorlevel 1 exit /b 1

echo.
echo Coordinator publish ready:
echo   %OUTPUT%\FlickCoordinator.exe
exit /b 0
