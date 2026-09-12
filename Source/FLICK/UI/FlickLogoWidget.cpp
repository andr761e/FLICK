#include "UI/FlickLogoWidget.h"

#include "Rendering/DrawElements.h"
#include "RenderingThread.h"

UTexture2D* SFlickLogoWidget::LoadPreparedTexture(const FString& TexturePath)
{
	UTexture2D* Texture = LoadObject<UTexture2D>(nullptr, *TexturePath);
	if (!Texture)
	{
		return nullptr;
	}

	const bool bNeedsResourceRefresh = !Texture->NeverStream
		|| Texture->LODGroup != TEXTUREGROUP_UI
		|| Texture->Filter != TF_Trilinear;
	Texture->NeverStream = true;
	Texture->LODGroup = TEXTUREGROUP_UI;
	Texture->Filter = TF_Trilinear;
	Texture->SetForceMipLevelsToBeResident(30.0f);
	if (bNeedsResourceRefresh)
	{
		Texture->UpdateResource();
	}
	Texture->WaitForStreaming();
	FlushRenderingCommands();
	return Texture;
}

void SFlickLogoWidget::Construct(const FArguments& InArgs)
{
	Opacity = InArgs._Opacity;
	bCropToArtwork = InArgs._CropToArtwork;
	DesiredSize = InArgs._DesiredSize;
	LogoTexture.Reset(LoadPreparedTexture(InArgs._TexturePath));
	LogoBrush.SetResourceObject(LogoTexture.Get());
	if (bCropToArtwork)
	{
		// The source logo deliberately has generous transparent space for the splash screen.
		// Crop that space when the same texture is used as a compact main-menu wordmark.
		LogoBrush.SetUVRegion(FBox2f(FVector2f(0.02f, 0.25f), FVector2f(0.99f, 0.72f)));
		LogoBrush.ImageSize = FVector2D(1490.0f, 481.0f);
	}
	else
	{
		LogoBrush.ImageSize = !DesiredSize.IsNearlyZero()
			? DesiredSize
			: LogoTexture.IsValid()
			? FVector2D(LogoTexture->GetSizeX(), LogoTexture->GetSizeY())
			: FVector2D(1536.0f, 1024.0f);
	}
	LogoBrush.DrawAs = ESlateBrushDrawType::Image;
	SetCanTick(false);
}

FVector2D SFlickLogoWidget::ComputeDesiredSize(const float LayoutScaleMultiplier) const
{
	return LogoBrush.ImageSize;
}

int32 SFlickLogoWidget::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	const int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	const bool bParentEnabled) const
{
	if (LogoTexture.IsValid())
	{
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry(),
			&LogoBrush,
			ESlateDrawEffect::None,
			FLinearColor(1.0f, 1.0f, 1.0f, FMath::Clamp(Opacity.Get(), 0.0f, 1.0f)));
	}
	return LayerId;
}
