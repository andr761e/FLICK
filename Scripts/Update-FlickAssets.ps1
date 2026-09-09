[CmdletBinding()]
param(
    [string]$UnrealEditor = $env:FLICK_UNREAL_EDITOR
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot 'FLICK.uproject'
$importScript = Join-Path $projectRoot 'Tools\Update-FlickAssets.py'

if (-not $UnrealEditor) {
    $defaultEditor = 'C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    if (Test-Path -LiteralPath $defaultEditor) {
        $UnrealEditor = $defaultEditor
    }
}

if (-not $UnrealEditor -or -not (Test-Path -LiteralPath $UnrealEditor)) {
    throw @'
UnrealEditor-Cmd.exe was not found. Set FLICK_UNREAL_EDITOR to its full path, for example:
  $env:FLICK_UNREAL_EDITOR = 'C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
'@
}

if (Get-Process -Name UnrealEditor -ErrorAction SilentlyContinue) {
    throw 'Close Unreal Editor before updating assets so it cannot overwrite the imported .uasset files.'
}

$requiredFiles = @(
    'AssetDevelopment\Pucks\HighDetail\exports\Standard.fbx',
    'AssetDevelopment\Pucks\HighDetail\exports\Toppler.fbx',
    'AssetDevelopment\Pucks\HighDetail\exports\Bouncer.fbx',
    'AssetDevelopment\Pucks\HighDetail\exports\Compact.fbx',
    'AssetDevelopment\Pucks\HighDetail\exports\Blocker.fbx',
    'AssetDevelopment\Pucks\HighDetail\exports\Slider.fbx',
    'AssetDevelopment\Pucks\HighDetail\exports\Grippy.fbx',
    'AssetDevelopment\Pucks\HighDetail\exports\Striker.fbx',
    'AssetDevelopment\Pucks\HighDetail\exports\Heavy.fbx',
    'AssetDevelopment\Arena\exports\SM_TestArena_Static.fbx',
    'AssetDevelopment\Arena\exports\SM_TestArena_Divider.fbx',
    'AssetDevelopment\Arena\exports\SM_TestArena_DividerSocket.fbx',
    'AssetDevelopment\Arena\exports\SM_TestArena_SwitchHousing.fbx',
    'AssetDevelopment\Arena\exports\SM_TestArena_SwitchDot.fbx',
    'AssetDevelopment\Arena\exports\SM_TestArena_SignalTrace.fbx',
    'AssetDevelopment\Stadium\exports\SM_TestStadium_Structure.fbx',
	'AssetDevelopment\Stadium\exports\SM_TestStadium_Lights.fbx',
	'AssetDevelopment\BOB Arena\exports\SM_BobArena_HighDetail.fbx'
)

$missingFiles = @($requiredFiles | Where-Object {
    -not (Test-Path -LiteralPath (Join-Path $projectRoot $_))
})
if ($missingFiles.Count -gt 0) {
    throw "Asset update stopped because required FBX files are missing:`n  $($missingFiles -join "`n  ")"
}

Write-Host 'Updating FLICK puck, arena, stadium, and BOB arena assets...'
& $UnrealEditor $projectFile `
    -run=pythonscript `
    "-script=$importScript" `
    -unattended `
    -nop4 `
    -nosplash `
    -NoSound

if ($LASTEXITCODE -ne 0) {
    throw "Unreal asset update failed with exit code $LASTEXITCODE. See Saved\Logs\FLICK.log."
}

$logFile = Join-Path $projectRoot 'Saved\Logs\FLICK.log'
if (-not (Test-Path -LiteralPath $logFile) -or
    -not (Select-String -LiteralPath $logFile -SimpleMatch 'FLICK_ASSET_UPDATE_COMPLETE' -Quiet)) {
    throw 'Unreal exited without confirming completion. See Saved\Logs\FLICK.log.'
}

Write-Host 'Asset update complete. Review the meshes in Unreal, then commit the updated Content/TestArena and Content/BOB .uasset files.'
