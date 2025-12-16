#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UPanelWidget;
class USessionEntryWidget;
class USteamSessionSubsystem;

UCLASS()
class UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UMainMenuWidget(const FObjectInitializer& ObjectInitializer);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UButton* HostButton;

	UPROPERTY(meta = (BindWidget))
	UButton* RefreshButton;

	UPROPERTY(meta = (BindWidget))
	UButton* JoinButton;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* NicknameText;

	UPROPERTY(meta = (BindWidget))
	UPanelWidget* SessionListPanel;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* StatusText;

	UPROPERTY(meta = (BindWidget))
	UButton* FilterAllButton;

	UPROPERTY(meta = (BindWidget))
	UButton* FilterFriendsButton;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steam|Session", meta = (AllowPrivateAccess = "true"))
	TSubclassOf<USessionEntryWidget> SessionEntryClass;

	UPROPERTY(BlueprintReadOnly, Category = "Steam|Session", meta = (AllowPrivateAccess = "true"))
	int32 SelectedSessionIndex;

	// Subsystem 캐시
	UPROPERTY()
	USteamSessionSubsystem* CachedSessionSubsystem;

	UFUNCTION()
	void OnHostButtonClicked();

	UFUNCTION()
	void OnRefreshButtonClicked();

	UFUNCTION()
	void OnJoinButtonClicked();

	UFUNCTION()
	void HandleSessionListUpdated();

	void RebuildSessionList();
	void SetStatusMessage(const FString& Message);

	UFUNCTION()
	void OnFilterAllClicked();

	UFUNCTION()
	void OnFilterFriendsClicked();

public:
	UFUNCTION(BlueprintPure, Category = "Steam|Subsystem")
	USteamSessionSubsystem* GetSteamSessionSubsystem() const;

	UFUNCTION(BlueprintPure, Category = "Steam|Session")
	int32 GetSessionResultCount() const;

	UFUNCTION(BlueprintPure, Category = "Steam|Session")
	FString GetSessionOwnerNameBP(int32 Index) const;

	UFUNCTION(BlueprintCallable, Category = "Steam|Session")
	void SetSelectedSessionIndex(int32 Index);

	UFUNCTION(BlueprintCallable, Category = "Steam|SteamInfo")
	void UpdateSteamNickname();
};