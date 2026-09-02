#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Utility/FlickLaunchMath.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFlickLaunchMathTest,
	"FLICK.LaunchMath.BasicShotCalculation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFlickLaunchMathTest::RunTest(const FString& Parameters)
{
	const FVector PieceLocation(0.0f, 0.0f, 50.0f);
	const float MaxDragDistance = 100.0f;
	const float MinDragDistance = 10.0f;
	const float PowerExponent = 1.0f;

	{
		const FFlickLaunchResult Result = FlickLaunchMath::CalculateLaunch(
			PieceLocation,
			PieceLocation,
			MaxDragDistance,
			MinDragDistance,
			PowerExponent);
		TestFalse(TEXT("Zero drag is invalid"), Result.bValidShot);
	}

	{
		const FFlickLaunchResult Result = FlickLaunchMath::CalculateLaunch(
			PieceLocation,
			FVector(-5.0f, 0.0f, 200.0f),
			MaxDragDistance,
			MinDragDistance,
			PowerExponent);
		TestFalse(TEXT("Drag below minimum is invalid"), Result.bValidShot);
	}

	{
		const FFlickLaunchResult Result = FlickLaunchMath::CalculateLaunch(
			PieceLocation,
			FVector(-100.0f, 0.0f, -100.0f),
			MaxDragDistance,
			MinDragDistance,
			PowerExponent);
		TestTrue(TEXT("Max drag is valid"), Result.bValidShot);
		TestEqual(TEXT("Max drag clamps to full power"), Result.NormalizedPower, 1.0f);
		TestTrue(TEXT("Direction is normalized"), FMath::IsNearlyEqual(Result.Direction.Size(), 1.0f));
		TestTrue(TEXT("Pulling left launches right"), Result.Direction.Equals(FVector::XAxisVector, KINDA_SMALL_NUMBER));
	}

	{
		const FFlickLaunchResult Result = FlickLaunchMath::CalculateLaunch(
			PieceLocation,
			FVector(-1000.0f, 0.0f, 200.0f),
			MaxDragDistance,
			MinDragDistance,
			PowerExponent);
		TestTrue(TEXT("Beyond max drag is valid"), Result.bValidShot);
		TestEqual(TEXT("Beyond max drag remains full power"), Result.NormalizedPower, 1.0f);
	}

	{
		const FFlickLaunchResult Result = FlickLaunchMath::CalculateLaunch(
			PieceLocation,
			FVector(0.0f, -50.0f, -999.0f),
			MaxDragDistance,
			MinDragDistance,
			PowerExponent);
		TestTrue(TEXT("Z is ignored for valid drag"), Result.bValidShot);
		TestTrue(TEXT("Pulling down launches up in world Y"), Result.Direction.Equals(FVector::YAxisVector, KINDA_SMALL_NUMBER));
	}

	{
		const FFlickLaunchResult Result = FlickLaunchMath::CalculateLaunch(
			PieceLocation,
			FVector(-50.0f, 0.0f, PieceLocation.Z),
			MaxDragDistance,
			MinDragDistance,
			2.0f);
		TestTrue(TEXT("Half drag with nonlinear curve is valid"), Result.bValidShot);
		TestTrue(TEXT("Power exponent gives low-power precision"), FMath::IsNearlyEqual(Result.NormalizedPower, 0.25f));
	}

	{
		const FFlickLaunchResult Result = FlickLaunchMath::CalculateLaunch(
			PieceLocation,
			FVector(-50.0f, 0.0f, PieceLocation.Z),
			0.0f,
			MinDragDistance,
			PowerExponent);
		TestFalse(TEXT("Zero maximum drag is safely rejected"), Result.bValidShot);
	}

	return true;
}

#endif
