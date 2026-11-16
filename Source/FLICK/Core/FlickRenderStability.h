#pragma once

namespace FlickRenderStability
{
	// Console controls allow immediate comparison/rollback without editing a
	// player's lighting preferences. Zero restores the previous presentation.
	bool IsEnabled();
	bool PauseStableSampling();
	float ReflectionQuality();
	float FinalGatherQuality();
}
