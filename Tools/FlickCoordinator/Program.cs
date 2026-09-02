using System.Collections.Concurrent;
using System.Diagnostics;
using System.Net;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.Json.Serialization;

var builder = WebApplication.CreateBuilder(args);
builder.Logging.AddFilter("Microsoft.AspNetCore", LogLevel.Warning);
builder.Services.ConfigureHttpJsonOptions(options =>
{
    options.SerializerOptions.PropertyNamingPolicy = JsonNamingPolicy.SnakeCaseLower;
    options.SerializerOptions.DictionaryKeyPolicy = JsonNamingPolicy.SnakeCaseLower;
});

var app = builder.Build();
var settings = CoordinatorSettings.FromEnvironment(app.Environment.ContentRootPath);
var configurationErrors = settings.Validate();
if (configurationErrors.Count > 0)
{
    throw new InvalidOperationException("Invalid FLICK coordinator configuration: " + string.Join(" ", configurationErrors));
}
var steamTickets = new SteamTicketVerifier(settings, app.Logger);
var coordinator = new CoordinatorState(settings, steamTickets, app.Logger);

app.MapGet("/health", () => Results.Ok(coordinator.GetHealth()));
app.MapGet("/health/live", () => Results.Ok(new { status = "alive" }));
app.MapGet("/health/ready", () => coordinator.IsReady
    ? Results.Ok(coordinator.GetHealth())
    : Results.Json(coordinator.GetHealth(), statusCode: StatusCodes.Status503ServiceUnavailable));

app.MapPost("/v1/matchmaking/tickets", async (QueueRequest request, CancellationToken cancellationToken) =>
{
    var result = await coordinator.EnqueueAsync(request, cancellationToken);
    return result.Error is null
        ? Results.Accepted($"/v1/matchmaking/tickets/{result.TicketId}", new { ticket_id = result.TicketId, status = "searching" })
        : Results.BadRequest(new { error = result.Error });
});

app.MapGet("/v1/matchmaking/tickets/{ticketId}", (string ticketId) =>
{
    var snapshot = coordinator.GetTicket(ticketId);
    return snapshot is null ? Results.NotFound(new { error = "Matchmaking ticket was not found." }) : Results.Ok(snapshot);
});

app.MapDelete("/v1/matchmaking/tickets/{ticketId}", (string ticketId) =>
{
    return coordinator.Cancel(ticketId)
        ? Results.Ok(new { ticket_id = ticketId, status = "cancelled" })
        : Results.NotFound(new { error = "Matchmaking ticket was not found." });
});

app.MapPost("/v1/matchmaking/reservations/verify", (HttpRequest http, ReservationVerifyRequest request) =>
{
    if (!coordinator.IsServerAuthorized(http, request.MatchId))
    {
        return Results.Json(new { error = "Invalid game-server credential." }, statusCode: StatusCodes.Status401Unauthorized);
    }
    var result = coordinator.VerifyReservation(request);
    return result.Accepted
        ? Results.Ok(result)
        : Results.Json(result, statusCode: StatusCodes.Status403Forbidden);
});

app.MapPost("/v1/servers/heartbeat", (HttpRequest http, ServerHeartbeatRequest request) =>
{
    if (!coordinator.IsServerAuthorized(http, request.MatchId, request.ServerId))
    {
        return Results.Json(new { error = "Invalid game-server credential." }, statusCode: StatusCodes.Status401Unauthorized);
    }
    return coordinator.Heartbeat(request)
        ? Results.Ok(new { accepted = true })
        : Results.NotFound(new { error = "Allocated match was not found." });
});

app.MapPost("/v1/servers/matches/{matchId}/complete", (HttpRequest http, string matchId, ServerMatchCompleteRequest request) =>
{
    if (!coordinator.IsServerAuthorized(http, matchId, request.ServerId))
    {
        return Results.Json(new { error = "Invalid game-server credential." }, statusCode: StatusCodes.Status401Unauthorized);
    }
    return coordinator.CompleteServerMatch(matchId, request)
        ? Results.Ok(new { accepted = true, match_id = matchId })
        : Results.NotFound(new { error = "Allocated match was not found." });
});

app.MapPost("/v1/auth/steam", async (HttpRequest http, RankedAuthRequest request, CancellationToken cancellationToken) =>
{
    if (!coordinator.IsServerAuthorized(http))
    {
        return Results.Json(new { error = "Invalid game-server credential." }, statusCode: StatusCodes.Status401Unauthorized);
    }
    var verification = await steamTickets.VerifyAsync(
        request.SteamTicket,
        request.SteamTicketAudience,
        request.ClaimedAccountId,
        cancellationToken);
    if (!verification.Accepted)
    {
        return Results.Json(new { error = verification.Error }, statusCode: StatusCodes.Status403Forbidden);
    }
    if (!coordinator.IsAccountAllocatedToServer(http.Headers["X-Flick-Server-Id"].ToString(), verification.AccountId))
    {
        return Results.Json(new { error = "The authenticated Steam account is not allocated to this server." }, statusCode: StatusCodes.Status403Forbidden);
    }
    var progress = coordinator.GetProgress(verification.AccountId, request.SeasonId, request.Playlist);
    return Results.Ok(new { authenticated = true, account_id = verification.AccountId, progress });
});

app.MapPost("/v1/ranked/matches", (HttpRequest http, RankedMatchRequest request) =>
{
    if (!coordinator.IsServerAuthorized(http, request.MatchId, request.ServerId))
    {
        return Results.Json(new { error = "Invalid game-server credential." }, statusCode: StatusCodes.Status401Unauthorized);
    }
    var error = coordinator.RegisterRankedMatch(request);
    return error is null
        ? Results.Ok(new { accepted = true, match_id = request.MatchId })
        : Results.Json(new { error }, statusCode: StatusCodes.Status422UnprocessableEntity);
});

app.MapPost("/v1/ranked/matches/{matchId}/result", (HttpRequest http, string matchId, RankedResultRequest request) =>
{
    if (!coordinator.IsServerAuthorized(http, matchId, request.ServerId))
    {
        return Results.Json(new { error = "Invalid game-server credential." }, statusCode: StatusCodes.Status401Unauthorized);
    }
    var settlement = coordinator.SettleRankedMatch(matchId, request);
    return settlement.Error is null
        ? Results.Ok(settlement.Response)
        : Results.Json(new { error = settlement.Error }, statusCode: StatusCodes.Status422UnprocessableEntity);
});

coordinator.StartSupervision(app.Lifetime.ApplicationStopping);
app.Lifetime.ApplicationStopping.Register(coordinator.StopAllServers);
app.Logger.LogInformation(
    "FLICK coordinator ready. Environment={Environment}, AutoLaunch={AutoLaunch}, ServerHost={ServerHost}",
    settings.EnvironmentName,
    settings.AutoLaunchServers,
    settings.GetServerHostPath());
app.Run();

sealed class CoordinatorState
{
    private readonly object gate = new();
    private readonly CoordinatorSettings settings;
    private readonly SteamTicketVerifier steamTickets;
    private readonly ILogger logger;
    private readonly Dictionary<string, QueueTicket> tickets = new(StringComparer.Ordinal);
    private readonly Dictionary<string, MatchAllocationState> matches = new(StringComparer.Ordinal);
    private readonly Dictionary<string, ReservationState> reservations = new(StringComparer.Ordinal);
    private readonly Dictionary<string, RankedMatchRequest> rankedMatches = new(StringComparer.Ordinal);
    private readonly Dictionary<string, RankedSettlementResponse> settledMatches = new(StringComparer.Ordinal);
    private readonly Dictionary<string, PlayerProgress> ratings = new(StringComparer.Ordinal);
    private int nextPort;
    private CancellationTokenSource? supervisionCancellation;
    private Task? supervisionTask;
    private DateTimeOffset lastSupervisionUtc = DateTimeOffset.MinValue;

    public CoordinatorState(CoordinatorSettings settings, SteamTicketVerifier steamTickets, ILogger logger)
    {
        this.settings = settings;
        this.steamTickets = steamTickets;
        this.logger = logger;
        nextPort = settings.FirstServerPort - 1;
        LoadRatings();
    }

    public bool IsReady => settings.Validate().Count == 0;

    public object GetHealth()
    {
        lock (gate)
        {
            return new
            {
                status = "ok",
                queued_parties = tickets.Values.Count(ticket => ticket.Status == "searching"),
                starting_servers = matches.Values.Count(match => !match.Ready && !match.Completed && !match.Failed),
                ready_servers = matches.Values.Count(match => match.Ready && !match.Completed && !match.Failed),
                failed_servers = matches.Values.Count(match => match.Failed),
                auto_launch = settings.AutoLaunchServers,
                production = settings.IsProduction,
                steam_validation = settings.RequireSteamTickets,
                server_launch_available = settings.IsServerLaunchAvailable(),
                last_supervision_unix = lastSupervisionUtc == DateTimeOffset.MinValue ? 0 : lastSupervisionUtc.ToUnixTimeSeconds()
            };
        }
    }

    public async Task<(string? TicketId, string? Error)> EnqueueAsync(QueueRequest request, CancellationToken cancellationToken)
    {
        var error = ValidateQueueRequest(request);
        if (error is not null)
        {
            return (null, error);
        }
        foreach (var member in request.Members)
        {
            var verification = await steamTickets.VerifyAsync(
                member.SteamTicket,
                string.Empty,
                member.AccountId,
                cancellationToken);
            if (!verification.Accepted)
            {
                return (null, $"Steam authentication failed for {member.DisplayName}: {verification.Error}");
            }
        }
        request = request with
        {
            Members = request.Members.Select(member => member with { SteamTicket = string.Empty }).ToList()
        };
        lock (gate)
        {
            var memberIds = request.Members.Select(member => member.AccountId).ToHashSet(StringComparer.Ordinal);
            if (tickets.Values.Any(ticket => ticket.Status == "searching" && ticket.Request.Members.Any(member => memberIds.Contains(member.AccountId))))
            {
                return (null, "A party member already has an active matchmaking ticket.");
            }
            var ticket = new QueueTicket(Guid.NewGuid().ToString("N"), request, DateTimeOffset.UtcNow);
            tickets.Add(ticket.Id, ticket);
            logger.LogInformation(
                "Queue accepted {Ticket}: party={Party}, members={Members}, format={Format}, ranked={Ranked}",
                ticket.Id,
                request.PartyId,
                request.Members.Count,
                $"{request.PlayersPerTeam}v{request.PlayersPerTeam}",
                request.Ranked);
            TryAllocateMatches();
            return (ticket.Id, null);
        }
    }

    public TicketSnapshot? GetTicket(string ticketId)
    {
        lock (gate)
        {
            if (!tickets.TryGetValue(ticketId, out var ticket))
            {
                return null;
            }
            if (ticket.Status == "allocating")
            {
                return new TicketSnapshot(ticket.Id, ticket.Status, null, null);
            }
            return new TicketSnapshot(ticket.Id, ticket.Status, ticket.Allocation, ticket.Error);
        }
    }

    public bool Cancel(string ticketId)
    {
        lock (gate)
        {
            if (!tickets.TryGetValue(ticketId, out var ticket) || ticket.Status != "searching")
            {
                return false;
            }
            ticket.Status = "cancelled";
            return true;
        }
    }

    public bool IsServerAuthorized(HttpRequest request, string? expectedMatchId = null, string? expectedServerId = null)
    {
        var authorization = request.Headers.Authorization.ToString();
        if (!authorization.StartsWith("Bearer ", StringComparison.Ordinal))
        {
            return false;
        }
        var suppliedCredential = authorization[7..];
        var suppliedServerId = request.Headers["X-Flick-Server-Id"].ToString();
        var serverIdentityMatches = false;
        lock (gate)
        {
            var match = matches.Values.FirstOrDefault(item =>
                item.ServerId == suppliedServerId
                && !item.Failed
                && item.CredentialExpiresUtc > DateTimeOffset.UtcNow
                && (expectedMatchId is null || item.MatchId == expectedMatchId));
            serverIdentityMatches = match is not null
                && (expectedServerId is null || match.ServerId == expectedServerId);
            if (match is not null
                && serverIdentityMatches
                && CredentialMatches(suppliedCredential, match.CredentialHash))
            {
                return true;
            }
        }
        return serverIdentityMatches
            && settings.AllowDevelopmentServerKey
            && FixedTimeEquals(suppliedCredential, settings.ServerApiKey);
    }

    public bool IsAccountAllocatedToServer(string serverId, string accountId)
    {
        lock (gate)
        {
            return matches.Values.Any(match =>
                match.ServerId == serverId
                && !match.Completed
                && !match.Failed
                && match.Reservations.Any(reservation => reservation.AccountId == accountId));
        }
    }

    public ReservationVerifyResponse VerifyReservation(ReservationVerifyRequest request)
    {
        lock (gate)
        {
            var reservationFound = reservations.TryGetValue(request.ReservationToken, out var reservation);
            var matchFound = matches.TryGetValue(request.MatchId, out var match);
            var reservationMatches = reservationFound
                && reservation!.MatchId == request.MatchId
                && reservation.AccountId == request.AccountId;
            var reservationCurrent = reservationFound && reservation!.ExpiresUtc >= DateTimeOffset.UtcNow;
            if (!reservationMatches || !reservationCurrent || !matchFound || match!.Completed)
            {
                logger.LogWarning(
                    "Reservation rejected: match={Match} account={Account} token_found={TokenFound} reservation_match={ReservationMatch} current={Current} match_found={MatchFound} completed={Completed}",
                    request.MatchId,
                    request.AccountId,
                    reservationFound,
                    reservationMatches,
                    reservationCurrent,
                    matchFound,
                    matchFound && match!.Completed);
                return new ReservationVerifyResponse(false, request.AccountId, 0, -1, 0, "Reservation is invalid or expired.");
            }
            var verifiedReservation = reservation!;
            verifiedReservation.LastVerifiedUtc = DateTimeOffset.UtcNow;
            verifiedReservation.ReconnectDeadlineUtc = DateTimeOffset.UtcNow.AddSeconds(settings.ReconnectGraceSeconds);
            return new ReservationVerifyResponse(
                true,
                verifiedReservation.AccountId,
                verifiedReservation.Team,
                verifiedReservation.PlayerSlot,
                verifiedReservation.ReconnectDeadlineUtc.ToUnixTimeSeconds(),
                null);
        }
    }

    public bool Heartbeat(ServerHeartbeatRequest request)
    {
        lock (gate)
        {
            var matchFound = matches.TryGetValue(request.MatchId, out var match);
            if (!matchFound || match!.ServerId != request.ServerId || match.Completed)
            {
                logger.LogWarning(
                    "Heartbeat rejected: match={Match} server={Server} match_found={MatchFound} expected_server={ExpectedServer} completed={Completed}",
                    request.MatchId,
                    request.ServerId,
                    matchFound,
                    matchFound ? match!.ServerId : "none",
                    matchFound && match!.Completed);
                return false;
            }
            match.LastHeartbeatUtc = DateTimeOffset.UtcNow;
            if (!match.Ready)
            {
                match.Ready = true;
                match.FirstHeartbeatUtc = match.LastHeartbeatUtc;
                SetMatchTicketsReady(match);
                logger.LogInformation("Server {Server} is ready for match {Match}", match.ServerId, match.MatchId);
            }
            return true;
        }
    }

    public bool CompleteServerMatch(string matchId, ServerMatchCompleteRequest request)
    {
        MatchAllocationState? match;
        lock (gate)
        {
            if (!matches.TryGetValue(matchId, out match) || match.ServerId != request.ServerId)
            {
                return false;
            }
            match.Completed = true;
            match.CompletedUtc = DateTimeOffset.UtcNow;
            match.CredentialExpiresUtc = DateTimeOffset.UtcNow.AddSeconds(120);
        }
        _ = RetireServerAfterDelay(match, TimeSpan.FromSeconds(90));
        return true;
    }

    public PlayerProgress GetProgress(string accountId, string seasonId, string playlist)
    {
        lock (gate)
        {
            return GetOrCreateProgress(accountId, seasonId, playlist).Clone();
        }
    }

    public string? RegisterRankedMatch(RankedMatchRequest request)
    {
        lock (gate)
        {
            if (string.IsNullOrWhiteSpace(request.MatchId) || request.Participants.Count != request.PlayersPerTeam * 2)
            {
                return "The ranked roster is incomplete.";
            }
            if (!matches.TryGetValue(request.MatchId, out var allocation) || allocation.Completed)
            {
                return "The coordinator has no active allocation for this match.";
            }
            var allocatedAccounts = allocation.Reservations.Select(item => item.AccountId).ToHashSet(StringComparer.Ordinal);
            if (request.Participants.Any(participant => !allocatedAccounts.Contains(participant.AccountId)))
            {
                return "The ranked roster differs from the allocated roster.";
            }
            if (rankedMatches.TryGetValue(request.MatchId, out var existing))
            {
                return JsonSerializer.Serialize(existing) == JsonSerializer.Serialize(request)
                    ? null
                    : "The ranked match ID has a conflicting registration.";
            }
            rankedMatches.Add(request.MatchId, request);
            return null;
        }
    }

    public (RankedSettlementResponse? Response, string? Error) SettleRankedMatch(string matchId, RankedResultRequest request)
    {
        lock (gate)
        {
            if (settledMatches.TryGetValue(matchId, out var previous))
            {
                return (previous with { Duplicate = true }, null);
            }
            if (!rankedMatches.TryGetValue(matchId, out var registration) || request.MatchId != matchId)
            {
                return (null, "The ranked match was not registered.");
            }
            if (request.Outcome is < 1 or > 3)
            {
                return (null, "The match outcome is invalid.");
            }

            var oldProgress = registration.Participants.ToDictionary(
                participant => participant.AccountId,
                participant => GetOrCreateProgress(participant.AccountId, registration.SeasonId, registration.Playlist).Clone(),
                StringComparer.Ordinal);
            var updates = new List<RankedPlayerUpdate>();
            foreach (var participant in registration.Participants)
            {
                var progress = oldProgress[participant.AccountId];
                var opponents = registration.Participants.Where(other => other.Team != participant.Team).ToList();
                var opponentRating = (int)Math.Round(opponents.Average(other => oldProgress[other.AccountId].Rating));
                var score = request.Outcome == 3 ? 0.5 : request.Outcome == participant.Team ? 1.0 : 0.0;
                var expected = 1.0 / (1.0 + Math.Pow(10.0, (opponentRating - progress.Rating) / 400.0));
                var delta = (int)Math.Round((progress.MatchesPlayed < 5 ? 56.0 : 32.0) * (score - expected));
                var oldTier = GetTier(progress);
                var oldDivision = GetDivision(progress);
                var updated = GetOrCreateProgress(participant.AccountId, registration.SeasonId, registration.Playlist);
                updated.Rating = Math.Clamp(progress.Rating + delta, 0, 3000);
                updated.MatchesPlayed++;
                if (score > 0.5) updated.Wins++;
                else if (score < 0.5) updated.Losses++;
                else updated.Draws++;
                updates.Add(new RankedPlayerUpdate(
                    participant.AccountId,
                    progress.Rating,
                    updated.Rating,
                    updated.MatchesPlayed,
                    oldTier,
                    GetTier(updated),
                    oldDivision,
                    GetDivision(updated)));
            }
            var response = new RankedSettlementResponse(true, false, matchId, updates);
            settledMatches.Add(matchId, response);
            SaveRatings();
            AppendMatchHistory(registration, request, response);
            return (response, null);
        }
    }

    public void StopAllServers()
    {
        supervisionCancellation?.Cancel();
        lock (gate)
        {
            foreach (var match in matches.Values)
            {
                StopServer(match);
            }
        }
    }

    public void StartSupervision(CancellationToken applicationStopping)
    {
        supervisionCancellation = CancellationTokenSource.CreateLinkedTokenSource(applicationStopping);
        supervisionTask = Task.Run(() => SuperviseServersAsync(supervisionCancellation.Token));
    }

    private async Task SuperviseServersAsync(CancellationToken cancellationToken)
    {
        using var timer = new PeriodicTimer(TimeSpan.FromSeconds(settings.SupervisionIntervalSeconds));
        try
        {
            while (await timer.WaitForNextTickAsync(cancellationToken))
            {
                lock (gate)
                {
                    lastSupervisionUtc = DateTimeOffset.UtcNow;
                    foreach (var match in matches.Values.Where(item => !item.Completed && !item.Failed).ToList())
                    {
                        if (match.Process is not null && HasExited(match.Process))
                        {
                            FailMatch(match, $"Server process exited with code {GetExitCode(match.Process)}.");
                            continue;
                        }
                        if (lastSupervisionUtc >= match.CredentialExpiresUtc)
                        {
                            FailMatch(match, "Server workload credential expired before match completion.");
                            continue;
                        }
                        if (!match.Ready && lastSupervisionUtc >= match.StartupDeadlineUtc)
                        {
                            FailMatch(match, "Server did not become ready before the startup timeout.");
                            continue;
                        }
                        if (match.Ready
                            && lastSupervisionUtc - match.LastHeartbeatUtc > TimeSpan.FromSeconds(settings.HeartbeatTimeoutSeconds))
                        {
                            FailMatch(match, "Server heartbeat timed out.");
                        }
                    }
                }
            }
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
        }
    }

    private string? ValidateQueueRequest(QueueRequest request)
    {
        if (request.PlayersPerTeam is < 1 or > 3 || request.Members.Count is < 1 or > 3
            || request.Members.Count > request.PlayersPerTeam)
        {
            return "The party does not fit the selected team size.";
        }
        if (request.Variant is < 0 or > 2 || string.IsNullOrWhiteSpace(request.PartyId))
        {
            return "The playlist or party identity is invalid.";
        }
        if (request.Members.Any(member => string.IsNullOrWhiteSpace(member.AccountId))
            || request.Members.Select(member => member.AccountId).Distinct(StringComparer.Ordinal).Count() != request.Members.Count)
        {
            return "Every party member needs a unique account identity.";
        }
        if (settings.RequireSteamTickets && request.Members.Any(member => string.IsNullOrWhiteSpace(member.SteamTicket)))
        {
            return "Every party member needs a Steam WebAPI ticket.";
        }
        return null;
    }

    private void TryAllocateMatches()
    {
        var searching = tickets.Values.Where(ticket => ticket.Status == "searching").OrderBy(ticket => ticket.CreatedUtc).ToList();
        foreach (var group in searching.GroupBy(ticket => new
        {
            ticket.Request.Region,
            ticket.Request.Variant,
            ticket.Request.PlayersPerTeam,
            ticket.Request.Ranked
        }))
        {
            var candidates = group.Take(12).ToList();
            var combinations = FindCombinations(candidates, group.Key.PlayersPerTeam);
            foreach (var teamOne in combinations)
            {
                var remaining = candidates.Except(teamOne).ToList();
                var teamTwo = FindCombinations(remaining, group.Key.PlayersPerTeam).FirstOrDefault();
                if (teamTwo is null)
                {
                    continue;
                }
                if (group.Key.Ranked)
                {
                    var oneRating = teamOne.SelectMany(ticket => ticket.Request.Members).Average(member => member.RatingSnapshot);
                    var twoRating = teamTwo.SelectMany(ticket => ticket.Request.Members).Average(member => member.RatingSnapshot);
                    if (Math.Abs(oneRating - twoRating) > settings.MaximumInitialMmrGap)
                    {
                        continue;
                    }
                }
                Allocate(teamOne, teamTwo);
                TryAllocateMatches();
                return;
            }
        }
    }

    private static List<List<QueueTicket>> FindCombinations(IReadOnlyList<QueueTicket> tickets, int targetPlayers)
    {
        var result = new List<List<QueueTicket>>();
        void Search(int index, int count, List<QueueTicket> selected)
        {
            if (count == targetPlayers)
            {
                result.Add(new List<QueueTicket>(selected));
                return;
            }
            if (count > targetPlayers || index >= tickets.Count)
            {
                return;
            }
            for (var cursor = index; cursor < tickets.Count; cursor++)
            {
                selected.Add(tickets[cursor]);
                Search(cursor + 1, count + tickets[cursor].Request.Members.Count, selected);
                selected.RemoveAt(selected.Count - 1);
            }
        }
        Search(0, 0, new List<QueueTicket>());
        return result;
    }

    private void Allocate(IReadOnlyList<QueueTicket> teamOne, IReadOnlyList<QueueTicket> teamTwo)
    {
        var matchId = Guid.NewGuid().ToString("N");
        var serverId = $"local-{matchId[..10]}";
        var port = Interlocked.Increment(ref nextPort);
        var address = $"{settings.ServerPublicHost}:{port}";
        var allReservations = new List<ReservationState>();
        AddTeamReservations(teamOne, 1, matchId, allReservations);
        AddTeamReservations(teamTwo, 2, matchId, allReservations);
        var workloadCredential = CreateToken();
        var allTickets = teamOne.Concat(teamTwo).ToList();
        var match = new MatchAllocationState(
            matchId,
            serverId,
            address,
            port,
            allReservations,
            allTickets.Select(ticket => ticket.Id).ToList(),
            SHA256.HashData(Encoding.UTF8.GetBytes(workloadCredential)),
            DateTimeOffset.UtcNow.AddSeconds(settings.ServerStartupTimeoutSeconds),
            DateTimeOffset.UtcNow.AddSeconds(settings.ServerCredentialLifetimeSeconds));
        matches.Add(matchId, match);
        foreach (var reservation in allReservations)
        {
            reservations.Add(reservation.Token, reservation);
        }
        var sample = allTickets[0].Request;
        if (settings.AutoLaunchServers && !StartServer(match, sample, workloadCredential))
        {
            FailMatch(match, "The coordinator could not launch an Unreal server process.");
            return;
        }
        if (!settings.AutoLaunchServers)
        {
            match.Ready = true;
            match.FirstHeartbeatUtc = DateTimeOffset.UtcNow;
            match.LastHeartbeatUtc = DateTimeOffset.UtcNow;
        }
        foreach (var ticket in allTickets)
        {
            ticket.Status = match.Ready ? "allocated" : "allocating";
            ticket.Allocation = new AllocationResponse(
                matchId,
                serverId,
                address,
                ticket.Request.Variant,
                ticket.Request.PlayersPerTeam,
                ticket.Request.Ranked,
                DateTimeOffset.UtcNow.AddMinutes(10).ToUnixTimeSeconds(),
                allReservations
                    .Where(reservation => ticket.Request.Members.Any(member => member.AccountId == reservation.AccountId))
                    .Select(reservation => new ReservationResponse(
                        reservation.AccountId,
                        reservation.Token,
                        reservation.Team,
                        reservation.PlayerSlot))
                    .ToList());
        }
        logger.LogInformation(
            "Allocated match {Match} to {Server} at {Address}: {Players} players",
            matchId,
            serverId,
            address,
            allReservations.Count);
    }

    private void AddTeamReservations(
        IEnumerable<QueueTicket> teamTickets,
        int team,
        string matchId,
        List<ReservationState> output)
    {
        var playerSlot = 0;
        foreach (var ticket in teamTickets)
        {
            foreach (var member in ticket.Request.Members.OrderBy(member => member.PartySlot))
            {
                output.Add(new ReservationState(
                    matchId,
                    member.AccountId,
                    CreateToken(),
                    team,
                    playerSlot++,
                    DateTimeOffset.UtcNow.AddMinutes(10)));
            }
        }
    }

    private bool StartServer(MatchAllocationState match, QueueRequest request, string workloadCredential)
    {
        if (!settings.IsServerLaunchAvailable())
        {
            logger.LogError("Unreal server launch path is invalid. Host={Host}, Project={Project}", settings.GetServerHostPath(), settings.ProjectPath);
            return false;
        }
        var map = $"/Engine/Maps/Templates/OpenWorld?FlickNetworkMatch?FlickMatchmaking?FlickPlayersPerTeam={request.PlayersPerTeam}?FlickVariant={request.Variant}";
        if (request.Ranked) map += "?FlickRanked";
        map += $"?FlickCoordinatorMatchId={match.MatchId}";
        var packagedServer = !string.IsNullOrWhiteSpace(settings.ServerExecutablePath);
        var start = new ProcessStartInfo(settings.GetServerHostPath())
        {
            UseShellExecute = false,
            CreateNoWindow = true,
            WindowStyle = ProcessWindowStyle.Hidden,
            WorkingDirectory = packagedServer
                ? Path.GetDirectoryName(settings.ServerExecutablePath) ?? Environment.CurrentDirectory
                : Path.GetDirectoryName(settings.ProjectPath) ?? Environment.CurrentDirectory
        };
        if (!packagedServer)
        {
            start.ArgumentList.Add(settings.ProjectPath);
        }
        start.ArgumentList.Add(map);
        if (!packagedServer)
        {
            start.ArgumentList.Add("-server");
        }
        start.ArgumentList.Add("-unattended");
        start.ArgumentList.Add("-NoSplash");
        start.ArgumentList.Add("-NullRHI");
        start.ArgumentList.Add("-nosound");
        if (!settings.EnableSteamOnServers)
        {
            start.ArgumentList.Add("-nosteam");
        }
        start.ArgumentList.Add($"-port={match.Port}");
        start.ArgumentList.Add($"-QueryPort={match.Port + 19000}");
        start.ArgumentList.Add($"-FlickCoordinatorUrl={settings.PublicBaseUrl}");
        start.ArgumentList.Add($"-FlickRankedBackendUrl={settings.PublicBaseUrl}");
        start.ArgumentList.Add($"-FlickServerId={match.ServerId}");
        start.Environment["FLICK_BACKEND_SERVER_KEY"] = workloadCredential;
        start.Environment["FLICK_SERVER_ID"] = match.ServerId;
        try
        {
            match.Process = Process.Start(start);
            return match.Process is not null;
        }
        catch (Exception exception)
        {
            logger.LogError(exception, "Could not launch Unreal authority for match {Match}", match.MatchId);
            return false;
        }
    }

    private void SetMatchTicketsReady(MatchAllocationState match)
    {
        foreach (var ticketId in match.TicketIds)
        {
            if (tickets.TryGetValue(ticketId, out var ticket) && ticket.Status == "allocating")
            {
                ticket.Status = "allocated";
            }
        }
    }

    private void FailMatch(MatchAllocationState match, string reason)
    {
        if (match.Failed || match.Completed)
        {
            return;
        }
        match.Failed = true;
        match.Failure = reason;
        match.Completed = true;
        match.CompletedUtc = DateTimeOffset.UtcNow;
        foreach (var ticketId in match.TicketIds)
        {
            if (tickets.TryGetValue(ticketId, out var ticket))
            {
                ticket.Status = "error";
                ticket.Allocation = null;
                ticket.Error = reason;
            }
        }
        StopServer(match);
        logger.LogError("Allocation {Match} on {Server} failed: {Reason}", match.MatchId, match.ServerId, reason);
    }

    private static bool HasExited(Process process)
    {
        try { return process.HasExited; }
        catch { return true; }
    }

    private static int GetExitCode(Process process)
    {
        try { return process.ExitCode; }
        catch { return -1; }
    }

    private static bool CredentialMatches(string supplied, byte[] expectedHash)
    {
        if (string.IsNullOrEmpty(supplied)) return false;
        var suppliedHash = SHA256.HashData(Encoding.UTF8.GetBytes(supplied));
        return CryptographicOperations.FixedTimeEquals(suppliedHash, expectedHash);
    }

    private static bool FixedTimeEquals(string supplied, string expected)
    {
        if (string.IsNullOrEmpty(supplied) || string.IsNullOrEmpty(expected)) return false;
        return CryptographicOperations.FixedTimeEquals(
            SHA256.HashData(Encoding.UTF8.GetBytes(supplied)),
            SHA256.HashData(Encoding.UTF8.GetBytes(expected)));
    }

    private async Task RetireServerAfterDelay(MatchAllocationState match, TimeSpan delay)
    {
        await Task.Delay(delay);
        lock (gate)
        {
            StopServer(match);
        }
    }

    private void StopServer(MatchAllocationState match)
    {
        try
        {
            if (match.Process is { HasExited: false })
            {
                match.Process.Kill(true);
            }
        }
        catch (Exception exception)
        {
            logger.LogWarning(exception, "Could not retire local server {Server}", match.ServerId);
        }
    }

    private PlayerProgress GetOrCreateProgress(string accountId, string seasonId, string playlist)
    {
        var key = $"{seasonId}|{playlist}|{accountId}";
        if (!ratings.TryGetValue(key, out var progress))
        {
            progress = new PlayerProgress();
            ratings.Add(key, progress);
        }
        return progress;
    }

    private static int GetTier(PlayerProgress progress)
    {
        if (progress.MatchesPlayed < 5) return 0;
        if (progress.Rating < 900) return 1;
        if (progress.Rating < 1050) return 2;
        if (progress.Rating < 1200) return 3;
        if (progress.Rating < 1350) return 4;
        if (progress.Rating < 1500) return 5;
        if (progress.Rating < 1700) return 6;
        return 7;
    }

    private static int GetDivision(PlayerProgress progress)
    {
        var tier = GetTier(progress);
        if (tier == 0) return 0;
        var ranges = new (int Min, int Max)[]
        {
            (0, 899), (900, 1049), (1050, 1199), (1200, 1349),
            (1350, 1499), (1500, 1699), (1700, 3000)
        };
        var range = ranges[tier - 1];
        var alpha = (double)(progress.Rating - range.Min) / Math.Max(1, range.Max - range.Min + 1);
        return Math.Clamp((int)Math.Floor(alpha * 4.0) + 1, 1, 4);
    }

    private void LoadRatings()
    {
        try
        {
            if (File.Exists(settings.RatingsPath))
            {
                var loaded = JsonSerializer.Deserialize<Dictionary<string, PlayerProgress>>(File.ReadAllText(settings.RatingsPath));
                if (loaded is not null)
                {
                    foreach (var item in loaded) ratings[item.Key] = item.Value;
                }
            }
        }
        catch (Exception exception)
        {
            logger.LogWarning(exception, "Could not load local coordinator ratings");
        }
    }

    private void SaveRatings()
    {
        Directory.CreateDirectory(Path.GetDirectoryName(settings.RatingsPath)!);
        File.WriteAllText(settings.RatingsPath, JsonSerializer.Serialize(ratings, new JsonSerializerOptions { WriteIndented = true }));
    }

    private void AppendMatchHistory(RankedMatchRequest match, RankedResultRequest result, RankedSettlementResponse settlement)
    {
        Directory.CreateDirectory(Path.GetDirectoryName(settings.MatchHistoryPath)!);
        File.AppendAllText(
            settings.MatchHistoryPath,
            JsonSerializer.Serialize(new { match, result, settlement, recorded_utc = DateTimeOffset.UtcNow }) + Environment.NewLine);
    }

    private static string CreateToken()
    {
        return Convert.ToBase64String(RandomNumberGenerator.GetBytes(32)).TrimEnd('=').Replace('+', '-').Replace('/', '_');
    }
}

sealed class CoordinatorSettings
{
    public required string EnvironmentName { get; init; }
    public required string ProjectPath { get; init; }
    public required string UnrealEditorCommandPath { get; init; }
    public required string ServerExecutablePath { get; init; }
    public required string ServerPublicHost { get; init; }
    public required string PublicBaseUrl { get; init; }
    public required string ServerApiKey { get; init; }
    public required string SteamWebApiKey { get; init; }
    public required string SteamAppId { get; init; }
    public required string SteamTicketIdentity { get; init; }
    public required string RatingsPath { get; init; }
    public required string MatchHistoryPath { get; init; }
    public bool AutoLaunchServers { get; init; }
    public bool RequireSteamTickets { get; init; }
    public bool EnableSteamOnServers { get; init; }
    public bool AllowDevelopmentServerKey { get; init; }
    public int FirstServerPort { get; init; }
    public int ServerStartupTimeoutSeconds { get; init; }
    public int HeartbeatTimeoutSeconds { get; init; }
    public int SupervisionIntervalSeconds { get; init; }
    public int ServerCredentialLifetimeSeconds { get; init; }
    public int ReconnectGraceSeconds { get; init; }
    public int MaximumInitialMmrGap { get; init; }

    public static CoordinatorSettings FromEnvironment(string contentRoot)
    {
        var repositoryRoot = Path.GetFullPath(Path.Combine(contentRoot, "..", ".."));
        var environmentName = Environment.GetEnvironmentVariable("FLICK_COORDINATOR_ENVIRONMENT") ?? "Development";
        var isProduction = string.Equals(environmentName, "Production", StringComparison.OrdinalIgnoreCase);
        return new CoordinatorSettings
        {
            EnvironmentName = environmentName,
            ProjectPath = Environment.GetEnvironmentVariable("FLICK_PROJECT_PATH")
                ?? Path.Combine(repositoryRoot, "FLICK.uproject"),
            UnrealEditorCommandPath = Environment.GetEnvironmentVariable("FLICK_UNREAL_EDITOR_CMD")
                ?? @"C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor-Cmd.exe",
            ServerExecutablePath = Environment.GetEnvironmentVariable("FLICK_SERVER_EXECUTABLE") ?? string.Empty,
            ServerPublicHost = Environment.GetEnvironmentVariable("FLICK_SERVER_PUBLIC_HOST") ?? "127.0.0.1",
            PublicBaseUrl = Environment.GetEnvironmentVariable("FLICK_COORDINATOR_PUBLIC_URL")
                ?? "http://127.0.0.1:8090",
            ServerApiKey = Environment.GetEnvironmentVariable("FLICK_BACKEND_SERVER_KEY")
                ?? "flick-local-development-key",
            SteamWebApiKey = Environment.GetEnvironmentVariable("FLICK_STEAM_WEB_API_KEY") ?? string.Empty,
            SteamAppId = Environment.GetEnvironmentVariable("FLICK_STEAM_APP_ID") ?? "480",
            SteamTicketIdentity = Environment.GetEnvironmentVariable("FLICK_STEAM_TICKET_IDENTITY") ?? "FLICK",
            RatingsPath = Path.Combine(repositoryRoot, "Saved", "Coordinator", "ratings.json"),
            MatchHistoryPath = Path.Combine(repositoryRoot, "Saved", "Coordinator", "match-history.jsonl"),
            AutoLaunchServers = !string.Equals(Environment.GetEnvironmentVariable("FLICK_COORDINATOR_AUTO_LAUNCH"), "false", StringComparison.OrdinalIgnoreCase),
            RequireSteamTickets = isProduction || string.Equals(Environment.GetEnvironmentVariable("FLICK_COORDINATOR_REQUIRE_STEAM"), "true", StringComparison.OrdinalIgnoreCase),
            EnableSteamOnServers = isProduction || string.Equals(Environment.GetEnvironmentVariable("FLICK_SERVER_ENABLE_STEAM"), "true", StringComparison.OrdinalIgnoreCase),
            AllowDevelopmentServerKey = !isProduction && !string.Equals(Environment.GetEnvironmentVariable("FLICK_ALLOW_DEVELOPMENT_SERVER_KEY"), "false", StringComparison.OrdinalIgnoreCase),
            FirstServerPort = ReadInt("FLICK_COORDINATOR_FIRST_PORT", 7780),
            ServerStartupTimeoutSeconds = ReadInt("FLICK_COORDINATOR_STARTUP_TIMEOUT", 45),
            HeartbeatTimeoutSeconds = ReadInt("FLICK_COORDINATOR_HEARTBEAT_TIMEOUT", 20),
            SupervisionIntervalSeconds = ReadInt("FLICK_COORDINATOR_SUPERVISION_INTERVAL", 2),
            ServerCredentialLifetimeSeconds = ReadInt("FLICK_SERVER_CREDENTIAL_LIFETIME", 7200),
            ReconnectGraceSeconds = ReadInt("FLICK_COORDINATOR_RECONNECT_GRACE", 45),
            MaximumInitialMmrGap = ReadInt("FLICK_COORDINATOR_MMR_GAP", 350)
        };
    }

    public bool IsProduction => string.Equals(EnvironmentName, "Production", StringComparison.OrdinalIgnoreCase);

    public string GetServerHostPath() => string.IsNullOrWhiteSpace(ServerExecutablePath)
        ? UnrealEditorCommandPath
        : ServerExecutablePath;

    public bool IsServerLaunchAvailable()
    {
        if (!AutoLaunchServers) return true;
        if (!string.IsNullOrWhiteSpace(ServerExecutablePath)) return File.Exists(ServerExecutablePath);
        return !IsProduction && File.Exists(UnrealEditorCommandPath) && File.Exists(ProjectPath);
    }

    public List<string> Validate()
    {
        var errors = new List<string>();
        if (!Uri.TryCreate(PublicBaseUrl, UriKind.Absolute, out var publicUri))
            errors.Add("FLICK_COORDINATOR_PUBLIC_URL must be an absolute URL.");
        else if (IsProduction && publicUri.Scheme != Uri.UriSchemeHttps)
            errors.Add("Production requires an HTTPS FLICK_COORDINATOR_PUBLIC_URL.");
        if (RequireSteamTickets && string.IsNullOrWhiteSpace(SteamWebApiKey))
            errors.Add("Steam ticket validation requires FLICK_STEAM_WEB_API_KEY.");
        if (RequireSteamTickets && (!uint.TryParse(SteamAppId, out var appId) || appId == 0))
            errors.Add("Steam ticket validation requires a numeric FLICK_STEAM_APP_ID.");
        else if (IsProduction && SteamAppId == "480")
            errors.Add("Production cannot use Valve's shared test App ID 480.");
        if (RequireSteamTickets && string.IsNullOrWhiteSpace(SteamTicketIdentity))
            errors.Add("Steam ticket validation requires FLICK_STEAM_TICKET_IDENTITY.");
        if (IsProduction && string.IsNullOrWhiteSpace(ServerExecutablePath))
            errors.Add("Production requires a packaged FLICK_SERVER_EXECUTABLE.");
        if (IsProduction && (string.IsNullOrWhiteSpace(ServerPublicHost)
            || ServerPublicHost is "127.0.0.1" or "localhost" or "::1"))
            errors.Add("Production requires a public FLICK_SERVER_PUBLIC_HOST.");
        if (!IsServerLaunchAvailable())
            errors.Add($"The configured server host does not exist: {GetServerHostPath()}.");
        if (ServerStartupTimeoutSeconds < 5 || HeartbeatTimeoutSeconds < 5 || SupervisionIntervalSeconds < 1
            || ServerCredentialLifetimeSeconds < 300)
            errors.Add("Server lifecycle timeouts are below their safe minimums.");
        return errors;
    }

    private static int ReadInt(string name, int fallback)
    {
        return int.TryParse(Environment.GetEnvironmentVariable(name), out var value) ? value : fallback;
    }
}

sealed class QueueTicket(string id, QueueRequest request, DateTimeOffset createdUtc)
{
    public string Id { get; } = id;
    public QueueRequest Request { get; } = request;
    public DateTimeOffset CreatedUtc { get; } = createdUtc;
    public string Status { get; set; } = "searching";
    public AllocationResponse? Allocation { get; set; }
    public string? Error { get; set; }
}

sealed class MatchAllocationState(
    string matchId,
    string serverId,
    string address,
    int port,
    List<ReservationState> reservations,
    List<string> ticketIds,
    byte[] credentialHash,
    DateTimeOffset startupDeadlineUtc,
    DateTimeOffset credentialExpiresUtc)
{
    public string MatchId { get; } = matchId;
    public string ServerId { get; } = serverId;
    public string Address { get; } = address;
    public int Port { get; } = port;
    public List<ReservationState> Reservations { get; } = reservations;
    public List<string> TicketIds { get; } = ticketIds;
    public byte[] CredentialHash { get; } = credentialHash;
    public DateTimeOffset StartupDeadlineUtc { get; } = startupDeadlineUtc;
    public DateTimeOffset CredentialExpiresUtc { get; set; } = credentialExpiresUtc;
    public Process? Process { get; set; }
    public DateTimeOffset LastHeartbeatUtc { get; set; } = DateTimeOffset.UtcNow;
    public DateTimeOffset? FirstHeartbeatUtc { get; set; }
    public DateTimeOffset? CompletedUtc { get; set; }
    public string? Failure { get; set; }
    public bool Ready { get; set; }
    public bool Failed { get; set; }
    public bool Completed { get; set; }
}

sealed class ReservationState(
    string matchId,
    string accountId,
    string token,
    int team,
    int playerSlot,
    DateTimeOffset expiresUtc)
{
    public string MatchId { get; } = matchId;
    public string AccountId { get; } = accountId;
    public string Token { get; } = token;
    public int Team { get; } = team;
    public int PlayerSlot { get; } = playerSlot;
    public DateTimeOffset ExpiresUtc { get; } = expiresUtc;
    public DateTimeOffset LastVerifiedUtc { get; set; }
    public DateTimeOffset ReconnectDeadlineUtc { get; set; }
}

sealed class PlayerProgress
{
    public int Rating { get; set; } = 1000;
    public int MatchesPlayed { get; set; }
    public int Wins { get; set; }
    public int Losses { get; set; }
    public int Draws { get; set; }
    public PlayerProgress Clone() => new()
    {
        Rating = Rating,
        MatchesPlayed = MatchesPlayed,
        Wins = Wins,
        Losses = Losses,
        Draws = Draws
    };
}

sealed class SteamTicketVerifier
{
    private static readonly HttpClient Http = new() { Timeout = TimeSpan.FromSeconds(10) };
    private readonly CoordinatorSettings settings;
    private readonly ILogger logger;
    private readonly ConcurrentDictionary<string, CachedSteamTicket> cache = new(StringComparer.Ordinal);

    public SteamTicketVerifier(CoordinatorSettings settings, ILogger logger)
    {
        this.settings = settings;
        this.logger = logger;
    }

    public async Task<SteamTicketVerification> VerifyAsync(
        string ticket,
        string identity,
        string claimedAccountId,
        CancellationToken cancellationToken)
    {
        if (!settings.RequireSteamTickets)
        {
            return string.IsNullOrWhiteSpace(claimedAccountId)
                ? SteamTicketVerification.Rejected("The account identity is missing.")
                : SteamTicketVerification.AcceptedAs(claimedAccountId);
        }
        if (string.IsNullOrWhiteSpace(ticket))
        {
            return SteamTicketVerification.Rejected("A Steam WebAPI ticket is required.");
        }

        var cacheKey = Convert.ToHexString(SHA256.HashData(Encoding.UTF8.GetBytes(ticket)));
        if (cache.TryGetValue(cacheKey, out var cached) && cached.ExpiresUtc > DateTimeOffset.UtcNow)
        {
            return AccountsMatch(cached.AccountId, claimedAccountId)
                ? SteamTicketVerification.AcceptedAs(cached.AccountId)
                : SteamTicketVerification.Rejected("The Steam ticket does not belong to the claimed account.");
        }

        try
        {
            if (!string.IsNullOrWhiteSpace(identity)
                && !string.Equals(identity, settings.SteamTicketIdentity, StringComparison.Ordinal))
            {
                return SteamTicketVerification.Rejected("The Steam ticket identity is invalid.");
            }
            var fields = new Dictionary<string, string>
            {
                ["key"] = settings.SteamWebApiKey,
                ["appid"] = settings.SteamAppId,
                ["ticket"] = ticket,
                ["identity"] = settings.SteamTicketIdentity
            };
            var query = string.Join("&", fields.Select(field =>
                $"{WebUtility.UrlEncode(field.Key)}={WebUtility.UrlEncode(field.Value)}"));
            using var response = await Http.GetAsync(
                $"https://partner.steam-api.com/ISteamUserAuth/AuthenticateUserTicket/v1/?{query}",
                cancellationToken);
            if (response.StatusCode is HttpStatusCode.Unauthorized or HttpStatusCode.Forbidden)
            {
                logger.LogError("Steam rejected the configured publisher Web API credential ({StatusCode})", (int)response.StatusCode);
                return SteamTicketVerification.Rejected("Steam authentication is temporarily unavailable.");
            }
            if (!response.IsSuccessStatusCode)
            {
                logger.LogWarning("Steam ticket validation failed with HTTP {StatusCode}", (int)response.StatusCode);
                return SteamTicketVerification.Rejected("Steam could not validate the authentication ticket.");
            }

            await using var stream = await response.Content.ReadAsStreamAsync(cancellationToken);
            using var document = await JsonDocument.ParseAsync(stream, cancellationToken: cancellationToken);
            if (!document.RootElement.TryGetProperty("response", out var responseJson)
                || !responseJson.TryGetProperty("params", out var parameters)
                || !parameters.TryGetProperty("result", out var result)
                || !string.Equals(result.GetString(), "OK", StringComparison.OrdinalIgnoreCase)
                || !parameters.TryGetProperty("steamid", out var steamIdJson))
            {
                return SteamTicketVerification.Rejected("Steam rejected the authentication ticket.");
            }
            var accountId = steamIdJson.GetString() ?? string.Empty;
            if (!AccountsMatch(accountId, claimedAccountId))
            {
                return SteamTicketVerification.Rejected("The Steam ticket does not belong to the claimed account.");
            }
            if ((parameters.TryGetProperty("vacbanned", out var vacBanned) && vacBanned.ValueKind == JsonValueKind.True)
                || (parameters.TryGetProperty("publisherbanned", out var publisherBanned) && publisherBanned.ValueKind == JsonValueKind.True))
            {
                return SteamTicketVerification.Rejected("This Steam account is not permitted to join online FLICK matches.");
            }
            cache[cacheKey] = new CachedSteamTicket(accountId, DateTimeOffset.UtcNow.AddMinutes(2));
            if (cache.Count > 2048)
            {
                foreach (var expired in cache.Where(item => item.Value.ExpiresUtc <= DateTimeOffset.UtcNow).Select(item => item.Key))
                    cache.TryRemove(expired, out _);
            }
            return SteamTicketVerification.AcceptedAs(accountId);
        }
        catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
        {
            throw;
        }
        catch (Exception exception)
        {
            logger.LogWarning("Steam ticket validation request failed ({ExceptionType})", exception.GetType().Name);
            return SteamTicketVerification.Rejected("Steam authentication is temporarily unavailable.");
        }
    }

    private static bool AccountsMatch(string authenticated, string claimed) =>
        !string.IsNullOrWhiteSpace(authenticated)
        && string.Equals(authenticated, claimed, StringComparison.Ordinal);
}

sealed record CachedSteamTicket(string AccountId, DateTimeOffset ExpiresUtc);
sealed record SteamTicketVerification(bool Accepted, string AccountId, string? Error)
{
    public static SteamTicketVerification AcceptedAs(string accountId) => new(true, accountId, null);
    public static SteamTicketVerification Rejected(string error) => new(false, string.Empty, error);
}

sealed record QueueRequest(string PartyId, string Region, int Variant, int PlayersPerTeam, bool Ranked, List<QueueMember> Members);
sealed record QueueMember(string AccountId, string DisplayName, string SteamTicket, int PartySlot, int RatingSnapshot);
sealed record TicketSnapshot(string TicketId, string Status, AllocationResponse? Allocation, string? Error);
sealed record AllocationResponse(string MatchId, string ServerId, string Address, int Variant, int PlayersPerTeam, bool Ranked, long ExpiresUnix, List<ReservationResponse> Reservations);
sealed record ReservationResponse(string AccountId, string Token, int Team, int PlayerSlot);
sealed record ReservationVerifyRequest(string MatchId, string AccountId, string ReservationToken);
sealed record ReservationVerifyResponse(bool Accepted, string AccountId, int Team, int PlayerSlot, long ReconnectDeadlineUnix, string? Error);
sealed record ServerHeartbeatRequest(string MatchId, string ServerId, long UnixTime);
sealed record ServerMatchCompleteRequest(string MatchId, string ServerId, int Outcome, bool Forfeit, long CompletedUnixTime);
sealed record RankedAuthRequest(string ClaimedAccountId, string SteamTicket, string SteamTicketAudience, string SeasonId, string Playlist);
sealed record RankedParticipant(string AccountId, int Team, int PlayerSlot, int AuthenticatedRating);
sealed record RankedMatchRequest(string MatchId, string ServerId, string SeasonId, string Playlist, int Variant, int PlayersPerTeam, long StartedUnixTime, List<RankedParticipant> Participants);
sealed record RankedResultRequest(string MatchId, string ServerId, string SeasonId, string Playlist, int Variant, int PlayersPerTeam, long StartedUnixTime, List<RankedParticipant> Participants, int Outcome, bool Forfeit, long CompletedUnixTime);
sealed record RankedPlayerUpdate(string AccountId, int OldRating, int NewRating, int MatchesPlayed, int OldTier, int NewTier, int OldDivision, int NewDivision);
sealed record RankedSettlementResponse(bool Accepted, bool Duplicate, string MatchId, List<RankedPlayerUpdate> Updates);
