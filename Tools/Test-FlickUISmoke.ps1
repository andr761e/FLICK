<#
.SYNOPSIS
Captures the two high-risk menu layouts at standard and ultrawide resolutions.
.NOTES
Requires a current FLICKEditor build. This checks that both screens render at
the requested size; inspect the PNGs for text clipping and overlapping widgets.
#>
[CmdletBinding()]
param(
    [string]$EngineRoot = 'C:\Program Files\Epic Games\UE_5.8'
)

$ErrorActionPreference = 'Stop'
$captureScript = Join-Path $PSScriptRoot 'Capture-FlickUI.ps1'
Add-Type -AssemblyName System.Drawing
foreach ($size in @(@(1280, 720), @(1600, 900), @(3440, 1440))) {
    $captures = & $captureScript -Screen Customize,CustomizePuck,PrivateMatch -Width $size[0] -Height $size[1] -EngineRoot $EngineRoot
    if (@($captures).Count -ne 3) {
        throw "Expected three UI captures at $($size[0])x$($size[1])."
    }
    foreach ($capture in $captures) {
        if (-not (Test-Path -LiteralPath $capture.Screenshot -PathType Leaf)) {
            throw "Missing UI capture: $($capture.Screenshot)"
        }
        $bitmap = [System.Drawing.Image]::FromFile($capture.Screenshot)
        try {
            if ($bitmap.Width -ne $size[0] -or $bitmap.Height -ne $size[1]) {
                throw "Wrong capture dimensions: $($capture.Screenshot) ($($bitmap.Width)x$($bitmap.Height))"
            }
        }
        finally {
            $bitmap.Dispose()
        }
        Write-Host "PASS $($capture.Screen) $($size[0])x$($size[1]): $($capture.Screenshot)"
    }
}
