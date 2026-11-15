#if WITH_DEV_AUTOMATION_TESTS
#include "Core/FlickReplayInterpolation.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFlickReplayInterpolationTest,"FLICK.Replays.MotionInterpolation",
	EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FFlickReplayInterpolationTest::RunTest(const FString&)
{
	using FlickReplayInterpolation::Position;
	const FVector A(0,10,250),B(10,0,250),V(100,-100,0);
	TestTrue(TEXT("Uniform recorded velocity remains uniform"),Position(A,V,B,V,.1f,.25f).Equals(FMath::Lerp(A,B,.25f),.001));
	TestTrue(TEXT("Sample endpoints retained"),Position(A,V,B,V,.1f,1).Equals(B,.001));
	// A continuously decelerating trajectory is quadratic; endpoint positions
	// and velocities must reproduce its middle instead of a linear speed jump.
	TestTrue(TEXT("Recorded deceleration is smooth"),Position(FVector::ZeroVector,FVector(20,0,0),FVector(15,0,0),FVector(10,0,0),1,.5f).Equals(FVector(8.75,0,0),.001));
	for(int32 I=0;I<=100;++I)
	{
		const FVector P=Position(A,FVector(10000,10000,10000),B,FVector(-10000,-10000,-10000),.125f,I/100.f);
		if(!TestTrue(TEXT("Collision tangents cannot overshoot the recorded bounds"),P.X>=0&&P.X<=10&&P.Y>=0&&P.Y<=10&&P.Z==250)) break;
	}
	TestTrue(TEXT("Absent velocities retain linear fallback"),Position(A,FVector::ZeroVector,B,FVector::ZeroVector,.1f,.5f).Equals((A+B)*.5,.001));
	return true;
}
#endif
