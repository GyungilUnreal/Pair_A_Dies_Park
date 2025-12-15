#include "SessionEntryWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "SteamSessionSubsystem.h"

void USessionEntryWidget::InitEntry(USteamSessionSubsystem* InSubsystem, int32 InSessionIndex, const FString& InOwnerName)
{
	SessionSubsystem = InSubsystem;
	SessionIndex = InSessionIndex;

	if (SessionText)
	{
		SessionText->SetText(FText::FromString(InOwnerName));
	}
}

void USessionEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &USessionEntryWidget::OnJoinButtonClicked);
	}
}

void USessionEntryWidget::OnJoinButtonClicked()
{
	if (SessionSubsystem)
	{
		SessionSubsystem->JoinSessionByIndex(SessionIndex);
	}
}
