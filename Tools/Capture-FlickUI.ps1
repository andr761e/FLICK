<#
.SYNOPSIS
Captures an actual Unreal viewport, including Slate UI, using the current editor build.
.EXAMPLE
.\Tools\Capture-FlickUI.ps1 -Screen Home -Width 1600 -Height 900
.EXAMPLE
.\Tools\Capture-FlickUI.ps1 -Screen Play,Format,Settings,Match,Pause -Width 1280 -Height 720
.NOTES
Build FLICKEditor first. Uses the existing non-shipping preview flags and Shot showui.
Each capture gets an isolated UserDir under Saved/UICaptures so preview matches and
settings do not overwrite the developer's normal profile. No Steam connection is used.
#>
[CmdletBinding()]
param(
    [ValidateSet('Home', 'Play', 'Format', 'Profile', 'Settings', 'Lineup', 'Class', 'Shop', 'Social', 'Match', 'Training', 'Pause', 'Result', 'Scoreboard', 'TestArena', 'TestArenaStates', 'TestArenaSettings')]
    [string[]]$Screen = @('Home'),
    [ValidateRange(640, 7680)]
    [int]$Width = 1600,
    [ValidateRange(480, 4320)]
    [int]$Height = 900,
    [string]$Map = '/Engine/Maps/Entry',
    [ValidateRange(0, 3)]
    [int]$CameraView = 0,
    [ValidateRange(15, 600)]
    [int]$TimeoutSeconds = 120,
    [string]$EngineRoot = $(if ($env:FLICK_UNREAL_ENGINE_ROOT) { $env:FLICK_UNREAL_ENGINE_ROOT } else { 'C:\Program Files\Epic Games\UE_5.6' })
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$projectPath = Join-Path $projectRoot 'FLICK.uproject'
$editorPath = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
if (-not (Test-Path -LiteralPath $editorPath -PathType Leaf)) {
    throw "UnrealEditor.exe was not found: $editorPath"
}
if (-not (Test-Path -LiteralPath $projectPath -PathType Leaf)) {
    throw "FLICK.uproject was not found: $projectPath"
}

$previewFlags = @{
    Home = @()
    TestArena = @('-FlickTestArenaPreview')
    TestArenaSettings = @('-FlickTestArenaSettingsPreview')
    TestArenaStates = @('-FlickTestArenaPreview', '-FlickTestArenaStatePreview')
    Play = @('-FlickModeSelectPreview')
    Format = @('-FlickModeSelectPreview', '-FlickPlayFormatPreview')
    Profile = @('-FlickProfilePreview')
    Settings = @('-FlickSettingsPreview')
    Lineup = @('-Flick4v4LoadoutPreview')
    Class = @('-FlickClassSelectPreview')
    Shop = @('-FlickItemShopPreview')
    Social = @('-FlickSocialPreview')
    Match = @('-Flick4v4Preview')
    Training = @('-FlickTrainingPreview')
    Pause = @('-FlickPausePreview')
    Result = @('-FlickMatchResultPreview')
    Scoreboard = @('-Flick4v4Preview', '-FlickScoreboardPreview')
}
$runName = '{0}-{1}' -f (Get-Date -Format 'yyyyMMdd-HHmmss'), ([guid]::NewGuid().ToString('N').Substring(0, 6))
$captureRoot = Join-Path $projectRoot ('Saved\UICaptures\' + $runName)
New-Item -ItemType Directory -Path $captureRoot -Force | Out-Null
$captures = @()

foreach ($screenName in $Screen) {
    $captureName = '{0}-{1}x{2}' -f $screenName.ToLowerInvariant(), $Width, $Height
    $userDir = Join-Path $captureRoot $captureName
    New-Item -ItemType Directory -Path $userDir -Force | Out-Null
    $logPath = Join-Path $userDir 'capture.log'
    # Start-Process joins ArgumentList into the native command line. Quote only
    # known path arguments; screen flags are selected from the closed table above.
    $arguments = @(
        ('"{0}"' -f $projectPath),
        ('"{0}"' -f $Map),
        '-game', '-windowed', '-RenderOffscreen', '-unattended', '-nosplash',
        '-ddc=InstalledNoZenLocalFallback',
        '-nosound', '-nosteam', '-NoScreenMessages', '-ForceRes',
        "-ResX=$Width", "-ResY=$Height", '-FlickSkipIntro', '-FlickCaptureFrame',
        "-FlickCameraView=$CameraView", '-FlickTestArenaSeed=1337',
        ('-UserDir="{0}/"' -f $userDir.Replace('\', '/')),
        ('-ShaderWorkingDir="{0}/"' -f (Join-Path $projectRoot 'Intermediate\UIShaders').Replace('\', '/')),
        ('-abslog="{0}"' -f $logPath)
    ) + $previewFlags[$screenName]
    Write-Host "Capturing $screenName at ${Width}x${Height}..."
    $previousCachePath = [Environment]::GetEnvironmentVariable('UE-LocalDataCachePath', 'Process')
    try {
        [Environment]::SetEnvironmentVariable('UE-LocalDataCachePath', (Join-Path $projectRoot 'DerivedDataCache'), 'Process')
        $captureProcess = Start-Process -FilePath $editorPath -ArgumentList $arguments -WorkingDirectory $projectRoot -WindowStyle Hidden -PassThru
    }
    finally {
        [Environment]::SetEnvironmentVariable('UE-LocalDataCachePath', $previousCachePath, 'Process')
    }
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while (-not $captureProcess.HasExited) {
        if ((Get-Date) -ge $deadline) {
            # Stop only the process started by this invocation.
            Stop-Process -Id $captureProcess.Id -Force -ErrorAction SilentlyContinue
            throw "Capture timed out for $screenName. See $logPath"
        }
        Start-Sleep -Milliseconds 500
        $captureProcess.Refresh()
    }
    if ($captureProcess.ExitCode -ne 0) {
        throw "Unreal exited with code $($captureProcess.ExitCode) for $screenName. See $logPath"
    }
    $screenshot = Get-ChildItem -LiteralPath $userDir -Filter '*.png' -Recurse -File |
        Where-Object { $_.DirectoryName -match '[\\/]Screenshots[\\/]' } |
        Sort-Object LastWriteTimeUtc -Descending | Select-Object -First 1
    if (-not $screenshot) {
        throw "Unreal exited without a UI screenshot for $screenName. See $logPath"
    }
    $outputPath = Join-Path $captureRoot ($captureName + '.png')
    Copy-Item -LiteralPath $screenshot.FullName -Destination $outputPath
    $captures += [pscustomobject]@{
        Screen = $screenName
        Width = $Width
        Height = $Height
        Screenshot = $outputPath
        Log = $logPath
    }
    Write-Host "Saved $outputPath"
}

$captures | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $captureRoot 'captures.json') -Encoding UTF8
$captures
