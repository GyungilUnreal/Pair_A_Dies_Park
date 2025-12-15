#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SteamAvatarListWidget.generated.h"

class UVerticalBox;
class USteamAvatarRowWidget;
class USteamSessionSubsystem;
class ASteamGameStateBase;
class UTexture2D;

UCLASS()
class USteamAvatarListWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// UI 생성 후 호출(또는 BeginPlay에서 BP로 호출)
	UFUNCTION(BlueprintCallable, Category = "Steam|Avatar")
	void BindAndBuildList(bool bLarge = true);

protected:
	virtual void NativeDestruct() override;

	// 아바타 준비 콜백
	UFUNCTION()
	void HandleAvatarReady(FString SteamId, UTexture2D* AvatarTexture);

	// PlayerArray 기반으로 Row들을 재구축
	void RebuildRowsFromGameState();

protected:
	UPROPERTY(meta = (BindWidget))
	UVerticalBox* PlayerListBox = nullptr;

	// UMG에서 Row 위젯 클래스를 지정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Steam|Avatar")
	TSubclassOf<USteamAvatarRowWidget> RowWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<USteamSessionSubsystem> CachedSessionSubsystem;

	// SteamId -> RowWidget
	UPROPERTY()
	TMap<FString, TObjectPtr<USteamAvatarRowWidget>> RowBySteamId;

	bool bWantsLarge = true;
};
