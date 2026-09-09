@echo off
setlocal
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0Scripts\Update-FlickAssets.ps1" %*
exit /b %errorlevel%
