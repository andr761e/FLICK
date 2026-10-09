#pragma once
#include "CoreMinimal.h"

namespace FlickPracticeCatalog
{
	constexpr int32 CategoryCount = 4;
	constexpr int32 DifficultyCount = 3;
	constexpr int32 ChallengesPerRun = 5;
	bool IsValid(int32 Category, int32 Difficulty);
	const TCHAR* Name(int32 Category);
	const TCHAR* Description(int32 Category);
	const TCHAR* DifficultyName(int32 Difficulty);
	int32 BestScore(int32 Category, int32 Difficulty);
	void RecordScore(int32 Category, int32 Difficulty, int32 Score);
	int32 PerfectPacks();
}
