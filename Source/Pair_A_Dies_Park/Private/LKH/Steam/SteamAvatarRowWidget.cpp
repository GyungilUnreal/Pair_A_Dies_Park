#include "SteamAvatarRowWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

void USteamAvatarRowWidget::SetPlayerInfo(const FString& InSteamId, const FString& InDisplayName)
{
	SteamId = InSteamId;
	if (NameText)
	{
		NameText->SetText(FText::FromString(InDisplayName));
	}
}

void USteamAvatarRowWidget::SetAvatarTexture(UTexture2D* InTex)
{
	if (AvatarImage && InTex)
	{
		AvatarImage->SetBrushFromTexture(InTex, true);
	}
}