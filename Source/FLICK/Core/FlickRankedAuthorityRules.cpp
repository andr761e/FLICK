#include "Core/FlickRankedAuthorityRules.h"

#include "Core/FlickTeamRules.h"

bool FlickRankedAuthorityRules::ValidateMatchRequest(
	const FFlickRankedMatchRequest& Request,
	const int32 MaximumTeamRatingSpread,
	FString& OutError)
{
	OutError.Reset();
	if (Request.MatchId.IsEmpty() || Request.MatchId.Len() > 96)
	{
		OutError = TEXT("The ranked match ID is missing or invalid.");
		return false;
	}
	if (Request.SeasonId.IsEmpty() || Request.SeasonId.Len() > 64)
	{
		OutError = TEXT("The ranked season ID is missing or invalid.");
		return false;
	}
	const int32 PlayersPerTeam = FlickTeamRules::ClampPlayersPerTeam(Request.PlayersPerTeam);
	if (PlayersPerTeam != Request.PlayersPerTeam
		|| Request.Participants.Num() != PlayersPerTeam * 2)
	{
		OutError = TEXT("The ranked roster does not match the playlist team size.");
		return false;
	}

	TSet<FString> AccountIds;
	for (const EFlickTeam Team : { EFlickTeam::Player1, EFlickTeam::Player2 })
	{
		TSet<int32> PlayerSlots;
		int32 MinimumRating = FlickRankRules::MaximumRating;
		int32 MaximumRating = FlickRankRules::MinimumRating;
		int32 TeamCount = 0;
		for (const FFlickRankedParticipant& Participant : Request.Participants)
		{
			if (Participant.Team != Team)
			{
				continue;
			}
			if (Participant.AccountId.IsEmpty() || Participant.AccountId.Len() > 128
				|| AccountIds.Contains(Participant.AccountId))
			{
				OutError = TEXT("The ranked roster contains a missing or duplicate account.");
				return false;
			}
			if (Participant.PlayerSlot < 0 || Participant.PlayerSlot >= PlayersPerTeam
				|| PlayerSlots.Contains(Participant.PlayerSlot))
			{
				OutError = TEXT("The ranked roster contains an invalid or duplicate team slot.");
				return false;
			}
			if (Participant.Rating < FlickRankRules::MinimumRating
				|| Participant.Rating > FlickRankRules::MaximumRating)
			{
				OutError = TEXT("The ranked roster contains an invalid rating.");
				return false;
			}
			AccountIds.Add(Participant.AccountId);
			PlayerSlots.Add(Participant.PlayerSlot);
			MinimumRating = FMath::Min(MinimumRating, Participant.Rating);
			MaximumRating = FMath::Max(MaximumRating, Participant.Rating);
			++TeamCount;
		}
		if (TeamCount != PlayersPerTeam)
		{
			OutError = TEXT("Each ranked team must contain the full expected roster.");
			return false;
		}
		if (MaximumTeamRatingSpread >= 0
			&& MaximumRating - MinimumRating > MaximumTeamRatingSpread)
		{
			OutError = FString::Printf(
				TEXT("A party exceeds the allowed %d MMR team spread."),
				MaximumTeamRatingSpread);
			return false;
		}
	}
	return true;
}

int32 FlickRankedAuthorityRules::GetAverageTeamRating(
	const TArray<FFlickRankedParticipant>& Participants,
	const EFlickTeam Team)
{
	int32 TotalRating = 0;
	int32 Count = 0;
	for (const FFlickRankedParticipant& Participant : Participants)
	{
		if (Participant.Team == Team)
		{
			TotalRating += Participant.Rating;
			++Count;
		}
	}
	return Count > 0
		? FMath::RoundToInt(static_cast<float>(TotalRating) / Count)
		: FlickRankRules::DefaultRating;
}

EFlickRankMatchResult FlickRankedAuthorityRules::GetPlayerResult(
	const EFlickTeam Team,
	const EFlickMatchOutcome Outcome)
{
	if (Outcome == EFlickMatchOutcome::Draw)
	{
		return EFlickRankMatchResult::Draw;
	}
	const bool bWon = (Team == EFlickTeam::Player1 && Outcome == EFlickMatchOutcome::Player1Wins)
		|| (Team == EFlickTeam::Player2 && Outcome == EFlickMatchOutcome::Player2Wins);
	return bWon ? EFlickRankMatchResult::Win : EFlickRankMatchResult::Loss;
}

