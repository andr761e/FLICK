@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\Test-FlickCoordinator.ps1"
if errorlevel 1 exit /b %errorlevel%
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\Test-FlickCoordinatorLifecycle.ps1"
if errorlevel 1 exit /b %errorlevel%
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\Test-FlickCoordinatorMatching.ps1"
exit /b %errorlevel%
