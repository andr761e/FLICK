#if WITH_DEV_AUTOMATION_TESTS

#include "Audio/FlickSoundSynthesis.h"
#include "Audio.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickSoundSynthesisTest, "FLICK.Audio.Synthesis",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickSoundSynthesisTest::RunTest(const FString& Parameters)
{
	struct FCue { EFlickGeneratedSoundKind Kind; const TCHAR* Name; float Duration; float Frequency; };
	const FCue Cues[] = {
		{EFlickGeneratedSoundKind::UiNavigate, TEXT("01-navigate"), 0.055f, 740.0f},
		{EFlickGeneratedSoundKind::UiConfirm, TEXT("02-confirm"), 0.18f, 587.33f},
		{EFlickGeneratedSoundKind::Launch, TEXT("03-launch"), 0.30f, 170.0f},
		{EFlickGeneratedSoundKind::Impact, TEXT("04-puck-impact"), 0.26f, 95.0f},
		{EFlickGeneratedSoundKind::RimImpact, TEXT("05-rim-impact"), 0.42f, 245.0f},
		{EFlickGeneratedSoundKind::RingOut, TEXT("06-ring-out"), 0.56f, 220.0f},
		{EFlickGeneratedSoundKind::Turn, TEXT("07-turn"), 0.30f, 587.33f},
		{EFlickGeneratedSoundKind::RoundWin, TEXT("08-round-win"), 0.78f, 587.33f},
		{EFlickGeneratedSoundKind::MatchWin, TEXT("09-match-win"), 1.40f, 587.33f},
		{EFlickGeneratedSoundKind::ReplayMusic, TEXT("10-replay-music"), 3.0f, 110.0f}
	};
	const bool bExport = FParse::Param(FCommandLine::Get(), TEXT("FlickExportSoundReview"));
	const FString ReviewDirectory = FPaths::ProjectSavedDir() / TEXT("AudioReview");
	if (bExport) IFileManager::Get().MakeDirectory(*ReviewDirectory, true);
	for (const FCue& Cue : Cues)
	{
		for (const float Timbre : {0.0f, 0.5f, 1.0f})
		{
			for (const float Intensity : {0.0f, 0.5f, 1.0f})
			{
				const TArray<int16> Samples = FlickSoundSynthesis::Render(Cue.Kind, Cue.Duration, Cue.Frequency, 173, Timbre, Intensity);
				TestEqual(TEXT("Requested sample length"), Samples.Num(), FMath::RoundToInt(Cue.Duration * FlickSoundSynthesis::SampleRate));
				if (Samples.IsEmpty()) continue;
				TestEqual(TEXT("Silent first sample prevents onset click"), Samples[0], static_cast<int16>(0));
				TestEqual(TEXT("Silent last sample prevents cutoff click"), Samples.Last(), static_cast<int16>(0));
				int32 Peak = 0;
				double Energy = 0.0;
				for (const int16 Sample : Samples)
				{
					Peak = FMath::Max(Peak, FMath::Abs(static_cast<int32>(Sample)));
					Energy += static_cast<double>(Sample) * Sample;
				}
				TestTrue(FString::Printf(TEXT("%s has audible signal and headroom"), Cue.Name),
					FMath::Sqrt(Energy / Samples.Num()) > 100.0 && Peak < 28000);
				TestTrue(TEXT("Rendering is deterministic"), Samples == FlickSoundSynthesis::Render(
					Cue.Kind, Cue.Duration, Cue.Frequency, 173, Timbre, Intensity));
				if (bExport && Timbre == 0.5f && Intensity == 1.0f)
				{
					TArray<uint8> Wave;
					SerializeWaveFile(Wave, reinterpret_cast<const uint8*>(Samples.GetData()),
						Samples.Num() * sizeof(int16), 1, FlickSoundSynthesis::SampleRate);
					TestTrue(TEXT("Review WAV saved"), FFileHelper::SaveArrayToFile(Wave, *(ReviewDirectory / (FString(Cue.Name) + TEXT(".wav")))));
				}
			}
		}
	}
	TestTrue(TEXT("Invalid duration rejected"), FlickSoundSynthesis::Render(EFlickGeneratedSoundKind::Impact, -1.0f, 220.0f, 1).IsEmpty());
	TestTrue(TEXT("Impact timbre responds to piece material"),
		FlickSoundSynthesis::Render(EFlickGeneratedSoundKind::Impact, 0.26f, 140.0f, 1, 0.0f)
		!= FlickSoundSynthesis::Render(EFlickGeneratedSoundKind::Impact, 0.26f, 140.0f, 1, 1.0f));
	return true;
}

#endif
