#pragma once

#include "UI/FlickUITheme.h"

namespace FlickPlaylistStyle
{
	// Filled, symmetric arrow silhouette, independent of font substitution.
	inline TArray<FVector2D> ArrowOutline(const FVector2D Size)
	{
		TArray<FVector2D> Points = {{0,.46},{.72,.46},{.43,.17},{.50,.10},
			{.90,.5},{.50,.90},{.43,.83},{.72,.54},{0,.54}};
		for (auto& P : Points) P *= Size;
		return Points;
	}
	// TArray::Add can reallocate. Never pass a reference to one of its own
	// elements when closing a polyline (this asserts in Development builds).
	inline TArray<FVector2D> GetClosedOutline(const FVector2D Size, const float Cut)
	{
		auto Outline = FlickUITheme::GetPanelOutline(Size, Cut);
		const FVector2D First = Outline[0];
		Outline.Add(First);
		return Outline;
	}
}
