#include "Core/FlickBotShotPlanner.h"

namespace
{
	float DistanceToSegment(const FVector2D& Point, const FVector2D& Start, const FVector2D& End)
	{
		const FVector2D Segment = End - Start;
		const float LengthSquared = Segment.SizeSquared();
		if (LengthSquared <= KINDA_SMALL_NUMBER)
		{
			return FVector2D::Distance(Point, Start);
		}
		const float Alpha = FMath::Clamp(FVector2D::DotProduct(Point - Start, Segment) / LengthSquared, 0.0f, 1.0f);
		return FVector2D::Distance(Point, Start + Segment * Alpha);
	}

	bool DoesSegmentHitDivider(
		const FVector2D& Start,
		const FVector2D& End,
		const FFlickBotDividerState& Divider,
		const float Clearance)
	{
		FVector2D Tangent = Divider.Tangent.GetSafeNormal();
		if (Tangent.IsNearlyZero())
		{
			Tangent = FVector2D(1.0f, 0.0f);
		}
		const FVector2D Normal(-Tangent.Y, Tangent.X);
		const FVector2D LocalStart(
			FVector2D::DotProduct(Start - Divider.Center, Tangent),
			FVector2D::DotProduct(Start - Divider.Center, Normal));
		const FVector2D LocalEnd(
			FVector2D::DotProduct(End - Divider.Center, Tangent),
			FVector2D::DotProduct(End - Divider.Center, Normal));
		const FVector2D Delta = LocalEnd - LocalStart;
		const float ExtentX = FMath::Max(1.0f, Divider.HalfLength + Clearance);
		const float ExtentY = FMath::Max(1.0f, Divider.HalfThickness + Clearance);
		float EntryTime = 0.0f;
		float ExitTime = 1.0f;
		const auto ClipAxis = [&EntryTime, &ExitTime](const float Origin, const float Direction, const float Extent)
		{
			if (FMath::Abs(Direction) <= KINDA_SMALL_NUMBER)
			{
				return FMath::Abs(Origin) <= Extent;
			}
			float First = (-Extent - Origin) / Direction;
			float Second = (Extent - Origin) / Direction;
			if (First > Second)
			{
				Swap(First, Second);
			}
			EntryTime = FMath::Max(EntryTime, First);
			ExitTime = FMath::Min(ExitTime, Second);
			return EntryTime <= ExitTime;
		};
		return ClipAxis(LocalStart.X, Delta.X, ExtentX)
			&& ClipAxis(LocalStart.Y, Delta.Y, ExtentY);
	}

	float GetDividerPathPenalty(
		const FVector2D& Start,
		const FVector2D& End,
		const float Clearance,
		const TArray<FFlickBotDividerState>& Dividers,
		const int32 IgnoredDivider = INDEX_NONE)
	{
		float Penalty = 0.0f;
		for (int32 Index = 0; Index < Dividers.Num(); ++Index)
		{
			if (Index != IgnoredDivider && Dividers[Index].bRaised
				&& DoesSegmentHitDivider(Start, End, Dividers[Index], Clearance))
			{
				Penalty += 1.0f;
			}
		}
		return Penalty;
	}

	float GetSwitchDeploymentRisk(
		const FVector2D& Start,
		const FVector2D& End,
		const float PieceRadius,
		const TArray<FFlickBotDividerState>& Dividers)
	{
		const FVector2D Path = End - Start;
		const float PathLength = Path.Size();
		if (PathLength <= KINDA_SMALL_NUMBER)
		{
			return 0.0f;
		}
		const FVector2D Direction = Path / PathLength;
		float Risk = 0.0f;
		for (const FFlickBotDividerState& Divider : Dividers)
		{
			if (Divider.bRaised
				|| DistanceToSegment(Divider.SwitchPosition, Start, End) > PieceRadius + 12.0f)
			{
				continue;
			}
			const float SwitchProgress = FVector2D::DotProduct(Divider.SwitchPosition - Start, Direction);
			const float DividerProgress = FVector2D::DotProduct(Divider.Center - Start, Direction);
			if (SwitchProgress > 0.0f && DividerProgress > SwitchProgress
				&& DoesSegmentHitDivider(Start, End, Divider, PieceRadius))
			{
				Risk += 1.0f;
			}
		}
		return Risk;
	}

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

	bool BuildBankDirection(
		const FFlickBotPieceState& Shooter,
		const FFlickBotPieceState& Target,
		const FFlickBotDividerState& Divider,
		FVector2D& OutDirection,
		FVector2D& OutBouncePoint,
		float& OutPathDistance)
	{
		FVector2D Tangent = Divider.Tangent.GetSafeNormal();
		if (!Divider.bRaised || Tangent.IsNearlyZero())
		{
			return false;
		}
		FVector2D Normal(-Tangent.Y, Tangent.X);
		const float ShooterSide = FVector2D::DotProduct(Shooter.Position - Divider.Center, Normal);
		if (FMath::IsNearlyZero(ShooterSide))
		{
			return false;
		}
		Normal *= FMath::Sign(ShooterSide);
		const FVector2D BouncePlaneCenter = Divider.Center
			+ Normal * (Divider.HalfThickness + Shooter.Radius);
		if (FVector2D::DotProduct(Target.Position - BouncePlaneCenter, Normal) <= 0.0f)
		{
			return false;
		}

		const FVector2D MirroredTarget = Target.Position
			- Normal * (2.0f * FVector2D::DotProduct(Target.Position - BouncePlaneCenter, Normal));
		const FVector2D ToMirroredTarget = MirroredTarget - Shooter.Position;
		const float Denominator = FVector2D::DotProduct(ToMirroredTarget, Normal);
		if (FMath::Abs(Denominator) <= KINDA_SMALL_NUMBER)
		{
			return false;
		}
		const float HitTime = FVector2D::DotProduct(BouncePlaneCenter - Shooter.Position, Normal) / Denominator;
		if (HitTime <= 0.06f || HitTime >= 0.94f)
		{
			return false;
		}

		OutBouncePoint = Shooter.Position + ToMirroredTarget * HitTime;
		if (FMath::Abs(FVector2D::DotProduct(OutBouncePoint - Divider.Center, Tangent))
			> FMath::Max(0.0f, Divider.HalfLength - Shooter.Radius * 0.3f))
		{
			return false;
		}
		OutDirection = (OutBouncePoint - Shooter.Position).GetSafeNormal();
		OutPathDistance = FVector2D::Distance(Shooter.Position, OutBouncePoint)
			+ FVector2D::Distance(OutBouncePoint, Target.Position);
		return !OutDirection.IsNearlyZero();
	}

	void ConsiderCandidate(
		FFlickBotShotPlan& BestPlan,
		const FFlickBotPieceState& Shooter,
		const FFlickBotPieceState& Target,
		const FVector2D& Direction,
		const float TargetEdgeProgress,
		const float DistanceFraction,
		const float CandidateScore,
		const FFlickBotShotTuning& Tuning,
		FRandomStream& RandomStream)
	{
		const float Score = CandidateScore
			+ RandomStream.FRandRange(-Tuning.DecisionNoise, Tuning.DecisionNoise);
		if (Score <= BestPlan.Score)
		{
			return;
		}

		const float EdgePower = (1.0f - TargetEdgeProgress) * 0.16f;
		const float DistancePower = DistanceFraction * 0.34f;
		const float Power = FMath::Clamp(
			Tuning.MinimumPower + EdgePower + DistancePower
				+ RandomStream.FRandRange(-Tuning.PowerVariation, Tuning.PowerVariation),
			Tuning.MinimumPower,
			Tuning.MaximumPower);
		const float AimRadians = FMath::DegreesToRadians(
			RandomStream.FRandRange(-Tuning.AimErrorDegrees, Tuning.AimErrorDegrees));
		const float CosAngle = FMath::Cos(AimRadians);
		const float SinAngle = FMath::Sin(AimRadians);

		BestPlan.ShooterPieceId = Shooter.PieceId;
		BestPlan.TargetPieceId = Target.PieceId;
		BestPlan.Direction = FVector2D(
			Direction.X * CosAngle - Direction.Y * SinAngle,
			Direction.X * SinAngle + Direction.Y * CosAngle);
		BestPlan.NormalizedPower = Power;
		BestPlan.Score = Score;
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
			const FVector2D ExitEnd = Target.Position + ShotDirection * SafeArenaRadius * 2.1f;
			const float ApproachDividerPenalty = GetDividerPathPenalty(
				Shooter.Position, Target.Position, Shooter.Radius, Tuning.Dividers);
			const float ExitDividerPenalty = GetDividerPathPenalty(
				Target.Position, ExitEnd, Target.Radius, Tuning.Dividers);
			const float SwitchRisk = GetSwitchDeploymentRisk(
				Shooter.Position, ExitEnd, Shooter.Radius, Tuning.Dividers);
			const float Score = OutwardAlignment * 2.45f
				+ TargetEdgeProgress * 1.35f
				- DistanceFraction * 0.42f
				- BlockerPenalty * 1.7f
				- ApproachDividerPenalty * 5.2f * Tuning.DividerAwareness
				- ExitDividerPenalty * 2.1f * Tuning.DividerAwareness
				- SwitchRisk * 3.2f * Tuning.DividerAwareness;
			ConsiderCandidate(
				BestPlan, Shooter, Target, ShotDirection, TargetEdgeProgress,
				DistanceFraction, Score, Tuning, RandomStream);

			if (Tuning.BankShotSkill <= KINDA_SMALL_NUMBER)
			{
				continue;
			}
			for (int32 DividerIndex = 0; DividerIndex < Tuning.Dividers.Num(); ++DividerIndex)
			{
				FVector2D BankDirection;
				FVector2D BouncePoint;
				float BankDistance = 0.0f;
				if (!BuildBankDirection(
					Shooter, Target, Tuning.Dividers[DividerIndex],
					BankDirection, BouncePoint, BankDistance))
				{
					continue;
				}
				const float OtherDividerPenalty = GetDividerPathPenalty(
					Shooter.Position, BouncePoint, Shooter.Radius, Tuning.Dividers, DividerIndex)
					+ GetDividerPathPenalty(
						BouncePoint, Target.Position, Shooter.Radius, Tuning.Dividers, DividerIndex);
				if (OtherDividerPenalty > KINDA_SMALL_NUMBER)
				{
					continue;
				}
				const FVector2D FinalDirection = (Target.Position - BouncePoint).GetSafeNormal();
				const float BankOutwardAlignment = FMath::Clamp(
					(FVector2D::DotProduct(FinalDirection, TargetOutwardDirection) + 1.0f) * 0.5f,
					0.0f,
					1.0f);
				const float BankDistanceFraction = FMath::Clamp(
					BankDistance / (SafeArenaRadius * 2.5f), 0.0f, 1.0f);
				const float BankBlockerPenalty = GetBlockerPenalty(
					Shooter, Target, BankDirection,
					FVector2D::Distance(Shooter.Position, BouncePoint), Pieces, BotTeam);
				const float BankScore = BankOutwardAlignment * 2.45f
					+ TargetEdgeProgress * 1.35f
					- BankDistanceFraction * 0.62f
					- BankBlockerPenalty * 1.7f
					+ ApproachDividerPenalty * 4.4f * Tuning.BankShotSkill
					- (1.0f - Tuning.BankShotSkill) * 2.4f;
				ConsiderCandidate(
					BestPlan, Shooter, Target, BankDirection, TargetEdgeProgress,
					BankDistanceFraction, BankScore, Tuning, RandomStream);
			}
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
