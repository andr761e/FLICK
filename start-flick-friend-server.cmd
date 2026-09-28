@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Tools\Start-FlickFriendServer.ps1" -HostIp "%~1"
exit /b %errorlevel%
