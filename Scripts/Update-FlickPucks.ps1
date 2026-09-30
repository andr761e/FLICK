[CmdletBinding()]
param(
    [string]$UnrealEditor = $env:FLICK_UNREAL_EDITOR
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if (Get-Process -Name UnrealEditor -ErrorAction SilentlyContinue) {
    throw 'Close Unreal Editor before updating pucks.'
}
if (-not $UnrealEditor) {
    $UnrealEditor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
}
foreach ($executable in @($UnrealEditor)) {
    if (-not $executable -or -not (Test-Path -LiteralPath $executable)) {
        throw 'UnrealEditor-Cmd.exe is missing. Set FLICK_UNREAL_EDITOR.'
    }
}
foreach ($name in @('Standard','Toppler','Bouncer','Compact','Blocker','Slider','Grippy','Striker','Heavy')) {
    if (-not (Test-Path -LiteralPath (Join-Path $projectRoot "AssetDevelopment\Pucks\ClassicBlue\exports\$name.fbx"))) {
        throw "Missing base puck: $name.fbx"
    }
}
$logFile = Join-Path $projectRoot 'Saved\Logs\FlickPuckUpdate.log'
Write-Host 'Importing nine base puck meshes and Orange cosmetic materials...'
& $UnrealEditor (Join-Path $projectRoot 'FLICK.uproject') -run=pythonscript `
    "-script=$(Join-Path $projectRoot 'Tools\Update-FlickPucks.py')" `
    -unattended -nop4 -nosplash -NullRHI "-abslog=$logFile"
if ($LASTEXITCODE -ne 0 -or -not (Select-String -LiteralPath $logFile -SimpleMatch 'FLICK_PUCK_UPDATE_COMPLETE' -Quiet)) {
    throw "Puck import failed. See $logFile"
}
Write-Host 'Pucks updated: nine shared meshes with Classic Blue and Classic Orange skins. No player variants or arena imports.'
