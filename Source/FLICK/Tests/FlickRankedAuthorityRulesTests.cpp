#if WITH_DEV_AUTOMATION_TESTS

#include "Core/FlickRankedAuthorityRules.h"
#include "Misc/AutomationTest.h"

namespace
{
	FFlickRankedMatchRequest MakeValidRequest(const int32 PlayersPerTeam)
	{
		FFlickRankedMatchRequest Request;
		Request.MatchId = TEXT("authority-test-match");
		Request.SeasonId = TEXT("SEASON_1");
		Request.Variant = EFlickMatchVariant::Classic;
		Request.PlayersPerTeam = PlayersPerTeam;
		Request.StartedUnixTime = 1;
		for (const EFlickTeam Team : { EFlickTeam::Player1, EFlickTeam::Player2 })
		{
			for (int32 Slot = 0; Slot < PlayersPerTeam; ++Slot)
			{
				FFlickRankedParticipant Participant;
				Participant.AccountId = FString::Printf(
					TEXT("player-%d-%d"),
					static_cast<int32>(Team),
					Slot);
				Participant.Team = Team;
				Participant.PlayerSlot = Slot;
				Participant.Rating = 1000 + Slot * 25;
				Request.Participants.Add(Participant);
			}
		}
		return Request;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickRankedAuthorityRosterTest,
	"FLICK.Ranked.Authority.ValidatesRosterAndPartySpread",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickRankedAuthorityRosterTest::RunTest(const FString& Parameters)
{
	FString Error;
	FFlickRankedMatchRequest Request = MakeValidRequest(3);
	TestTrue(TEXT("A complete unique 3v3 roster is accepted"), FlickRankedAuthorityRules::ValidateMatchRequest(Request, 350, Error));

	Request.Participants[1].AccountId = Request.Participants[0].AccountId;
	TestFalse(TEXT("Duplicate accounts are rejected"), FlickRankedAuthorityRules::ValidateMatchRequest(Request, 350, Error));
	TestTrue(TEXT("Duplicate rejection has a useful error"), !Error.IsEmpty());

	Request = MakeValidRequest(3);
	Request.Participants[2].Rating = 1600;
	TestFalse(TEXT("An excessive party rating spread is rejected"), FlickRankedAuthorityRules::ValidateMatchRequest(Request, 350, Error));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickRankedAuthorityOutcomeTest,
	"FLICK.Ranked.Authority.ResolvesTeamOutcomes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickRankedAuthorityOutcomeTest::RunTest(const FString& Parameters)
{
	const FFlickRankedMatchRequest Request = MakeValidRequest(2);
	TestEqual(
		TEXT("Team one average uses authenticated roster ratings"),
		FlickRankedAuthorityRules::GetAverageTeamRating(Request.Participants, EFlickTeam::Player1),
		1013);
	TestEqual(
		TEXT("Winner receives a win"),
		static_cast<uint8>(FlickRankedAuthorityRules::GetPlayerResult(EFlickTeam::Player1, EFlickMatchOutcome::Player1Wins)),
		static_cast<uint8>(EFlickRankMatchResult::Win));
	TestEqual(
		TEXT("Opponent receives a loss"),
		static_cast<uint8>(FlickRankedAuthorityRules::GetPlayerResult(EFlickTeam::Player2, EFlickMatchOutcome::Player1Wins)),
		static_cast<uint8>(EFlickRankMatchResult::Loss));
	TestEqual(
		TEXT("Both teams receive a draw"),
		static_cast<uint8>(FlickRankedAuthorityRules::GetPlayerResult(EFlickTeam::Player2, EFlickMatchOutcome::Draw)),
		static_cast<uint8>(EFlickRankMatchResult::Draw));
	return true;
}

#endif

