#include "Core/FlickBotShotPlanner.h"

namespace
{
	float GetBlockerPenalty(
		const FFlickBotPieceState& Shooter,
		const FFlickBotPieceState& Target,
		const FVector2D& ShotDirection,
		const float ShotDistance,
		const TArray<FFlickBotPieceState>& Pieces,
		const EFlickTeam BotTeam)
	{
		float Penalty = 0.0f;
		for (const FFlickBotPieceState& Other : Pieces)
		{
			if (Other.PieceId == Shooter.PieceId || Other.PieceId == Target.PieceId)
			{
				continue;
			}

			const FVector2D ToOther = Other.Position - Shooter.Position;
			const float DistanceAlongShot = FVector2D::DotProduct(ToOther, ShotDirection);
			if (DistanceAlongShot <= Shooter.Radius * 0.35f
				|| DistanceAlongShot >= ShotDistance - Target.Radius * 0.35f)
			{
				continue;
			}

			const float PerpendicularDistance = FMath::Abs(
				ToOther.X * ShotDirection.Y - ToOther.Y * ShotDirection.X);
			const float Clearance = FMath::Max(Shooter.Radius + Other.Radius + 8.0f, 1.0f);
			if (PerpendicularDistance >= Clearance)
			{
				continue;
			}

			const float Occlusion = 1.0f - PerpendicularDistance / Clearance;
			Penalty += Occlusion * (Other.Team == BotTeam ? 1.35f : 0.8f);
		}
		return Penalty;
	}
}

FFlickBotShotPlan FlickBotShotPlanner::PlanShot(
	const TArray<FFlickBotPieceState>& Pieces,
	const EFlickTeam BotTeam,
	const FFlickBotShotTuning& Tuning,
	FRandomStream& RandomStream)
{
	FFlickBotShotPlan BestPlan;
	if (BotTeam == EFlickTeam::None)
	{
		return BestPlan;
	}

	const EFlickTeam OpponentTeam = GetOpposingTeam(BotTeam);
	const float SafeArenaRadius = FMath::Max(Tuning.ArenaRadius, 1.0f);
	for (const FFlickBotPieceState& Shooter : Pieces)
	{
		if (Shooter.Team != BotTeam)
		{
			continue;
		}

		for (const FFlickBotPieceState& Target : Pieces)
		{
			if (Target.Team != OpponentTeam)
			{
				continue;
			}

			const FVector2D ShooterToTarget = Target.Position - Shooter.Position;
			const float ShotDistance = ShooterToTarget.Size();
			if (ShotDistance <= KINDA_SMALL_NUMBER)
			{
				continue;
			}

			const FVector2D ShotDirection = ShooterToTarget / ShotDistance;
			FVector2D TargetOutwardDirection = Target.Position.GetSafeNormal();
			if (TargetOutwardDirection.IsNearlyZero())
			{
				TargetOutwardDirection = ShotDirection;
			}

			const float OutwardAlignment = FMath::Clamp(
				(FVector2D::DotProduct(ShotDirection, TargetOutwardDirection) + 1.0f) * 0.5f,
				0.0f,
				1.0f);
			const float TargetEdgeProgress = FMath::Clamp(Target.Position.Size() / SafeArenaRadius, 0.0f, 1.0f);
			const float DistanceFraction = FMath::Clamp(ShotDistance / (SafeArenaRadius * 2.0f), 0.0f, 1.0f);
			const float BlockerPenalty = GetBlockerPenalty(
				Shooter,
				Target,
				ShotDirection,
				ShotDistance,
				Pieces,
				BotTeam);
			const float TieBreakNoise = RandomStream.FRandRange(-Tuning.DecisionNoise, Tuning.DecisionNoise);
			const float Score = OutwardAlignment * 2.45f
				+ TargetEdgeProgress * 1.35f
				- DistanceFraction * 0.42f
				- BlockerPenalty * 1.7f
				+ TieBreakNoise;
			if (Score <= BestPlan.Score)
			{
				continue;
			}

			const float EdgePower = (1.0f - TargetEdgeProgress) * 0.16f;
			const float DistancePower = DistanceFraction * 0.34f;
			const float Power = FMath::Clamp(
				Tuning.MinimumPower + EdgePower + DistancePower
					+ RandomStream.FRandRange(-Tuning.PowerVariation, Tuning.PowerVariation),
				Tuning.MinimumPower,
				Tuning.MaximumPower);
			const float AimError = RandomStream.FRandRange(-Tuning.AimErrorDegrees, Tuning.AimErrorDegrees);
			const float AimRadians = FMath::DegreesToRadians(AimError);
			const float CosAngle = FMath::Cos(AimRadians);
			const float SinAngle = FMath::Sin(AimRadians);

			BestPlan.ShooterPieceId = Shooter.PieceId;
			BestPlan.TargetPieceId = Target.PieceId;
			BestPlan.Direction = FVector2D(
				ShotDirection.X * CosAngle - ShotDirection.Y * SinAngle,
				ShotDirection.X * SinAngle + ShotDirection.Y * CosAngle);
			BestPlan.NormalizedPower = Power;
			BestPlan.Score = Score;
		}
	}

	return BestPlan;
}

FFlickBotShotPlan FlickBotShotPlanner::PlanBobShot(
	const TArray<FFlickBotPieceState>& Pieces,
	const EFlickTeam BotTeam,
	const int32 StrikerPieceId,
	const TArray<FVector2D>& PocketPositions,
	const FFlickBotShotTuning& Tuning,
	FRandomStream& RandomStream)
{
	FFlickBotShotPlan BestPlan;
	if (BotTeam == EFlickTeam::None || StrikerPieceId == INDEX_NONE || PocketPositions.IsEmpty())
	{
		return BestPlan;
	}

	const FFlickBotPieceState* Striker = Pieces.FindByPredicate(
		[BotTeam, StrikerPieceId](const FFlickBotPieceState& Piece)
		{
			return Piece.PieceId == StrikerPieceId && Piece.Team == BotTeam;
		});
	if (!Striker)
	{
		return BestPlan;
	}

	const float SafeArenaRadius = FMath::Max(Tuning.ArenaRadius, 1.0f);
	for (const FFlickBotPieceState& Target : Pieces)
	{
		// BOB is scored by pocketing your own color. The dedicated striker is
		// the only legal shooter and must never be selected as an objective.
		if (Target.Team != BotTeam || Target.PieceId == StrikerPieceId)
		{
			continue;
		}

		for (const FVector2D& Pocket : PocketPositions)
		{
			const FVector2D TargetToPocket = Pocket - Target.Position;
			const float PocketDistance = TargetToPocket.Size();
			if (PocketDistance <= KINDA_SMALL_NUMBER)
			{
				continue;
			}

			const FVector2D PocketDirection = TargetToPocket / PocketDistance;
			const FVector2D ContactPoint = Target.Position
				- PocketDirection * (Striker->Radius + Target.Radius);
			const FVector2D StrikerToContact = ContactPoint - Striker->Position;
			const float ShotDistance = StrikerToContact.Size();
			if (ShotDistance <= KINDA_SMALL_NUMBER)
			{
				continue;
			}

			const FVector2D ShotDirection = StrikerToContact / ShotDistance;
			const FVector2D StrikerToTarget = (Target.Position - Striker->Position).GetSafeNormal();
			const float ApproachAlignment = FVector2D::DotProduct(StrikerToTarget, PocketDirection);
			const float ShotDistanceFraction = FMath::Clamp(ShotDistance / (SafeArenaRadius * 2.0f), 0.0f, 1.0f);
			const float PocketDistanceFraction = FMath::Clamp(PocketDistance / (SafeArenaRadius * 2.0f), 0.0f, 1.0f);
			const float BlockerPenalty = GetBlockerPenalty(
				*Striker,
				Target,
				ShotDirection,
				ShotDistance,
				Pieces,
				BotTeam);
			const float Score = ApproachAlignment * 3.0f
				- ShotDistanceFraction * 0.55f
				- PocketDistanceFraction * 0.9f
				- BlockerPenalty * 1.4f
				+ RandomStream.FRandRange(-Tuning.DecisionNoise, Tuning.DecisionNoise);
			if (Score <= BestPlan.Score)
			{
				continue;
			}

			const float Power = FMath::Clamp(
				Tuning.MinimumPower
					+ ShotDistanceFraction * 0.22f
					+ PocketDistanceFraction * 0.22f
					+ RandomStream.FRandRange(-Tuning.PowerVariation, Tuning.PowerVariation),
				Tuning.MinimumPower,
				Tuning.MaximumPower);
			const float AimRadians = FMath::DegreesToRadians(
				RandomStream.FRandRange(-Tuning.AimErrorDegrees, Tuning.AimErrorDegrees));
			const float CosAngle = FMath::Cos(AimRadians);
			const float SinAngle = FMath::Sin(AimRadians);

			BestPlan.ShooterPieceId = StrikerPieceId;
			BestPlan.TargetPieceId = Target.PieceId;
			BestPlan.Direction = FVector2D(
				ShotDirection.X * CosAngle - ShotDirection.Y * SinAngle,
				ShotDirection.X * SinAngle + ShotDirection.Y * CosAngle);
			BestPlan.NormalizedPower = Power;
			BestPlan.Score = Score;
		}
	}

	return BestPlan;
}
