param(
    [string]$HostIp
)

$ErrorActionPreference = 'Stop'
$Root = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($HostIp)) {
    $SavedHostFile = Join-Path $Root 'Builds\Development\Windows\friend-server.txt'
    if (-not (Test-Path $SavedHostFile)) {
        throw 'First setup: run start-flick-friend-server.cmd YOUR_VPN_OR_LAN_IP.'
    }
    $HostIp = (Get-Content $SavedHostFile -TotalCount 1).Trim()
}
$Address = $null
if (-not [System.Net.IPAddress]::TryParse($HostIp, [ref]$Address) -or
    $Address.AddressFamily -ne [System.Net.Sockets.AddressFamily]::InterNetwork) {
    throw 'Enter the IPv4 address of this PC on your VPN or LAN.'
}
$Bytes = $Address.GetAddressBytes()
$PrivateAddress = $Bytes[0] -eq 10 -or
    ($Bytes[0] -eq 172 -and $Bytes[1] -ge 16 -and $Bytes[1] -le 31) -or
    ($Bytes[0] -eq 192 -and $Bytes[1] -eq 168) -or
    ($Bytes[0] -eq 100 -and $Bytes[1] -ge 64 -and $Bytes[1] -le 127)
if (-not $PrivateAddress) {
    throw 'Use a private LAN or VPN IPv4 address. The development coordinator must not be exposed directly to the public internet.'
}
$LocalAddresses = [System.Net.NetworkInformation.NetworkInterface]::GetAllNetworkInterfaces() |
    ForEach-Object { $_.GetIPProperties().UnicastAddresses } |
    ForEach-Object { $_.Address.IPAddressToString }
if ($HostIp -notin $LocalAddresses) {
    throw "$HostIp is not assigned to this PC. Connect the VPN first, then use this PC's VPN IPv4 address."
}

$Project = Join-Path $Root 'FLICK.uproject'
$EngineRoot = if ($env:FLICK_UNREAL_ENGINE_ROOT) { $env:FLICK_UNREAL_ENGINE_ROOT } else { 'C:\Program Files\Epic Games\UE_5.8' }
$EditorCommand = Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$CoordinatorProject = Join-Path $Root 'Tools\FlickCoordinator\FlickCoordinator.csproj'
$CoordinatorDll = Join-Path $Root 'Tools\FlickCoordinator\bin\Release\net8.0\FlickCoordinator.dll'
if (-not (Test-Path $Project)) { throw "Project missing: $Project" }
if (-not (Test-Path $EditorCommand)) { throw "Unreal Editor command missing: $EditorCommand" }
if (-not (Get-Command dotnet -ErrorAction SilentlyContinue)) { throw 'The dotnet CLI is required.' }

$PackageRoot = Join-Path $Root 'Builds\Development\Windows'
if (Test-Path (Join-Path $PackageRoot 'FLICK.exe')) {
    Set-Content -Path (Join-Path $PackageRoot 'friend-server.txt') -Value $HostIp -NoNewline -Encoding ascii
    Write-Host "Configured the packaged clients for $HostIp. Share the Windows folder after starting this command."
} else {
    Write-Warning 'Packaged client missing. Run package-flick-development.cmd before sharing the game.'
}

dotnet build $CoordinatorProject --configuration Release
if ($LASTEXITCODE -ne 0) { throw 'Coordinator build failed.' }

$env:FLICK_PROJECT_PATH = $Project
$env:FLICK_UNREAL_EDITOR_CMD = $EditorCommand
$env:FLICK_COORDINATOR_ENVIRONMENT = 'Development'
$env:FLICK_COORDINATOR_PUBLIC_URL = "http://${HostIp}:8090"
$env:FLICK_SERVER_PUBLIC_HOST = $HostIp
$env:FLICK_COORDINATOR_FIRST_PORT = '7780'
$env:FLICK_COORDINATOR_STARTUP_TIMEOUT = '120'
$env:FLICK_COORDINATOR_AUTO_LAUNCH = 'true'
$env:FLICK_SERVER_ENABLE_STEAM = 'false'
$env:FLICK_COORDINATOR_REQUIRE_STEAM = 'false'
$env:FLICK_BACKEND_SERVER_KEY = 'flick-local-development-key'

Write-Host "Coordinator: $env:FLICK_COORDINATOR_PUBLIC_URL"
Write-Host "Game server ports: UDP 7780 and up (one per active match)"
Write-Host 'Keep this window open. Close it to stop the coordinator and its match servers.'
Write-Host 'Both players: open FLICK.exe, then select Casual or Competitive 1v1 and Search.'
Write-Host 'Both PCs must use the same configured Development build and be on the same VPN or LAN.'
dotnet $CoordinatorDll --urls 'http://0.0.0.0:8090'
exit $LASTEXITCODE
