#include "Pieces/FlickPiece.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DemoNetDriver.h"
#include "Game/FlickReplaySubsystem.h"
#include "HAL/IConsoleManager.h"
#include "Core/FlickReplayInterpolation.h"

namespace
{
	TAutoConsoleVariable<float> ReplayPoseDelay(TEXT("flick.Replay.PoseBufferSeconds"), .05f,
		TEXT("Minimum saved replay pose buffer (0-0.2 seconds). Sparse legacy recordings use a longer buffer."));
}

void AFlickPiece::PreReplicationForReplay(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplicationForReplay(ChangedPropertyTracker);
	if (const auto* Driver = GetWorld() ? GetWorld()->GetDemoNetDriver() : nullptr; Driver && Driver->IsRecording())
	{
		SavedReplaySampleTime = Driver->GetDemoCurrentTime();
		// The standard movement codec rounds positions to whole centimetres.
		// Preserve slow rolls in the replay without changing live network data.
		SavedReplayPosition = GetReplicatedMovement().Location;
		SavedReplayRotation = GetReplicatedMovement().Rotation;
		bHasSavedReplayPose = true;
	}
}

void AFlickPiece::OnRep_SavedReplayPose()
{
	if (!GetWorld() || !GetWorld()->IsPlayingReplay() || !bHasSavedReplayPose) return;
	QueueSavedReplayPose(SavedReplaySampleTime,
		FRepMovement::RebaseOntoLocalOrigin(SavedReplayPosition, this), SavedReplayRotation.Quaternion());
}

void AFlickPiece::OnRep_ReplicatedMovement()
{
	if (!GetWorld() || !GetWorld()->IsPlayingReplay())
	{
		Super::OnRep_ReplicatedMovement();
		return;
	}
	if (bHasSavedReplayPose) return;
	if (!IsReplicatingMovement()) return;
	const auto* Driver = GetWorld()->GetDemoNetDriver();
	if (!Driver) return;
	const auto& Movement = GetReplicatedMovement();
	QueueSavedReplayPose(SavedReplaySampleTime >= 0.f ? SavedReplaySampleTime : Driver->GetDemoCurrentTime(),
		FRepMovement::RebaseOntoLocalOrigin(Movement.Location, this), Movement.Rotation.Quaternion());
}

void AFlickPiece::QueueSavedReplayPose(float Time, const FVector& Location, const FQuat& Rotation)
{
	const auto* Replay = GetGameInstance()->GetSubsystem<UFlickReplaySubsystem>();
	const int32 Revision = Replay ? Replay->GetPlaybackRevision() : 0;
	FReplayPoseSample Pose;
	Pose.Time = Time;
	Pose.Location = Location;
	Pose.Rotation = Rotation;
	Pose.Velocity = GetReplicatedMovement().LinearVelocity;
	if(Pose.Velocity.ContainsNaN()) Pose.Velocity=FVector::ZeroVector;
	if (Pose.Location.ContainsNaN() || Pose.Rotation.ContainsNaN()) return;
	if (SavedReplayRevision != Revision || (Replay && Replay->IsSeeking())
		|| (!SavedReplayPoses.IsEmpty() && (Time < SavedReplayPoses.Last().Time
			|| Time - SavedReplayPoses.Last().Time > .25f
			|| FVector::DistSquared(Pose.Location, SavedReplayPoses.Last().Location) > FMath::Square(300.f))))
		SavedReplayPoses.Reset();
	SavedReplayRevision = Revision;
	// Fast-forward can deliver several packets within one playback frame.
	// Retain its final pose rather than a zero-duration interpolation segment.
	if (!SavedReplayPoses.IsEmpty() && FMath::IsNearlyEqual(Time, SavedReplayPoses.Last().Time))
		SavedReplayPoses.Last() = Pose;
	else
	{
		if (SavedReplayPoses.Num() >= 16) SavedReplayPoses.RemoveAt(0, 1, EAllowShrinking::No);
		SavedReplayPoses.Add(Pose);
	}
	// Apply once after packet delivery, in the actor tick. A seek/paused world
	// needs its pose immediately because ordinary actor ticking is suspended.
	if(GetWorld()->IsPaused() || (Replay && Replay->IsSeeking()) || SavedReplayPoses.Num()==1)
		UpdateSavedReplayPose();
}

void AFlickPiece::UpdateSavedReplayPose()
{
	if (!GetWorld() || !GetWorld()->IsPlayingReplay() || !PieceMesh) return;
	// A recorded match is a presentation of authoritative samples, not another
	// collision simulation that repeatedly corrects itself toward those samples.
	if (PieceMesh->IsSimulatingPhysics()) PieceMesh->SetSimulatePhysics(false);
	if (PieceMesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision)
		PieceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (SavedReplayPoses.IsEmpty()) return;
	const auto* Replay = GetGameInstance()->GetSubsystem<UFlickReplaySubsystem>();
	const auto* Driver = GetWorld()->GetDemoNetDriver();
	if (!Driver) return;
	const bool bSnap = GetWorld()->IsPaused() || (Replay && (Replay->IsSeeking()
		|| Replay->GetPlaybackRevision() != SavedReplayRevision));
	const FReplayPoseSample Latest = SavedReplayPoses.Last();
	if (bSnap)
	{
		SavedReplayPoses.Reset(); SavedReplayPoses.Add(Latest);
		SavedReplayRevision = Replay ? Replay->GetPlaybackRevision() : 0;
		SetActorLocationAndRotation(Latest.Location, Latest.Rotation, false, nullptr, ETeleportType::TeleportPhysics);
		return;
	}
	// A 50 ms buffer cannot interpolate across the 125 ms gaps in old 8 Hz
	// files: it catches the newest sample then freezes until another arrives.
	// Cover a complete source interval plus frame-arrival jitter for all pucks.
	const float SourceRate = Replay ? Replay->GetPoseSampleRate() : 8.f;
	const float Delay = FMath::Max(FMath::Clamp(ReplayPoseDelay.GetValueOnGameThread(), 0.f, .2f),
		FMath::Clamp(1.25f / FMath::Max(1.f, SourceRate), .025f, .3f));
	const float Time = Driver->GetDemoCurrentTime() - Delay;
	int32 Expired = 0;
	while (SavedReplayPoses.Num() - Expired > 2 && SavedReplayPoses[Expired + 1].Time <= Time) ++Expired;
	if (Expired) SavedReplayPoses.RemoveAt(0, Expired, EAllowShrinking::No);
	const auto& A = SavedReplayPoses[0];
	const auto& B = SavedReplayPoses.Num() > 1 ? SavedReplayPoses[1] : A;
	const float Alpha = B.Time > A.Time ? FMath::Clamp((Time - A.Time) / (B.Time - A.Time), 0.f, 1.f) : 1.f;
	SetActorLocationAndRotation(FlickReplayInterpolation::Position(A.Location,A.Velocity,B.Location,B.Velocity,B.Time-A.Time,Alpha),
		FQuat::Slerp(A.Rotation, B.Rotation, Alpha), false, nullptr, ETeleportType::TeleportPhysics);
}
