@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\Update-FlickPucks.ps1" %*
exit /b %errorlevel%
