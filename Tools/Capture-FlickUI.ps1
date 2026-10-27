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
    [ValidateSet('Home', 'Invite', 'Party', 'Challenges', 'Play', 'Format', 'CompetitiveFormat', 'Profile', 'Customize', 'CustomizePuck', 'PrivateMatch', 'Spectator', 'Settings', 'Video', 'Lighting', 'LightingPreview', 'Lineup', 'Class', 'Shop', 'Social', 'Match', 'Replay', 'AimArrow', 'PuckHover', 'Training', 'TrainingMenu', 'FreePlayMenu', 'Practice', 'PracticePacks', 'Pause', 'Result', 'PostMatch', 'PostMatchPrivate', 'PostMatchBob', 'PostMatchRanked', 'Scoreboard', 'TestArena', 'SwitchRim', 'StadiumSteps', 'TestArenaStates', 'TestArenaSettings', 'Bob', 'BobPocket', 'BobStriker')]
    [string[]]$Screen = @('Home'),
    [ValidateRange(640, 7680)]
    [int]$Width = 1600,
    [ValidateRange(480, 4320)]
    [int]$Height = 900,
    [string]$Map = '/Engine/Maps/Entry',
    [ValidateRange(0, 3)]
    [int]$CameraView = 0,
    # Small orbit offsets reproduce view-dependent material glare.
    [ValidateRange(-180, 180)]
    [float]$CameraOrbit = 0,
    # Isolated visual QA framing only; never changes the normal player profile.
    [ValidateRange(0.75, 1.25)]
    [float]$CameraDistance = 0.9,
    # Isolated lighting experiments; only these seven presentation controls exist.
    [hashtable]$LightingValues = @{},
    [ValidateRange(0, 3)]
    [int]$Quality = 3,
    [ValidateRange(0, 5)]
    [int]$PuckSkin = 0,
    [ValidateRange(-1, 5)]
    [int]$MatchPuckSkin = -1,
    # Capture-only exposure study; does not alter a saved preference.
    [Nullable[float]]$ExposureBias = $null,
    [ValidateRange(0, 8)]
    [int]$ReplayPuckClass = 0,
    [Nullable[float]]$CameraElevation = $null,
    [ValidateRange(-16, 16)]
    [int]$CameraZoomSteps = 0,
    [ValidateRange(-1, 2)]
    [int]$PuckVisualState = -1,
    [ValidateRange(1, 3)]
    [int]$PlayersPerTeam = 1,
    [ValidateRange(0, 4)]
    [int]$PlaySelection = 0,
    [ValidateRange(0, 3)]
    [int]$LineupSlot = 0,
    [ValidateRange(-1, 8)]
    [int]$LineupHover = -1,
    [switch]$LineupRadar,
    [ValidateRange(0, 14)]
    [int]$LockerCategory = 0,
    [ValidateRange(-1, 64)]
    [int]$LockerItem = -1,
    [ValidateRange(0, 4)]
    [int]$ProfilePage = 0,
    [ValidateRange(0, 9)]
    [int]$SettingsTab = 0,
    [ValidateRange(-1, 5)]
    [int]$ShopItem = -1,
    [ValidateRange(0, 4)]
    [int]$ShopFilter = 0,
    [switch]$ShopOwnedPreview,
    [ValidateRange(-1, 3)]
    [int]$PracticeCategory = -1,
    [ValidateRange(0, 2)]
    [int]$PracticeDifficulty = 0,
    [ValidateRange(15, 600)]
    [int]$TimeoutSeconds = 120,
    [ValidateRange(1, 30)]
    [float]$CaptureDelaySeconds = 2,
    # Sequence of live samples for temporal shimmer checks; retained in UserDir.
    [ValidateRange(1, 32)]
    [int]$CaptureFrames = 1,
    [switch]$HideControlOverview,
    # Opt in for audio-system checks; ordinary visual captures stay silent.
    [switch]$EnableAudio,
    [ValidateSet('Default', 'D3D11')]
    [string]$GraphicsAPI = 'Default',
    [string]$EngineRoot = $(if ($env:FLICK_UNREAL_ENGINE_ROOT) { $env:FLICK_UNREAL_ENGINE_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' })
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
    PostMatch = @('-FlickPostMatchPreview')
    PostMatchPrivate = @('-FlickPostMatchPreview', '-FlickPostMatchPrivate')
    PostMatchBob = @('-FlickPostMatchPreview', '-FlickPostMatchBob')
    PostMatchRanked = @('-FlickPostMatchPreview', '-FlickPostMatchRanked')
    Invite = @('-FlickPartyInvitePreview')
    Party = @('-FlickPartyTrayPreview')
    Challenges = @('-FlickChallengesPreview')
    TestArena = @('-FlickTestArenaPreview')
    SwitchRim = @('-FlickTestArenaPreview', '-FlickSwitchRimPreview')
    StadiumSteps = @('-FlickTestArenaPreview', '-FlickStadiumStepPreview')
    Bob = @('-FlickBobPreview')
    BobStriker = @('-FlickBobPreview', '-FlickBobStrikerPreview')
    BobPocket = @('-FlickBobPocketTest')
    TestArenaSettings = @('-FlickTestArenaSettingsPreview')
    TestArenaStates = @('-FlickTestArenaPreview', '-FlickTestArenaStatePreview')
    Play = @('-FlickModeSelectPreview')
    Format = @('-FlickModeSelectPreview', '-FlickPlayFormatPreview')
    CompetitiveFormat = @('-FlickModeSelectPreview', '-FlickPlayFormatPreview', '-FlickCompetitiveFormatPreview')
    Profile = @('-FlickProfilePreview')
    Customize = @('-FlickProfilePreview', '-FlickProfileCustomizePreview')
    CustomizePuck = @('-FlickProfilePreview', '-FlickProfileCustomizePreview')
    PrivateMatch = @('-FlickPrivateMatchPreview')
    Spectator = @('-FlickPrivateSpectatorPreview')
    Settings = @('-FlickSettingsPreview')
    Video = @('-FlickSettingsPreview')
    Lighting = @('-FlickSettingsPreview')
    LightingPreview = @('-FlickSettingsPreview', '-FlickLightingPreview')
    Lineup = @('-Flick4v4LoadoutPreview')
    Class = @('-FlickClassSelectPreview')
    Shop = @('-FlickItemShopPreview')
    Social = @('-FlickSocialPreview')
    Match = @('-Flick4v4Preview')
    Replay = @('-Flick4v4Preview', '-FlickReplayPreview', '-FlickFpsPreview')
    AimArrow = @('-Flick4v4Preview', '-FlickAimArrowPreview')
    PuckHover = @('-Flick4v4Preview', '-FlickPuckHoverPreview')
    Training = @('-FlickTrainingPreview')
    TrainingMenu = @('-FlickModeSelectPreview', '-FlickTrainingMenuPreview')
    FreePlayMenu = @('-FlickModeSelectPreview', '-FlickFreePlayMenuPreview')
    Practice = @('-FlickPracticePreview')
    PracticePacks = @('-FlickModeSelectPreview', '-FlickPracticeMenuPreview')
    Pause = @('-FlickPausePreview')
    # A completed series opens the winner cinematic over the round panel.
    # Preview a live board instead so Result captures the actual round overlay.
    Result = @('-Flick4v4Preview', '-FlickRoundOverPreview')
    Scoreboard = @('-Flick4v4Preview', '-FlickScoreboardPreview')
}
$runName = '{0}-{1}' -f (Get-Date -Format 'yyyyMMdd-HHmmss'), ([guid]::NewGuid().ToString('N').Substring(0, 6))
$captureRoot = Join-Path $projectRoot ('Saved\UICaptures\' + $runName)
New-Item -ItemType Directory -Path $captureRoot -Force | Out-Null
$captures = @()

foreach ($screenName in $Screen) {
    $selectedLockerCategory = if ($screenName -eq 'CustomizePuck') { 3 } else { $LockerCategory }
    $selectedSettingsTab = if ($screenName -eq 'Video') { 4 } elseif ($screenName -in @('Lighting', 'LightingPreview')) { 7 } else { $SettingsTab }
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
        '-nosteam', '-NoScreenMessages', '-ForceRes',
        "-ResX=$Width", "-ResY=$Height", '-FlickSkipIntro', '-FlickCaptureFrame', "-FlickCaptureDelaySeconds=$CaptureDelaySeconds",
        "-FlickCaptureFrames=$CaptureFrames",
        "-FlickCameraView=$CameraView", "-FlickPlayersPerTeam=$PlayersPerTeam", "-FlickLockerCategory=$selectedLockerCategory", "-FlickLockerItem=$LockerItem", "-FlickProfilePage=$ProfilePage", "-FlickSettingsTab=$selectedSettingsTab", '-FlickTestArenaSeed=1337',
        "-FlickCaptureOrbit=$($CameraOrbit.ToString([System.Globalization.CultureInfo]::InvariantCulture))",
        "-ini:GameUserSettings:[FLICK.GameplaySettings]:GameplayCameraDistance=$($CameraDistance.ToString([System.Globalization.CultureInfo]::InvariantCulture))",
        "-FlickPracticeCategory=$PracticeCategory", "-FlickPracticeDifficulty=$PracticeDifficulty",
        ('-UserDir="{0}/"' -f $userDir.Replace('\', '/')),
        ('-ShaderWorkingDir="{0}/"' -f (Join-Path $projectRoot 'Intermediate\UIShaders').Replace('\', '/')),
        ('-abslog="{0}"' -f $logPath)
    ) + $(if ($GraphicsAPI -eq 'D3D11') { @('-d3d11') } else { @() }) + $previewFlags[$screenName]
    if (-not $EnableAudio) { $arguments += '-nosound' }
    if ($HideControlOverview) {
        $arguments += '-ini:GameUserSettings:[FLICK.GameplaySettings]:ControlOverviewEnabled=False'
    }
    $arguments += @("-FlickShopInspect=$ShopItem", "-FlickShopFilter=$ShopFilter")
    foreach ($setting in $LightingValues.GetEnumerator()) {
        if ($setting.Key -notin @('Ambient', 'Fill', 'Key', 'Rim', 'Accents', 'Direct', 'Highlights')) {
            throw "Unknown lighting control: $($setting.Key)"
        }
        $value = [float]$setting.Value
        $maximum = if ($setting.Key -eq 'Highlights') { 1.0 } else { 3.0 }
        if ([float]::IsNaN($value) -or [float]::IsInfinity($value) -or $value -lt 0 -or $value -gt $maximum) {
            throw "Lighting control $($setting.Key) must be between 0 and $maximum"
        }
        $arguments += "-ini:GameUserSettings:[FLICK.LightingSettings]:Menu.$($setting.Key)=$($value.ToString([System.Globalization.CultureInfo]::InvariantCulture))"
    }
    foreach ($group in @('ViewDistance', 'AntiAliasing', 'Shadow', 'GlobalIllumination', 'Reflection', 'PostProcess', 'Texture', 'Effects', 'Foliage', 'Shading')) {
        $arguments += "-ini:GameUserSettings:[ScalabilityGroups]:sg.${group}Quality=$Quality"
    }
    $arguments += "-FlickBobStrikerSkin=$PuckSkin"
    $arguments += "-FlickCapturePuckSkin=$MatchPuckSkin"
    if ($null -ne $ExposureBias) { $arguments += "-FlickCaptureExposure=$ExposureBias" }
    if ($screenName -eq 'Replay') { $arguments += "-FlickCaptureReplayClass=$ReplayPuckClass" }
    if ($null -ne $CameraElevation) { $arguments += "-FlickCaptureElevation=$CameraElevation" }
    $arguments += "-FlickCaptureZoomSteps=$CameraZoomSteps"
    if ($PuckVisualState -ge 0) { $arguments += "-FlickCapturePuckState=$PuckVisualState" }
    $arguments += "-FlickPlaySelection=$PlaySelection"
    $arguments += @("-FlickLineupSlot=$LineupSlot", "-FlickLineupHover=$LineupHover")
    if ($LineupRadar) { $arguments += '-FlickLineupRadar' }
    if ($ShopOwnedPreview) { $arguments += '-FlickShopOwnedPreview' }
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
    $frameCount = @(Get-ChildItem -LiteralPath $screenshot.DirectoryName -Filter '*.png' -File).Count
    if ($frameCount -ne $CaptureFrames) {
        throw "Expected $CaptureFrames live screenshots, got $frameCount for $screenName. See $logPath"
    }
    $outputPath = Join-Path $captureRoot ($captureName + '.png')
    Copy-Item -LiteralPath $screenshot.FullName -Destination $outputPath
    $captures += [pscustomobject]@{
        Screen = $screenName
        Width = $Width
        Height = $Height
        Quality = $Quality
        CameraView = $CameraView
        CameraOrbit = $CameraOrbit
        LightingValues = $LightingValues
        MatchPuckSkin = $MatchPuckSkin
        ExposureBias = $ExposureBias
        CameraElevation = $CameraElevation
        CameraZoomSteps = $CameraZoomSteps
        PuckVisualState = $PuckVisualState
        ReplayPuckClass = $(if ($screenName -eq 'Replay') { $ReplayPuckClass } else { $null })
        Screenshot = $outputPath
        Log = $logPath
    }
    Write-Host "Saved $outputPath"
}

$captures | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $captureRoot 'captures.json') -Encoding UTF8
$captures
