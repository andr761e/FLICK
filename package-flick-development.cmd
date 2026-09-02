@echo off
setlocal

set "PROJECT_ROOT=%~dp0"
set "PROJECT=%PROJECT_ROOT%FLICK.uproject"
set "RUN_UAT=C:\Program Files\Epic Games\UE_5.6\Engine\Build\BatchFiles\RunUAT.bat"
set "OUTPUT=%PROJECT_ROOT%Builds\Development"
set "STEAM_APP_ID=%PROJECT_ROOT%Build\Steam\steam_appid.txt"

if not exist "%RUN_UAT%" (
    echo Unreal Automation Tool was not found at:
    echo   %RUN_UAT%
    echo Update RUN_UAT in package-flick-development.cmd if Unreal Engine moves.
    exit /b 1
)

if not exist "%PROJECT%" (
    echo FLICK.uproject was not found beside this command.
    exit /b 1
)

if not exist "%STEAM_APP_ID%" (
    echo Steam test App ID file was not found at:
    echo   %STEAM_APP_ID%
    exit /b 1
)

echo Packaging FLICK Development for Windows...
echo Output: %OUTPUT%
echo.

call "%RUN_UAT%" BuildCookRun ^
    -project="%PROJECT%" ^
    -noP4 ^
    -platform=Win64 ^
    -clientconfig=Development ^
    -build ^
    -cook ^
    -map=/Engine/Maps/Templates/OpenWorld ^
    -stage ^
    -pak ^
    -prereqs ^
    -archive ^
    -archivedirectory="%OUTPUT%" ^
    -utf8output

if errorlevel 1 (
    echo.
    echo FLICK packaging failed. Review the Unreal Automation Tool errors above.
    exit /b 1
)

set "PACKAGE_ROOT=%OUTPUT%\Windows"
set "GAME_BIN=%PACKAGE_ROOT%\FLICK\Binaries\Win64"

if not exist "%PACKAGE_ROOT%\FLICK.exe" (
    echo.
    echo Packaging completed, but the expected executable was not found:
    echo   %PACKAGE_ROOT%\FLICK.exe
    exit /b 1
)

if not exist "%GAME_BIN%\FLICK.exe" (
    echo.
    echo Packaging completed, but the expected game binary was not found:
    echo   %GAME_BIN%\FLICK.exe
    exit /b 1
)

copy /Y "%STEAM_APP_ID%" "%PACKAGE_ROOT%\steam_appid.txt" >nul
copy /Y "%STEAM_APP_ID%" "%GAME_BIN%\steam_appid.txt" >nul

echo.
echo FLICK Development package is ready:
echo   %PACKAGE_ROOT%
echo.
echo Zip the entire Windows folder when sharing the build.
exit /b 0
