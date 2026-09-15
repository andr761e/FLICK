#include "Core/FlickLineupRules.h"

#include "Core/FlickPieceArchetypeRules.h"

bool FlickLineupRules::IsValid(const TArray<EFlickPieceArchetype>& Lineup)
{
	if (Lineup.Num() != PiecesPerLineup)
	{
		return false;
	}
	TSet<EFlickPieceArchetype> UniquePieces;
	for (const EFlickPieceArchetype Archetype : Lineup)
	{
		const int32 Value = static_cast<int32>(Archetype);
		if (Value < 0 || Value >= FlickPieceArchetypeRules::ArchetypeCount)
		{
			return false;
		}
		UniquePieces.Add(Archetype);
	}
	return UniquePieces.Num() == PiecesPerLineup;
}
