@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\Test-FlickCoordinatorLifecycle.ps1"
exit /b %errorlevel%
