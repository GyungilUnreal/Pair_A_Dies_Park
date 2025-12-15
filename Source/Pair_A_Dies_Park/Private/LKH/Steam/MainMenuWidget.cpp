#include "MainMenuWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"

#include "SessionEntryWidget.h"
#include "SteamSessionSubsystem.h"

UMainMenuWidget::UMainMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, SelectedSessionIndex(-1)
	, CachedSessionSubsystem(nullptr)
{
}

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Subsystem 캐시
	CachedSessionSubsystem = GetSteamSessionSubsystem();

	// 세션 리스트 갱신 델리게이트 바인딩
	if (CachedSessionSubsystem)
	{
		CachedSessionSubsystem->OnSessionListUpdated.AddDynamic(this, &UMainMenuWidget::HandleSessionListUpdated);
	}

	if (HostButton)
	{
		HostButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnHostButtonClicked);
	}

	if (RefreshButton)
	{
		RefreshButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnRefreshButtonClicked);
	}

	if (JoinButton)
	{
		JoinButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnJoinButtonClicked);
	}

	if (FilterAllButton)
	{
		FilterAllButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnFilterAllClicked);
	}

	if (FilterFriendsButton)
	{
		FilterFriendsButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnFilterFriendsClicked);
	}

	UpdateSteamNickname();
}

void UMainMenuWidget::OnHostButtonClicked()
{
	if (USteamSessionSubsystem* S = GetSteamSessionSubsystem())
	{
		S->HostSession(4, false);
	}
}

void UMainMenuWidget::OnRefreshButtonClicked()
{
	if (USteamSessionSubsystem* S = GetSteamSessionSubsystem())
	{
		SetStatusMessage(TEXT("Searching sessions..."));
		S->FindSessions(false);
	}
}

void UMainMenuWidget::OnJoinButtonClicked()
{
	if (USteamSessionSubsystem* S = GetSteamSessionSubsystem())
	{
		if (SelectedSessionIndex >= 0)
		{
			S->JoinSessionByIndex(SelectedSessionIndex);
		}
	}
}

void UMainMenuWidget::HandleSessionListUpdated()
{
	RebuildSessionList();

	if (USteamSessionSubsystem* S = GetSteamSessionSubsystem())
	{
		const int32 Count = S->GetLastSessionSearchResultCount();
		SetStatusMessage(Count == 0 ? TEXT("No sessions found.") : TEXT(""));
	}
}

void UMainMenuWidget::RebuildSessionList()
{
	USteamSessionSubsystem* S = GetSteamSessionSubsystem();
	if (!S || !SessionListPanel || !SessionEntryClass)
		return;

	SessionListPanel->ClearChildren();

	const int32 Count = S->GetLastSessionSearchResultCount();
	const ESessionFilterMode Mode = S->GetSessionFilterMode();

	for (int32 Index = 0; Index < Count; ++Index)
	{
		bool bInclude = true;

		if (Mode == ESessionFilterMode::FriendsOnly)
		{
			bInclude = S->IsSessionOwnedByFriendByIndex(Index);
		}

		if (!bInclude)
		{
			continue;
		}

		const FString OwnerName = S->GetSessionOwnerName(Index);

		USessionEntryWidget* EntryWidget = CreateWidget<USessionEntryWidget>(GetWorld(), SessionEntryClass);
		if (!EntryWidget)
			continue;

		EntryWidget->InitEntry(S, Index, OwnerName);
		SessionListPanel->AddChild(EntryWidget);
	}

	SelectedSessionIndex = -1;
}

void UMainMenuWidget::SetStatusMessage(const FString& Message)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Message));
	}
}

void UMainMenuWidget::OnFilterAllClicked()
{
	if (USteamSessionSubsystem* S = GetSteamSessionSubsystem())
	{
		S->SetSessionFilterMode(ESessionFilterMode::All);
		RebuildSessionList();
	}
}

void UMainMenuWidget::OnFilterFriendsClicked()
{
	if (USteamSessionSubsystem* S = GetSteamSessionSubsystem())
	{
		S->SetSessionFilterMode(ESessionFilterMode::FriendsOnly);
		RebuildSessionList();
	}
}

USteamSessionSubsystem* UMainMenuWidget::GetSteamSessionSubsystem() const
{
	if (CachedSessionSubsystem)
	{
		return CachedSessionSubsystem;
	}

	if (const UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<USteamSessionSubsystem>();
	}
	return nullptr;
}

int32 UMainMenuWidget::GetSessionResultCount() const
{
	if (USteamSessionSubsystem* S = GetSteamSessionSubsystem())
	{
		return S->GetLastSessionSearchResultCount();
	}
	return 0;
}

FString UMainMenuWidget::GetSessionOwnerNameBP(int32 Index) const
{
	if (USteamSessionSubsystem* S = GetSteamSessionSubsystem())
	{
		return S->GetSessionOwnerName(Index);
	}
	return TEXT("");
}

void UMainMenuWidget::SetSelectedSessionIndex(int32 Index)
{
	SelectedSessionIndex = Index;
}

void UMainMenuWidget::UpdateSteamNickname()
{
	if (USteamSessionSubsystem* S = GetSteamSessionSubsystem())
	{
		if (NicknameText)
		{
			NicknameText->SetText(FText::FromString(S->GetSteamNickname()));
		}
	}
}