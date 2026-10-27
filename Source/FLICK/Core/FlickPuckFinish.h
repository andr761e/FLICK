#pragma once

// Visual energy budget only. Lower LED energy preserves saturated skin colours
// through the filmic tone mapper instead of flattening them into pastel white.
namespace FlickPuckFinish
{
	// The approved real-time locker look, shared by Knockout play and replays.
	constexpr float ReferenceExposureBias = -.45f;
	constexpr float ReferenceBloomIntensity = .32f;
	constexpr float ReferenceBloomThreshold = 1.35f;
	constexpr float IdleEmission = 3.2f;
	constexpr float HighlightEmission = 4.4f;
	constexpr float StrikerIdleEmission = 3.8f;
	constexpr float StrikerHighlightEmission = 5.0f;
}
