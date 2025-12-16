#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SessionEntryWidget.generated.h"

class UButton;
class UTextBlock;
class USteamSessionSubsystem;

UCLASS()
class USessionEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void InitEntry(USteamSessionSubsystem* InSubsystem, int32 InSessionIndex, const FString& InOwnerName);

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	UButton* JoinButton;

	UPROPERTY(meta = (BindWidget))
	UTextBlock* SessionText;

	UPROPERTY()
	USteamSessionSubsystem* SessionSubsystem;

	int32 SessionIndex = INDEX_NONE;

	UFUNCTION()
	void OnJoinButtonClicked();
};
