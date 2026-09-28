@echo off
setlocal
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Join-FlickFriendTest.ps1" -ManualSearch
exit /b %errorlevel%
