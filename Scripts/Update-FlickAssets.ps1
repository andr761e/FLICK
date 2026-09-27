[CmdletBinding()]
param(
    [string]$UnrealEditor = $env:FLICK_UNREAL_EDITOR,
    [switch]$Force,
    [switch]$ListOnly
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$projectFile = Join-Path $projectRoot 'FLICK.uproject'
$importScript = Join-Path $projectRoot 'Tools\Update-FlickAssets.py'

if (-not $UnrealEditor) {
    $defaultEditor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
    if (Test-Path -LiteralPath $defaultEditor) {
        $UnrealEditor = $defaultEditor
    }
}

if (-not $UnrealEditor -or -not (Test-Path -LiteralPath $UnrealEditor)) {
    throw @'
UnrealEditor-Cmd.exe was not found. Set FLICK_UNREAL_EDITOR to its full path, for example:
  $env:FLICK_UNREAL_EDITOR = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
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
    'AssetDevelopment\ClassicArena\exports\SM_ClassicArena_Premium.fbx',
    'AssetDevelopment\Arena\exports\SM_TestArena_Divider.fbx',
    'AssetDevelopment\Arena\exports\SM_TestArena_DividerSocket.fbx',
    'AssetDevelopment\Arena\exports\SM_TestArena_SwitchHousing.fbx',
    'AssetDevelopment\Arena\exports\SM_TestArena_SwitchDot.fbx',
    'AssetDevelopment\Arena\exports\SM_TestArena_SignalTrace.fbx',
    'AssetDevelopment\Stadium\exports\SM_TestStadium_Structure.fbx',
	'AssetDevelopment\Stadium\exports\SM_TestStadium_Lights.fbx',
	'AssetDevelopment\BOB Arena\exports\SM_BobArena_HighDetail.fbx',
	'AssetDevelopment\BOBStadium\exports\SM_BobStadium_Structure.fbx',
	'AssetDevelopment\BOBStadium\exports\SM_BobStadium_Lights.fbx'
)

$missingFiles = @($requiredFiles | Where-Object {
    -not (Test-Path -LiteralPath (Join-Path $projectRoot $_))
})
if ($missingFiles.Count -gt 0) {
    throw "Asset update stopped because required FBX files are missing:`n  $($missingFiles -join "`n  ")"
}

$stampFile = Join-Path $projectRoot 'Saved\FlickAssetSourceHashes.json'
$sourcePrefixes = @(
    'AssetDevelopment/Pucks/HighDetail/',
    'AssetDevelopment/Pucks/PlayerIdentity/P1/Blue/',
    'AssetDevelopment/Pucks/PlayerIdentity/P2/Blue/',
    'AssetDevelopment/Pucks/PlayerIdentity/P3/Blue/',
    'AssetDevelopment/Arena/',
    'AssetDevelopment/ClassicArena/',
    'AssetDevelopment/Stadium/',
    'AssetDevelopment/BOB Arena/',
    'AssetDevelopment/BOBStadium/'
)
$sourceManifests = @(
    'AssetDevelopment/Arena/dimensions.json',
    'AssetDevelopment/Pucks/HighDetail/manifests/archetype_exports.json',
    'AssetDevelopment/Pucks/PlayerIdentity/P1/Blue/manifest.json',
    'AssetDevelopment/Pucks/PlayerIdentity/P2/Blue/manifest.json',
    'AssetDevelopment/Pucks/PlayerIdentity/P3/Blue/manifest.json',
    'AssetDevelopment/Stadium/manifest.json',
    'AssetDevelopment/BOBStadium/manifest.json'
)
$sources = @(Get-ChildItem -LiteralPath (Join-Path $projectRoot 'AssetDevelopment') -Recurse -File |
    Where-Object {
        $relative = $_.FullName.Substring($projectRoot.Length + 1).Replace('\', '/')
        ($_.Extension -eq '.fbx' -and @($sourcePrefixes | Where-Object { $relative.StartsWith($_) }).Count -gt 0) -or
            ($relative -in $sourceManifests)
    })
$importerNames = @('Import-FlickBlueStandardPrototype.py', 'Import-FlickHighDetailPucks.py',
    'Import-FlickArena.py', 'Import-FlickClassicArena.py', 'Import-FlickStadium.py',
    'Import-FlickBobArena.py', 'Import-FlickBobStadium.py')
$sources += @(Get-ChildItem -LiteralPath (Join-Path $projectRoot 'Tools') -File |
    Where-Object { $_.Name -in $importerNames -or $_.Name -in @('Update-FlickAssets.py', 'FlickAssetSelection.py') })
$hashes = @{}
foreach ($source in $sources) {
    $relative = $source.FullName.Substring($projectRoot.Length + 1).Replace('\', '/')
    $hashes[$relative] = (Get-FileHash -LiteralPath $source.FullName -Algorithm SHA256).Hash
}
$previous = @{}
if (Test-Path -LiteralPath $stampFile) {
    $saved = Get-Content -LiteralPath $stampFile -Raw | ConvertFrom-Json
    foreach ($property in $saved.PSObject.Properties) { $previous[$property.Name] = $property.Value }
}
$changed = @($hashes.Keys | Where-Object { $Force -or -not $previous.ContainsKey($_) -or $previous[$_] -ne $hashes[$_] } | Sort-Object)
$removed = @($previous.Keys | Where-Object { -not $hashes.ContainsKey($_) })
if ($removed.Count -gt 0) {
    throw "Asset sources were removed since the last import. Restore or resolve them before updating:`n  $($removed -join "`n  ")"
}
if ($changed.Count -eq 0) {
    Write-Host 'No source assets changed; nothing to import.'
    return
}
Write-Host "Changed source files ($($changed.Count)):"
$changed | ForEach-Object { Write-Host "  $_" }
if ($ListOnly) { return }

$selection = if ($Force -or $previous.Count -eq 0 -or $changed -contains 'Tools/Update-FlickAssets.py' -or $changed -contains 'Tools/FlickAssetSelection.py') {
    $null # A missing selection means full import, including script changes.
} else { ConvertTo-Json -InputObject $changed -Compress }
$oldSelection = $env:FLICK_ASSET_UPDATE_SELECTION
try {
    $env:FLICK_ASSET_UPDATE_SELECTION = $selection
    Write-Host 'Updating changed FLICK assets...'
    & $UnrealEditor $projectFile `
        -run=pythonscript `
        "-script=$importScript" `
        -unattended `
        -nop4 `
        -nosplash `
        -NoSound
} finally {
    $env:FLICK_ASSET_UPDATE_SELECTION = $oldSelection
}

if ($LASTEXITCODE -ne 0) {
    throw "Unreal asset update failed with exit code $LASTEXITCODE. See Saved\Logs\FLICK.log."
}

$logFile = Join-Path $projectRoot 'Saved\Logs\FLICK.log'
if (-not (Test-Path -LiteralPath $logFile) -or
    -not (Select-String -LiteralPath $logFile -SimpleMatch 'FLICK_ASSET_UPDATE_COMPLETE' -Quiet)) {
    throw 'Unreal exited without confirming completion. See Saved\Logs\FLICK.log.'
}

$hashes | ConvertTo-Json -Depth 2 | Set-Content -LiteralPath $stampFile -Encoding UTF8
Write-Host 'Asset update complete. Review the updated meshes in Unreal, then commit their .uasset files.'
