<#
.SYNOPSIS
Tests the actual Development package, including its normal intro and Quit button.
.NOTES
Uses isolated profiles and offscreen rendering. Does not change the player's
settings, skip the intro, or run a preview arena. Times out a hung test process.
#>
[CmdletBinding()]
param(
    [string]$PackageRoot = '',
    [ValidateRange(15, 180)][int]$TimeoutSeconds = 60,
    [switch]$WithSteam
)
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
if (!$PackageRoot) { $PackageRoot = Join-Path $projectRoot 'Builds/Development/Windows' }
$binary = Join-Path $PackageRoot 'FLICK/Binaries/Win64/FLICK.exe'
if (!(Test-Path -LiteralPath $binary -PathType Leaf)) { throw "Package executable not found: $binary" }
$runRoot = Join-Path $projectRoot ('Saved/PackageStartupQA/' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N').Substring(0, 6))
foreach ($case in @('FreshProfile', 'SavedShowcase')) {
    $profile = Join-Path $runRoot $case
    New-Item -ItemType Directory -Path $profile -Force | Out-Null
    $expected = if ($case -eq 'SavedShowcase') { 7 } else { 0 }
    if ($case -eq 'SavedShowcase') {
        $configDir = Join-Path $profile 'Saved/Config/Windows'
        New-Item -ItemType Directory -Path $configDir -Force | Out-Null
        # Bouncer + Amethyst, with BOB remembered as the last selected playlist.
        # Main-menu startup must still be the solo Knockout showcase.
        # Start with a real versioned engine config: Unreal deliberately resets
        # a hand-written file without its required GameUserSettings version.
        $baseConfig = Get-Content -LiteralPath (Join-Path $runRoot 'FreshProfile/Saved/Config/Windows/GameUserSettings.ini') -Raw
        [IO.File]::WriteAllText((Join-Path $configDir 'GameUserSettings.ini'),
            $baseConfig + "`r`n[FLICK.ProfileCosmetics]`r`nShowcasePuck=7`r`nPuckSkin.Bouncer=3`r`n`r`n[FLICK.GameplaySettings]`r`nSelectedMatchVariant=1`r`n")
    }
    $log = Join-Path $profile 'startup.log'
    $arguments = @('-windowed', '-RenderOffscreen', '-unattended', '-nosplash', '-NoScreenMessages',
        '-ResX=1280', '-ResY=720', '-ForceRes', '-FlickStartupSmokeQuit', "-FlickExpectedShowcase=$expected",
        '-ExecCmds="Automation RunTests FLICK.Frontend.PackagedStartup"',
        ('-UserDir="{0}"' -f $profile), ('-abslog="{0}"' -f $log))
    if (!$WithSteam) { $arguments += '-nosteam' }
    $process = Start-Process -FilePath $binary -ArgumentList $arguments -WorkingDirectory (Split-Path -Parent $binary) -WindowStyle Hidden -PassThru
    try {
        $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
        while (!$process.WaitForExit(1000)) {
            if ((Get-Date) -gt $deadline) { throw "Packaged startup hung ($case). Log: $log" }
        }
        $content = Get-Content -LiteralPath $log -Raw
        if ($process.ExitCode -ne 0 -or $content -notmatch 'PACKAGED_STARTUP_SMOKE_TEST PASS' -or
            $content -notmatch 'PACKAGED_STARTUP_QUIT_BUTTON PASS' -or
            $content -notmatch 'Test Completed\. Result=\{Success\} Name=\{PackagedStartup\}' -or
            $content -match '=== Handled ensure: ===|Fatal error:|PACKAGED_STARTUP_SMOKE_TEST FAIL') {
            throw "Packaged startup failed ($case), exit $($process.ExitCode). Log: $log"
        }
        # Packaged games do not ship Unreal's editor-only HTML report template.
        # Keep a small local summary instead of requesting that missing template.
        $summary = [ordered]@{Case=$case; SteamEnabled=[bool]$WithSteam; Passed=$true; ExitCode=$process.ExitCode;
            ExpectedShowcase=$expected; IntroSkipped=$false; Log=$log; Screenshot=(Join-Path $profile 'Saved/PackagedStartup.png')}
        [IO.File]::WriteAllText((Join-Path $profile 'result.json'), ($summary | ConvertTo-Json))
        Write-Host "PASS $case (normal intro, equipped puck/skin, Settings, Play, Quit): $profile"
    }
    finally {
        if (!$process.HasExited) { Stop-Process -Id $process.Id }
        $process.Dispose()
    }
}
