@echo off
setlocal

set "PROJECT_ROOT=%~dp0"
set "SERVER_SOURCE=%PROJECT_ROOT%Builds\DedicatedServer"
set "SERVER_OUTPUT=%PROJECT_ROOT%Builds\OnlineStack\DedicatedServer"

call "%PROJECT_ROOT%package-flick-dedicated-server.cmd" %*
if errorlevel 1 exit /b 1

call "%PROJECT_ROOT%publish-flick-coordinator.cmd"
if errorlevel 1 exit /b 1

if exist "%SERVER_OUTPUT%" rmdir /s /q "%SERVER_OUTPUT%"
xcopy "%SERVER_SOURCE%\*" "%SERVER_OUTPUT%\" /e /i /q /y >nul
if errorlevel 1 exit /b 1

copy /y "%PROJECT_ROOT%Deploy\production.env.example" "%PROJECT_ROOT%Builds\OnlineStack\production.env.example" >nul

echo.
echo FLICK online stack ready:
echo   %PROJECT_ROOT%Builds\OnlineStack
echo.
echo Configure production.env.example before deploying. Secrets must not be committed.
exit /b 0
