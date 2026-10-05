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
$env:FLICK_COORDINATOR_RATINGS_PATH = Join-Path $LogDirectory 'postmatch-smoke-ratings.json'
[IO.File]::WriteAllText($env:FLICK_COORDINATOR_RATINGS_PATH, '{}')
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

    function Add-QueueTicket([string]$AccountId, [int]$ClaimedRating, [bool]$Ranked = $false) {
        $Body = @{
            request_id = "request-$AccountId"
            build_id = 'smoke-build'
            party_id = "party-$AccountId"
            region = 'local'
            variant = 0
            players_per_team = 1
            ranked = $Ranked
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


    $NextMatchId = [guid]::NewGuid().ToString()
    $RematchBody = @{ server_id = $StatusOne.allocation.server_id; new_match_id = $NextMatchId } | ConvertTo-Json
    $RematchUrl = "$BaseUrl/v1/servers/matches/$($StatusOne.allocation.match_id)/rematch"
    $Rematch = Invoke-RestMethod -Method Post -Uri $RematchUrl -Headers $Headers -ContentType 'application/json' -Body $RematchBody
    if (-not $Rematch.accepted -or $Rematch.match_id -ne $NextMatchId) { throw 'Rematch allocation failed.' }
    $Duplicate = Invoke-RestMethod -Method Post -Uri $RematchUrl -Headers $Headers -ContentType 'application/json' -Body $RematchBody
    if ($Duplicate.match_id -ne $NextMatchId) { throw 'Rematch retry was not idempotent.' }
    foreach ($Previous in @($StatusOne.allocation.reservations[0], $StatusTwo.allocation.reservations[0])) {
        $Verify = @{ match_id = $NextMatchId; account_id = $Previous.account_id; reservation_token = $Previous.token } | ConvertTo-Json
        $Verified = Invoke-RestMethod -Method Post -Uri "$BaseUrl/v1/matchmaking/reservations/verify" -Headers $Headers -ContentType 'application/json' -Body $Verify
        if (-not $Verified.accepted -or $Verified.team -ne $Previous.team -or $Verified.player_slot -ne $Previous.player_slot) { throw 'Rematch changed a reserved player seat.' }
    }
    $NextStartedBody = @{ match_id = $NextMatchId; server_id = $StatusOne.allocation.server_id; started_unix_time = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds() } | ConvertTo-Json
    Invoke-RestMethod -Method Post -Uri "$BaseUrl/v1/servers/matches/$NextMatchId/started" -Headers $Headers -ContentType 'application/json' -Body $NextStartedBody | Out-Null
    $NextCompleteBody = @{ match_id = $NextMatchId; server_id = $StatusOne.allocation.server_id; outcome = 2; forfeit = $false; completed_unix_time = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds() } | ConvertTo-Json
    Invoke-RestMethod -Method Post -Uri "$BaseUrl/v1/servers/matches/$NextMatchId/complete" -Headers $Headers -ContentType 'application/json' -Body $NextCompleteBody | Out-Null
    Write-Host 'PASS: same-server rematch retained seats and completed with a fresh match ID.'


    # Competitive rematches require settlement and register a new authoritative roster.
    $RankedOne = Add-QueueTicket 'RematchRankedOne' 1000 $true
    $RankedTwo = Add-QueueTicket 'RematchRankedTwo' 1000 $true
    $RankedStatus = Invoke-RestMethod -Uri "$BaseUrl/v1/matchmaking/tickets/$($RankedOne.ticket_id)"
    $RankedOther = Invoke-RestMethod -Uri "$BaseUrl/v1/matchmaking/tickets/$($RankedTwo.ticket_id)"
    $RankedAllocation = $RankedStatus.allocation
    $RankedHeaders = @{ Authorization = 'Bearer flick-local-development-key'; 'X-Flick-Server-Id' = $RankedAllocation.server_id }
    $Roster = @()
    foreach ($Reservation in @($RankedAllocation.reservations[0], $RankedOther.allocation.reservations[0])) {
        $Verify = @{ match_id = $RankedAllocation.match_id; account_id = $Reservation.account_id; reservation_token = $Reservation.token } | ConvertTo-Json
        Invoke-RestMethod -Method Post -Uri "$BaseUrl/v1/matchmaking/reservations/verify" -Headers $RankedHeaders -ContentType 'application/json' -Body $Verify | Out-Null
        $Roster += @{ account_id = $Reservation.account_id; team = $Reservation.team; player_slot = $Reservation.player_slot; authenticated_rating = 1000 }
    }
    $RankedMatch = @{ match_id = $RankedAllocation.match_id; server_id = $RankedAllocation.server_id; season_id = 'PRESEASON'; playlist = 'ranked-0-1'; variant = 0; players_per_team = 1; started_unix_time = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds(); participants = $Roster }
    Invoke-RestMethod -Method Post -Uri "$BaseUrl/v1/ranked/matches" -Headers $RankedHeaders -ContentType 'application/json' -Body ($RankedMatch | ConvertTo-Json -Depth 5) | Out-Null
    $Started = @{ match_id = $RankedMatch.match_id; server_id = $RankedMatch.server_id; started_unix_time = $RankedMatch.started_unix_time } | ConvertTo-Json
    Invoke-RestMethod -Method Post -Uri "$BaseUrl/v1/servers/matches/$($RankedMatch.match_id)/started" -Headers $RankedHeaders -ContentType 'application/json' -Body $Started | Out-Null
    $RankedResult = $RankedMatch.Clone()
    $RankedResult.outcome = 1; $RankedResult.forfeit = $false; $RankedResult.completed_unix_time = [DateTimeOffset]::UtcNow.ToUnixTimeSeconds()
    # Result and completion requests can arrive in either order.
    Invoke-RestMethod -Method Post -Uri "$BaseUrl/v1/ranked/matches/$($RankedMatch.match_id)/result" -Headers $RankedHeaders -ContentType 'application/json' -Body ($RankedResult | ConvertTo-Json -Depth 5) | Out-Null
    Invoke-RestMethod -Method Post -Uri "$BaseUrl/v1/servers/matches/$($RankedMatch.match_id)/complete" -Headers $RankedHeaders -ContentType 'application/json' -Body ($RankedResult | ConvertTo-Json -Depth 5) | Out-Null
    $NextRankedId = [guid]::NewGuid().ToString()
    $NextRankedBody = @{ server_id = $RankedMatch.server_id; new_match_id = $NextRankedId } | ConvertTo-Json
    Invoke-RestMethod -Method Post -Uri "$BaseUrl/v1/servers/matches/$($RankedMatch.match_id)/rematch" -Headers $RankedHeaders -ContentType 'application/json' -Body $NextRankedBody | Out-Null
    $RankedMatch.match_id = $NextRankedId
    Invoke-RestMethod -Method Post -Uri "$BaseUrl/v1/ranked/matches" -Headers $RankedHeaders -ContentType 'application/json' -Body ($RankedMatch | ConvertTo-Json -Depth 5) | Out-Null
    $StartedNext = @{ match_id = $NextRankedId; server_id = $RankedMatch.server_id; started_unix_time = $RankedMatch.started_unix_time } | ConvertTo-Json
    Invoke-RestMethod -Method Post -Uri "$BaseUrl/v1/servers/matches/$NextRankedId/started" -Headers $RankedHeaders -ContentType 'application/json' -Body $StartedNext | Out-Null
    $RankedResult.match_id = $NextRankedId
    $RankedResult.outcome = 2
    Invoke-RestMethod -Method Post -Uri "$BaseUrl/v1/servers/matches/$NextRankedId/complete" -Headers $RankedHeaders -ContentType 'application/json' -Body ($RankedResult | ConvertTo-Json -Depth 5) | Out-Null
    $ThirdId = [guid]::NewGuid().ToString()
    $ThirdBody = @{ server_id = $RankedMatch.server_id; new_match_id = $ThirdId } | ConvertTo-Json
    try {
        Invoke-RestMethod -Method Post -Uri "$BaseUrl/v1/servers/matches/$NextRankedId/rematch" -Headers $RankedHeaders -ContentType 'application/json' -Body $ThirdBody | Out-Null
        throw 'Competitive rematch started before the result settled.'
    } catch {
        if ($_.Exception.Response.StatusCode.value__ -ne 409) { throw }
    }
    Invoke-RestMethod -Method Post -Uri "$BaseUrl/v1/ranked/matches/$NextRankedId/result" -Headers $RankedHeaders -ContentType 'application/json' -Body ($RankedResult | ConvertTo-Json -Depth 5) | Out-Null
    Invoke-RestMethod -Method Post -Uri "$BaseUrl/v1/servers/matches/$NextRankedId/rematch" -Headers $RankedHeaders -ContentType 'application/json' -Body $ThirdBody | Out-Null
    Write-Host 'PASS: competitive result/completion ordering and fresh rematch registration.'

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
