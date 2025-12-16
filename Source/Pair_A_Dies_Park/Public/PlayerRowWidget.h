#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerRowWidget.generated.h"

class UImage;
class UTextBlock;
class ASteamPlayerState;

UCLASS()
class UPlayerRowWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    void BindPlayerState(ASteamPlayerState* InPS);

protected:
    UPROPERTY(meta = (BindWidget)) UImage* Image_Avatar;
    UPROPERTY(meta = (BindWidget)) UTextBlock* Text_Name;

private:
    UPROPERTY() TObjectPtr<ASteamPlayerState> BoundPS;

    UFUNCTION()
    void HandleAvatarReady(UTexture2D* Avatar);
};