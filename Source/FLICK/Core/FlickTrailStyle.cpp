#include "Core/FlickTrailStyle.h"

const FlickTrailStyle::FStyle& FlickTrailStyle::Get(const int32 Index)
{
	static const FStyle Styles[Count] = {
		{{.38f,.47f,.52f}, {1,1,1}, .7f, 0, 0, 1, 0, 2, TEXT("No trail. Keep the arena clear and the puck unchanged.")},
		{{.01f,.55f,1}, {.4f,.9f,1}, .72f, 9, 5, 1, 5, 3, TEXT("A feathered electric-blue wake with a silver core and fine speed filaments.")},
		{{1,.075f,.004f}, {1,.55f,.035f}, .8f, 13, 5, 8, 4, 1, TEXT("Flowing flame tongues, a hot amber core and drifting embers that fade in the air.")},
		{{.045f,.65f,1}, {.65f,.95f,1}, .82f, 10, 4, 2, 4, 1, TEXT("Layered frost ribbons scatter rotating ice crystals and silver glints.")},
		{{1,.24f,.008f}, {1,.8f,.16f}, .85f, 15, 4.5f, 5, 4, 1, TEXT("A molten gold-and-orange wake sheds bright cinders into a soft heat haze.")},
		{{.6f,.035f,1}, {.01f,.8f,1}, .85f, 12, 4, 1, 5, 2, TEXT("Violet and cyan streams weave together over a translucent, rippling wake.")},
		{{.48f,.06f,1}, {.7f,.6f,1}, .65f, 9, 7, 5, 5, 3, TEXT("Forked violet lightning, a white-hot core and short-lived electric sparks.")},
		{{.05f,.8f,1}, {1,.2f,.6f}, .8f, 12, 4, 5, 7, 3, TEXT("Seven flowing rainbow bands with tiny prismatic glints along the wake.")},
		{{1,.63f,.09f}, {1,.91f,.55f}, .85f, 9, 5, 5, 5, 2, TEXT("Champagne-gold speed filaments and a scattering of delicate golden stars.")},
		{{.32f,.02f,.85f}, {.01f,.6f,1}, .95f, 20, 3.5f, 5, 4, 1, TEXT("A flowing violet nebula with cyan wisps and individually fading starbursts.")},
		{{.015f,.38f,1}, {.2f,.9f,1}, .75f, 7, 4, 3, 2, 1, TEXT("Bright blue pixel tiles peel away from a fine digital stream and dissolve.")},
		{{1,.035f,.32f}, {1,.38f,.65f}, .9f, 10, 3.5f, 4, 3, 2, TEXT("A soft rose wake releases floating pink hearts with a gentle, fading pulse.")},
		{{.005f,.8f,.65f}, {.2f,1,1}, .95f, 19, 3, 1, 5, 1, TEXT("Translucent turquoise wisps curl and drift upward like a spectral current.")}
	};
	return Styles[FMath::Clamp(Index, 0, Count - 1)];
}

FLinearColor FlickTrailStyle::Rainbow(const float Phase)
{
	return FLinearColor::MakeFromHSV8(static_cast<uint8>(FMath::Frac(Phase) * 255.0f), 230, 255);
}
