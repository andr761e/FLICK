#pragma once

#include "CoreMinimal.h"
#include "Core/FlickPracticeCatalog.h"

namespace FlickPracticeStyle
{
	constexpr float ContentWidth = 1504;
	constexpr float ContentHeight = 824;
	constexpr float HeaderHeight = 150;
	constexpr float DetailWidth = 456;
	constexpr float DetailPadding = 26;
	constexpr float ColumnGap = 22;
	constexpr float CardGap = 16;
	constexpr float DifficultyHeight = 72;
	constexpr float StartHeight = 82;
	constexpr float FooterHeight = 74;
	constexpr float CardWidth = (ContentWidth-DetailWidth-ColumnGap-CardGap)/2;

	inline const TCHAR* FocusDescription(int32 Category)
	{
		static const TCHAR* Text[] = {
			TEXT("Land your entire puck inside\nthe marked circle."),
			TEXT("Eliminate the marked opponent\nwhile keeping your own pucks safe."),
			TEXT("Cross the marked switch.\nMaster approaches without\nleaving the arena."),
			TEXT("Pocket the marked puck\nwithout losing your striker.\nLearn cut angles.")};
		return Text[FMath::Clamp(Category,0,FlickPracticeCatalog::CategoryCount-1)];
	}
	inline const TCHAR* DifficultyDescription(int32 Category, int32 Difficulty)
	{
		static const TCHAR* Text[4][3] = {
			{TEXT("Learn touch, distance and shot power."),TEXT("Sharpen your accuracy and control."),TEXT("Master smaller targets and longer shots.")},
			{TEXT("Practice contact and safe knockouts."),TEXT("Judge angles. Keep friendly pucks safe."),TEXT("Eliminate targets without collateral.")},
			{TEXT("Find a controlled switch approach."),TEXT("Refine longer and angled approaches."),TEXT("Activate exactly one switch.")},
			{TEXT("Learn contact and pocket angles."),TEXT("Refine cuts and protect your striker."),TEXT("Sink tougher cuts without a striker foul.")}};
		return Text[FMath::Clamp(Category,0,3)][FMath::Clamp(Difficulty,0,2)];
	}
}
