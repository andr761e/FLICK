#include "Core/FlickPracticeCatalog.h"
#include "Misc/ConfigCacheIni.h"

namespace FlickPracticeCatalog
{
	static const TCHAR* Section = TEXT("FLICK.PracticeScores.v1");
	static FString Key(int32 Category, int32 Difficulty) { return FString::Printf(TEXT("Pack%d_Level%d"), Category, Difficulty); }
	bool IsValid(int32 Category, int32 Difficulty) { return Category >= 0 && Category < CategoryCount && Difficulty >= 0 && Difficulty < DifficultyCount; }
	const TCHAR* Name(int32 Category)
	{
		static const TCHAR* Names[] = {TEXT("PRECISION"), TEXT("KNOCKOUTS"), TEXT("SWITCH CONTROL"), TEXT("BOB POCKETS")};
		return Names[FMath::Clamp(Category, 0, CategoryCount - 1)];
	}
	const TCHAR* Description(int32 Category)
	{
		static const TCHAR* Text[] = {
			TEXT("Land your entire puck inside the marked circle. Learn touch, distance and shot power."),
			TEXT("Eliminate the marked opponent while keeping your own pucks safe."),
			TEXT("Cross the marked switch. Master approaches without leaving the arena."),
			TEXT("Pocket the marked puck without losing your striker. Learn cut angles.")};
		return Text[FMath::Clamp(Category, 0, CategoryCount - 1)];
	}
	const TCHAR* DifficultyName(int32 Difficulty)
	{
		static const TCHAR* Names[] = {TEXT("FOUNDATION"), TEXT("SKILLED"), TEXT("EXPERT")};
		return Names[FMath::Clamp(Difficulty, 0, DifficultyCount - 1)];
	}
	int32 BestScore(int32 Category, int32 Difficulty)
	{
		int32 Score = 0;
		if (GConfig && IsValid(Category, Difficulty)) GConfig->GetInt(Section, *Key(Category, Difficulty), Score, GGameUserSettingsIni);
		return FMath::Clamp(Score, 0, ChallengesPerRun);
	}
	void RecordScore(int32 Category, int32 Difficulty, int32 Score)
	{
		if (!GConfig || !IsValid(Category, Difficulty)) return;
		Score = FMath::Clamp(Score, 0, ChallengesPerRun);
		if (Score <= BestScore(Category, Difficulty)) return;
		GConfig->SetInt(Section, *Key(Category, Difficulty), Score, GGameUserSettingsIni);
		GConfig->Flush(false, GGameUserSettingsIni);
	}
	int32 PerfectPacks()
	{
		int32 Count = 0;
		for (int32 Category = 0; Category < CategoryCount; ++Category)
			for (int32 Difficulty = 0; Difficulty < DifficultyCount; ++Difficulty)
				Count += BestScore(Category, Difficulty) == ChallengesPerRun;
		return Count;
	}
}
