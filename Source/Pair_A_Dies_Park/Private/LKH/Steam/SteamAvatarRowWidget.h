#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SteamAvatarRowWidget.generated.h"

class UImage;
class UTextBlock;
class UTexture2D;

UCLASS()
class USteamAvatarRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Steam|Avatar")
	void SetPlayerInfo(const FString& InSteamId, const FString& InDisplayName);

	UFUNCTION(BlueprintCallable, Category = "Steam|Avatar")
	void SetAvatarTexture(UTexture2D* InTex);

	UFUNCTION(BlueprintPure, Category = "Steam|Avatar")
	const FString& GetSteamId() const { return SteamId; }

protected:
	UPROPERTY(meta = (BindWidget))
	UImage* AvatarImage = nullptr;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* NameText = nullptr;

private:
	UPROPERTY()
	FString SteamId;
};