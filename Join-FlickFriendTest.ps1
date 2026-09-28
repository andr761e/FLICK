param(
    [string]$HostIp,
    [string]$PlayerId,
    [switch]$ManualSearch
)

$ErrorActionPreference = 'Stop'
$Here = Split-Path -Parent $MyInvocation.MyCommand.Path
$PackageRoot = if (Test-Path (Join-Path $Here 'FLICK.exe')) {
    $Here
} else {
    Join-Path $Here 'Builds\Development\Windows'
}
if ($ManualSearch) {
    $EndpointFile = Join-Path $PackageRoot 'friend-server.txt'
    if (-not (Test-Path $EndpointFile)) {
        throw 'Friend server address missing. Start the server first, then share the configured Windows package.'
    }
    $HostIp = (Get-Content $EndpointFile -TotalCount 1).Trim()
    $PlayerId = (([Environment]::MachineName + '-' + [Environment]::UserName) -replace '[^A-Za-z0-9_-]', '_')
    $PlayerId = $PlayerId.Substring(0, [Math]::Min(32, $PlayerId.Length))
}
$Address = $null
if (-not [System.Net.IPAddress]::TryParse($HostIp, [ref]$Address) -or
    $Address.AddressFamily -ne [System.Net.Sockets.AddressFamily]::InterNetwork) {
    throw 'Enter the host PC VPN or LAN IPv4 address.'
}
if ($PlayerId -notmatch '^[A-Za-z0-9_-]{1,32}$') {
    throw 'Player ID must be 1-32 letters, numbers, underscores, or hyphens.'
}
$Game = Join-Path $Here 'FLICK.exe'
if (-not (Test-Path $Game)) {
    $Game = Join-Path $Here 'Builds\Development\Windows\FLICK.exe'
}
if (-not (Test-Path $Game)) {
    throw 'Packaged Development client not found. Run package-flick-development.cmd, then use this command from the package folder.'
}

$Url = "http://${HostIp}:8090"
Write-Host "Starting FLICK against $Url as $PlayerId..."
$Arguments = @(
    '/Engine/Maps/Entry', '-nosteam', '-windowed', '-ResX=1280', '-ResY=720',
    "-FlickCoordinatorUrl=$Url", "-FlickLocalAccountId=$PlayerId"
)
if (-not $ManualSearch) {
    $Arguments += @('-FlickCoordinatorAutoQueue', '-FlickCoordinatorTeamSize=1')
}
Start-Process -FilePath $Game -WorkingDirectory (Split-Path -Parent $Game) -ArgumentList $Arguments
