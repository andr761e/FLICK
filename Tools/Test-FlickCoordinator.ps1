$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$Dll = Join-Path $PSScriptRoot 'FlickCoordinator\bin\Release\net8.0\FlickCoordinator.dll'
$BaseUrl = 'http://127.0.0.1:8091'
$env:FLICK_COORDINATOR_AUTO_LAUNCH = 'false'
$env:FLICK_COORDINATOR_STARTUP_DELAY = '0'
$env:FLICK_COORDINATOR_PUBLIC_URL = $BaseUrl
$env:FLICK_BACKEND_SERVER_KEY = 'flick-local-development-key'

dotnet build (Join-Path $PSScriptRoot 'FlickCoordinator\FlickCoordinator.csproj') --configuration Release | Out-Host
if ($LASTEXITCODE -ne 0) { throw 'Coordinator build failed.' }

$LogDirectory = Join-Path $Root 'Saved\Coordinator'
New-Item -ItemType Directory -Force -Path $LogDirectory | Out-Null
$Process = Start-Process -FilePath 'dotnet' -ArgumentList @($Dll, '--urls', $BaseUrl) -WorkingDirectory $Root -WindowStyle Hidden -PassThru `
    -RedirectStandardOutput (Join-Path $LogDirectory 'coordinator-smoke.stdout.log') `
    -RedirectStandardError (Join-Path $LogDirectory 'coordinator-smoke.stderr.log')
try {
    $Healthy = $false
    for ($Attempt = 0; $Attempt -lt 30; $Attempt++) {
        try {
            $Health = Invoke-RestMethod -Uri "$BaseUrl/health" -TimeoutSec 1
            $Healthy = $Health.status -eq 'ok'
            if ($Healthy) { break }
        } catch {
            Start-Sleep -Milliseconds 200
        }
    }
    if (-not $Healthy) { throw 'Coordinator did not become healthy.' }
    $Liveness = Invoke-RestMethod -Uri "$BaseUrl/health/live"
    $Readiness = Invoke-RestMethod -Uri "$BaseUrl/health/ready"
    if ($Liveness.status -ne 'alive' -or $Readiness.status -ne 'ok') { throw 'Coordinator health endpoints failed.' }

    function Add-QueueTicket([string]$AccountId, [int]$ClaimedRating) {
        $Body = @{
            request_id = "request-$AccountId"
            build_id = 'smoke-build'
            party_id = "party-$AccountId"
            region = 'local'
            variant = 0
            players_per_team = 1
            ranked = $false
            members = @(@{
                account_id = $AccountId
                display_name = $AccountId
                steam_ticket = ''
                party_slot = 0
                rating_snapshot = $ClaimedRating
            })
        } | ConvertTo-Json -Depth 5
        return Invoke-RestMethod -Method Post -Uri "$BaseUrl/v1/matchmaking/tickets" -ContentType 'application/json' -Body $Body
    }

    $TicketOne = Add-QueueTicket 'SmokePlayer1' 0
	$TicketOneDuplicate = Add-QueueTicket 'SmokePlayer1' 0
	if ($TicketOne.ticket_id -ne $TicketOneDuplicate.ticket_id) { throw 'Duplicate queue submission was not idempotent.' }
	# These deliberately conflicting snapshots must still match because only coordinator-owned ratings are authoritative.
    $TicketTwo = Add-QueueTicket 'SmokePlayer2' 3000
    $StatusOne = Invoke-RestMethod -Uri "$BaseUrl/v1/matchmaking/tickets/$($TicketOne.ticket_id)"
    $StatusTwo = Invoke-RestMethod -Uri "$BaseUrl/v1/matchmaking/tickets/$($TicketTwo.ticket_id)"
    if ($StatusOne.status -ne 'allocated' -or $StatusTwo.status -ne 'allocated') { throw 'Tickets were not allocated.' }
    if ($StatusOne.allocation.match_id -ne $StatusTwo.allocation.match_id) { throw 'Players were allocated to different matches.' }

    $Headers = @{ Authorization = 'Bearer flick-local-development-key'; 'X-Flick-Server-Id' = $StatusOne.allocation.server_id }
    function Confirm-Reservation($Reservation, [int]$ExpectedTeam) {
        $Body = @{
            match_id = $StatusOne.allocation.match_id
            account_id = $Reservation.account_id
            reservation_token = $Reservation.token
        } | ConvertTo-Json
        $Verification = Invoke-RestMethod -Method Post -Uri "$BaseUrl/v1/matchmaking/reservations/verify" -Headers $Headers -ContentType 'application/json' -Body $Body
        if (-not $Verification.accepted -or $Verification.team -ne $ExpectedTeam -or $Verification.player_slot -ne 0) { throw 'Reservation verification failed.' }
        return $Body
    }
    $VerifyBody = Confirm-Reservation $StatusOne.allocation.reservations[0] 1
    Confirm-Reservation $StatusTwo.allocation.reservations[0] 2 | Out-Null

    try {
        $BadHeaders = @{ Authorization = 'Bearer definitely-wrong'; 'X-Flick-Server-Id' = $StatusOne.allocation.server_id }
        Invoke-RestMethod -Method Post -Uri "$BaseUrl/v1/matchmaking/reservations/verify" -Headers $BadHeaders -ContentType 'application/json' -Body $VerifyBody | Out-Null
        throw 'An invalid server credential was accepted.'
    } catch {
        if ($_.Exception.Response.StatusCode.value__ -ne 401) { throw }
    }

    $HeartbeatBody = @{
        match_id = $StatusOne.allocation.match_id
        server_id = $StatusOne.allocation.server_id
        unix_time = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds()
    } | ConvertTo-Json
    $Heartbeat = Invoke-RestMethod -Method Post -Uri "$BaseUrl/v1/servers/heartbeat" -Headers $Headers -ContentType 'application/json' -Body $HeartbeatBody
    if (-not $Heartbeat.accepted) { throw 'Server heartbeat failed.' }

    $StartedBody = @{
        match_id = $StatusOne.allocation.match_id
        server_id = $StatusOne.allocation.server_id
        started_unix_time = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds()
    } | ConvertTo-Json
    $Started = Invoke-RestMethod -Method Post -Uri "$BaseUrl/v1/servers/matches/$($StatusOne.allocation.match_id)/started" -Headers $Headers -ContentType 'application/json' -Body $StartedBody
    if (-not $Started.accepted) { throw 'Server start transition failed.' }

    $CompleteBody = @{
        match_id = $StatusOne.allocation.match_id
        server_id = $StatusOne.allocation.server_id
        outcome = 1
        forfeit = $false
        completed_unix_time = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds()
    } | ConvertTo-Json
    $Complete = Invoke-RestMethod -Method Post -Uri "$BaseUrl/v1/servers/matches/$($StatusOne.allocation.match_id)/complete" -Headers $Headers -ContentType 'application/json' -Body $CompleteBody
    if (-not $Complete.accepted) { throw 'Server completion report failed.' }

	function Add-BuildTicket([string]$AccountId, [string]$BuildId) {
		$Body = @{
			request_id = "build-request-$AccountId"
			build_id = $BuildId
			party_id = "build-party-$AccountId"
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
	$OtherBuildOne = Add-BuildTicket 'BuildPlayer1' 'build-a'
	$OtherBuildTwo = Add-BuildTicket 'BuildPlayer2' 'build-b'
	$OtherStatusOne = Invoke-RestMethod -Uri "$BaseUrl/v1/matchmaking/tickets/$($OtherBuildOne.ticket_id)"
	$OtherStatusTwo = Invoke-RestMethod -Uri "$BaseUrl/v1/matchmaking/tickets/$($OtherBuildTwo.ticket_id)"
	if ($OtherStatusOne.status -ne 'searching' -or $OtherStatusTwo.status -ne 'searching') { throw 'Network-incompatible builds were matched.' }
	Invoke-RestMethod -Method Delete -Uri "$BaseUrl/v1/matchmaking/tickets/$($OtherBuildOne.ticket_id)" | Out-Null
	Invoke-RestMethod -Method Delete -Uri "$BaseUrl/v1/matchmaking/tickets/$($OtherBuildTwo.ticket_id)" | Out-Null

    Write-Host "FLICK coordinator smoke test PASS: match $($StatusOne.allocation.match_id)"
} finally {
    if ($Process -and -not $Process.HasExited) { Stop-Process -Id $Process.Id -Force }
}
