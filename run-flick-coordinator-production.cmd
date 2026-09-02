@echo off
setlocal

set "PROJECT_ROOT=%~dp0"
set "COORDINATOR=%PROJECT_ROOT%Builds\OnlineStack\Coordinator\FlickCoordinator.exe"

if not exist "%COORDINATOR%" (
    echo Published coordinator not found. Run package-flick-online-stack.cmd first.
    exit /b 1
)
if "%FLICK_STEAM_WEB_API_KEY%"=="" (
    echo FLICK_STEAM_WEB_API_KEY is required.
    exit /b 1
)
if "%FLICK_SERVER_EXECUTABLE%"=="" (
    echo FLICK_SERVER_EXECUTABLE is required.
    exit /b 1
)
if "%FLICK_SERVER_PUBLIC_HOST%"=="" (
    echo FLICK_SERVER_PUBLIC_HOST is required.
    exit /b 1
)

set "FLICK_COORDINATOR_ENVIRONMENT=Production"
if "%ASPNETCORE_URLS%"=="" set "ASPNETCORE_URLS=http://0.0.0.0:8090"

echo Starting the FLICK production coordinator on %ASPNETCORE_URLS%...
"%COORDINATOR%"
exit /b %errorlevel%
