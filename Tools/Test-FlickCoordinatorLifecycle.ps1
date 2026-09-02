$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$Dll = Join-Path $PSScriptRoot 'FlickCoordinator\bin\Release\net8.0\FlickCoordinator.dll'
$BaseUrl = 'http://127.0.0.1:8092'
$env:FLICK_COORDINATOR_AUTO_LAUNCH = 'true'
$env:FLICK_COORDINATOR_PUBLIC_URL = $BaseUrl
$env:FLICK_SERVER_EXECUTABLE = $env:ComSpec
$env:FLICK_COORDINATOR_STARTUP_TIMEOUT = '5'
$env:FLICK_COORDINATOR_HEARTBEAT_TIMEOUT = '5'
$env:FLICK_COORDINATOR_SUPERVISION_INTERVAL = '1'
$env:FLICK_SERVER_CREDENTIAL_LIFETIME = '300'
$env:FLICK_ALLOW_DEVELOPMENT_SERVER_KEY = 'false'

dotnet build (Join-Path $PSScriptRoot 'FlickCoordinator\FlickCoordinator.csproj') --configuration Release | Out-Host
if ($LASTEXITCODE -ne 0) { throw 'Coordinator build failed.' }

$Process = Start-Process -FilePath 'dotnet' -ArgumentList @($Dll, '--urls', $BaseUrl) -WorkingDirectory $Root -WindowStyle Hidden -PassThru
try {
    for ($Attempt = 0; $Attempt -lt 30; $Attempt++) {
        try {
            $Health = Invoke-RestMethod -Uri "$BaseUrl/health/ready" -TimeoutSec 1
            if ($Health.status -eq 'ok') { break }
        } catch {
            Start-Sleep -Milliseconds 200
        }
    }
    if (-not $Health -or $Health.status -ne 'ok') { throw 'Coordinator did not become ready.' }

    function Add-QueueTicket([string]$AccountId) {
        $Body = @{
            party_id = "crash-$AccountId"
            region = 'local'
            variant = 0
            players_per_team = 1
            ranked = $false
            members = @(@{
                account_id = $AccountId
                display_name = $AccountId
                steam_ticket = ''
                party_slot = 0
                rating_snapshot = 1000
            })
        } | ConvertTo-Json -Depth 5
        Invoke-RestMethod -Method Post -Uri "$BaseUrl/v1/matchmaking/tickets" -ContentType 'application/json' -Body $Body
    }

    $TicketOne = Add-QueueTicket 'CrashPlayer1'
    $TicketTwo = Add-QueueTicket 'CrashPlayer2'
    $FirstStatus = Invoke-RestMethod -Uri "$BaseUrl/v1/matchmaking/tickets/$($TicketOne.ticket_id)"
    if ($FirstStatus.status -eq 'allocated' -or $FirstStatus.allocation) {
        throw 'An unready server allocation was exposed to the client.'
    }

    $Failed = $false
    for ($Attempt = 0; $Attempt -lt 30; $Attempt++) {
        $Status = Invoke-RestMethod -Uri "$BaseUrl/v1/matchmaking/tickets/$($TicketOne.ticket_id)"
        if ($Status.status -eq 'error') {
            $Failed = $true
            break
        }
        Start-Sleep -Milliseconds 250
    }
    if (-not $Failed) { throw 'The crashed server allocation did not fail.' }
    $Health = Invoke-RestMethod -Uri "$BaseUrl/health"
    if ($Health.failed_servers -lt 1) { throw 'The failed server was not reflected in health state.' }

    Write-Host 'FLICK coordinator lifecycle test PASS: crashed allocation was withheld and cleaned up.'
} finally {
    if ($Process -and -not $Process.HasExited) { Stop-Process -Id $Process.Id -Force }
}
