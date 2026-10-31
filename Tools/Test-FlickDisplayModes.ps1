<#
.SYNOPSIS
Exercises real window/fullscreen transitions with an isolated legacy profile.
.NOTES
Build FLICKEditor first. Only FLICK's own test windows are hidden/resized.
Other visible windows and desktop display settings are read before and after.
#>
[CmdletBinding()]
param([int]$TimeoutSeconds = 120)
$ErrorActionPreference = 'Stop'
$project = Split-Path $PSScriptRoot -Parent
$editor = 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe'
$runDir = Join-Path $project ('Saved/DisplayModeQA/' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
$legacySettings = @'
[/Script/Engine.GameUserSettings]
Version=5
FullscreenMode=0
LastConfirmedFullscreenMode=0
PreferredFullscreenMode=0
ResolutionSizeX=1680
ResolutionSizeY=1050
LastUserConfirmedResolutionSizeX=1680
LastUserConfirmedResolutionSizeY=1050
FrameRateLimit=60.000000
'@
foreach ($platform in @('Windows', 'WindowsEditor')) {
    $configDir = Join-Path $runDir ('Saved/Config/' + $platform)
    New-Item -ItemType Directory -Path $configDir -Force | Out-Null
    $legacySettings | Set-Content -LiteralPath (Join-Path $configDir 'GameUserSettings.ini') -Encoding UTF8
}

if (-not ('FlickDisplayAudit' -as [type])) {
Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
public static class FlickDisplayAudit {
    [StructLayout(LayoutKind.Sequential)] public struct Point { public int X,Y; }
    [StructLayout(LayoutKind.Sequential)] public struct Rect { public int Left,Top,Right,Bottom; }
    [StructLayout(LayoutKind.Sequential)] public struct Placement {
        public uint Length,Flags,Show; public Point Min,Max; public Rect Normal;
    }
    [StructLayout(LayoutKind.Sequential, CharSet=CharSet.Unicode)] struct Mode {
        [MarshalAs(UnmanagedType.ByValTStr,SizeConst=32)] public string Device;
        public ushort Spec,Driver,Size,Extra; public uint Fields;
        public int X,Y; public uint Orientation,FixedOutput;
        public short Color,Duplex,YResolution,TTOption,Collate;
        [MarshalAs(UnmanagedType.ByValTStr,SizeConst=32)] public string Form;
        public ushort LogPixels;
        public uint Bits,Width,Height,Flags,Hz,IcmMethod,IcmIntent,Media,Dither,Reserved1,Reserved2,PanWidth,PanHeight;
    }
    delegate bool Callback(IntPtr window,IntPtr data);
    [DllImport("user32.dll")] static extern bool EnumWindows(Callback callback,IntPtr data);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr window);
    [DllImport("user32.dll",EntryPoint="GetWindowLongPtrW")] static extern IntPtr GetWindowLongPtr(IntPtr window,int index);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr window,out uint id);
    [DllImport("user32.dll")] static extern bool GetWindowPlacement(IntPtr window,ref Placement placement);
    [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr window,out Rect rect);
    [DllImport("user32.dll")] static extern bool ShowWindow(IntPtr window,int command);
    [DllImport("user32.dll")] static extern int GetSystemMetrics(int index);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)] static extern bool EnumDisplaySettings(string device,int index,ref Mode mode);
    public static Dictionary<long,string> Windows() {
        var result=new Dictionary<long,string>();
        EnumWindows((window,data)=>{
            if(!IsWindowVisible(window)) return true;
            // Ignore passive tool/transparent overlays (e.g. NVIDIA's FPS layer).
            // Their own backing surfaces can move by a pixel when apps render.
            if((GetWindowLongPtr(window,-20).ToInt64() & (0x80L|0x20L|0x08000000L))!=0) return true;
            uint id; GetWindowThreadProcessId(window,out id);
            var p=new Placement {Length=(uint)Marshal.SizeOf(typeof(Placement))}; Rect r;
            if(GetWindowPlacement(window,ref p) && GetWindowRect(window,out r))
                result[window.ToInt64()]=String.Format("{0}|{1}|{2},{3},{4},{5}|{6},{7},{8},{9}",id,p.Show,
                    r.Left,r.Top,r.Right,r.Bottom,p.Normal.Left,p.Normal.Top,p.Normal.Right,p.Normal.Bottom);
            return true;
        },IntPtr.Zero);
        return result;
    }
    public static string Desktop() {
        var m=new Mode {Size=(ushort)Marshal.SizeOf(typeof(Mode))};
        if(!EnumDisplaySettings(null,-1,ref m)) throw new Exception("Cannot read desktop display mode");
        return String.Format("{0}x{1}@{2}Hz/{3}bit; virtual {4},{5},{6},{7}",m.Width,m.Height,m.Hz,m.Bits,
            GetSystemMetrics(76),GetSystemMetrics(77),GetSystemMetrics(78),GetSystemMetrics(79));
    }
    public static void HideOwnWindows(uint processId) {
        EnumWindows((window,data)=>{
            uint id; GetWindowThreadProcessId(window,out id);
            if(id==processId && IsWindowVisible(window)) ShowWindow(window,0);
            return true;
        },IntPtr.Zero);
    }
}
'@
}
$desktopBefore = [FlickDisplayAudit]::Desktop()
$windowsBefore = [FlickDisplayAudit]::Windows()
$log = Join-Path $runDir 'display-modes.log'
$arguments = @(
    ('"' + (Join-Path $project 'FLICK.uproject') + '"'), '/Engine/Maps/Entry',
    '-game', '-unattended', '-nop4', '-nosound', '-nosteam', '-nosplash',
    '-ddc=InstalledNoZenLocalFallback', '-FlickDisplayModeSmoke',
    ('-UserDir="' + $runDir + '"'), ('-abslog="' + $log + '"'),
    '-ExecCmds="t.IdleWhenNotForeground 0,Automation RunTests FLICK.UI.DisplayModeTransitions"',
    '-TestExit="Automation Test Queue Empty"'
)
$process = Start-Process -FilePath $editor -ArgumentList $arguments -PassThru -WindowStyle Hidden
$deadline = (Get-Date).AddSeconds($TimeoutSeconds)
$desktopStable = $true
try {
    while (-not $process.HasExited -and (Get-Date) -lt $deadline) {
        [FlickDisplayAudit]::HideOwnWindows([uint32]$process.Id)
        if ([FlickDisplayAudit]::Desktop() -ne $desktopBefore) { $desktopStable = $false }
        Start-Sleep -Milliseconds 200
        $process.Refresh()
    }
    if (-not $process.HasExited) { throw 'Display mode test timed out' }
} finally {
    if (-not $process.HasExited) { Stop-Process -Id $process.Id }
}
$windowsAfter = [FlickDisplayAudit]::Windows()
$changedWindows = @($windowsBefore.Keys | Where-Object {
    $windowsAfter.ContainsKey($_) -and $windowsBefore[$_] -ne $windowsAfter[$_]
} | ForEach-Object { [pscustomobject]@{Handle=$_; Before=$windowsBefore[$_]; After=$windowsAfter[$_]} })
$result = [pscustomobject]@{
    DesktopBefore=$desktopBefore; DesktopAfter=[FlickDisplayAudit]::Desktop()
    DesktopStable=$desktopStable; ExistingWindowsChecked=$windowsBefore.Count
    ChangedWindows=$changedWindows; ExitCode=$process.ExitCode; Log=$log
}
$result | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $runDir 'desktop-audit.json')
$contents = Get-Content -LiteralPath $log -Raw
if ($contents -notmatch 'DISPLAY_MODE_SMOKE_PASS' -or $contents -notmatch 'Result=\{Success\}' -or $process.ExitCode -ne 0) {
    throw "Display transition test failed. See $log"
}
if (-not $desktopStable -or $result.DesktopAfter -ne $desktopBefore -or $changedWindows.Count -gt 0) {
    throw "Desktop/other-window state changed during the test. See $runDir/desktop-audit.json"
}
$result
