$ErrorActionPreference = 'Stop'

$Root = Split-Path -Parent $PSScriptRoot
$Dll = Join-Path $PSScriptRoot 'FlickCoordinator\bin\Release\net8.0\FlickCoordinator.dll'
$BaseUrl = 'http://127.0.0.1:8093'
$RatingPath = Join-Path $Root 'Saved\Coordinator\match-quality-test-ratings.json'
New-Item -ItemType Directory -Force -Path (Split-Path -Parent $RatingPath) | Out-Null
@{
    'PRESEASON|ranked-0-1|QualityLow' = @{ Rating = 1000; MatchesPlayed = 10; Wins = 5; Losses = 5; Draws = 0 }
    'PRESEASON|ranked-0-1|QualityHigh' = @{ Rating = 1200; MatchesPlayed = 10; Wins = 5; Losses = 5; Draws = 0 }
} | ConvertTo-Json -Depth 4 | Set-Content -Path $RatingPath

$env:FLICK_COORDINATOR_AUTO_LAUNCH = 'false'
$env:FLICK_COORDINATOR_PUBLIC_URL = $BaseUrl
$env:FLICK_COORDINATOR_RATINGS_PATH = $RatingPath
$env:FLICK_RANKED_INITIAL_MMR_GAP = '50'
$env:FLICK_RANKED_MMR_EXPANSION_STEP = '200'
$env:FLICK_RANKED_MMR_EXPANSION_SECONDS = '1'
$env:FLICK_COORDINATOR_SUPERVISION_INTERVAL = '1'

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
            request_id = "quality-$AccountId"
            build_id = 'quality-build'
            party_id = "quality-party-$AccountId"
            region = 'local'
            variant = 0
            players_per_team = 1
            ranked = $true
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

    $Low = Add-QueueTicket 'QualityLow'
    $High = Add-QueueTicket 'QualityHigh'
    $Initial = Invoke-RestMethod -Uri "$BaseUrl/v1/matchmaking/tickets/$($Low.ticket_id)"
    if ($Initial.status -ne 'searching' -or $Initial.rating_tolerance -ne 50) {
        throw 'Ranked players outside the initial skill window matched too early.'
    }

    $Allocated = $false
    for ($Attempt = 0; $Attempt -lt 20; $Attempt++) {
        Start-Sleep -Milliseconds 250
        $LowStatus = Invoke-RestMethod -Uri "$BaseUrl/v1/matchmaking/tickets/$($Low.ticket_id)"
        $HighStatus = Invoke-RestMethod -Uri "$BaseUrl/v1/matchmaking/tickets/$($High.ticket_id)"
        if ($LowStatus.status -eq 'allocated' -and $HighStatus.status -eq 'allocated') {
            $Allocated = $true
            break
        }
    }
    if (-not $Allocated) { throw 'Expanded ranked skill window did not form a match.' }
    if ($LowStatus.allocation.match_id -ne $HighStatus.allocation.match_id) { throw 'Expanded tickets received different matches.' }

    Write-Host 'FLICK match-quality test PASS: narrow initial search expanded and matched canonical ratings.'
} finally {
    if ($Process -and -not $Process.HasExited) { Stop-Process -Id $Process.Id -Force }
    if (Test-Path -LiteralPath $RatingPath) { Remove-Item -LiteralPath $RatingPath -Force }
}
