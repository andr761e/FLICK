@echo off
setlocal
if "%~2"=="" (
    echo Usage: join-flick-friend-test.cmd VPN_OR_LAN_HOST_IP UNIQUE_PLAYER_ID
    echo Example: join-flick-friend-test.cmd 100.101.102.103 FriendPlayer
    exit /b 1
)
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Join-FlickFriendTest.ps1" -HostIp "%~1" -PlayerId "%~2"
exit /b %errorlevel%
