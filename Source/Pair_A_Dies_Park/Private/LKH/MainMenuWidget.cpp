#include "MainMenuWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/PanelWidget.h"
#include "Kismet/GameplayStatics.h"

#include "SteamSessionSubsystem.h"
#include "SessionEntryWidget.h"

UMainMenuWidget::UMainMenuWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
    , SelectedSessionIndex(-1)
    , CachedSubsystem(nullptr)
{
}

void UMainMenuWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // GameInstance Subsystem 방식으로 캐시
    CachedSubsystem = GetGameInstance() ? GetGameInstance()->GetSubsystem<USteamSessionSubsystem>() : nullptr;

    // 세션 리스트 갱신 델리게이트 바인딩
    if (CachedSubsystem)
    {
        CachedSubsystem->OnSessionListUpdated.AddDynamic(this, &UMainMenuWidget::HandleSessionListUpdated);
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

    // 필터 버튼 바인딩
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

/* ========================= 내부 델리게이트 함수 ========================= */

void UMainMenuWidget::OnHostButtonClicked()
{
    USteamSessionSubsystem* GI = GetSteamSessionSubsystem();
    if (GI)
    {
        GI->HostSession(4, false);
    }
}

void UMainMenuWidget::OnRefreshButtonClicked()
{
    USteamSessionSubsystem* GI = GetSteamSessionSubsystem();
    if (GI)
    {
        SetStatusMessage(TEXT("Searching sessions..."));
        GI->FindSessions(false);
    }

    // 결과는 OnSessionListUpdated → HandleSessionListUpdated → RebuildSessionList 로 이어짐
}

void UMainMenuWidget::OnJoinButtonClicked()
{
    USteamSessionSubsystem* GI = GetSteamSessionSubsystem();
    if (GI && SelectedSessionIndex >= 0)
    {
        GI->JoinSessionByIndex(SelectedSessionIndex);
    }
}

void UMainMenuWidget::HandleSessionListUpdated()
{
    // 세션 검색 결과가 바뀌면 리스트를 다시 그림
    RebuildSessionList();

    USteamSessionSubsystem* GI = GetSteamSessionSubsystem();
    if (!GI)
        return;

    const int32 Count = GI->GetLastSessionSearchResultCount();

    if (Count == 0)
    {
        SetStatusMessage(TEXT("No sessions found."));
    }
    else
    {
        SetStatusMessage(TEXT("")); // 성공 시 메시지 제거
    }
}

void UMainMenuWidget::RebuildSessionList()
{
    USteamSessionSubsystem* GI = GetSteamSessionSubsystem();
    if (!GI || !SessionListPanel || !SessionEntryClass)
        return;

    // 기존 항목 제거
    SessionListPanel->ClearChildren();

    const int32 Count = GI->GetLastSessionSearchResultCount();
    const ESessionFilterMode Mode = GI->GetSessionFilterMode();

    for (int32 Index = 0; Index < Count; ++Index)
    {
        bool bInclude = true;

        if (Mode == ESessionFilterMode::FriendsOnly)
        {
            bInclude = GI->IsSessionOwnedByFriendByIndex(Index);
        }

        if (!bInclude)
        {
            continue;
        }

        const FString OwnerName = GI->GetSessionOwnerName(Index);

        // USessionEntryWidget 생성
        USessionEntryWidget* EntryWidget = CreateWidget<USessionEntryWidget>(GetWorld(), SessionEntryClass);
        if (!EntryWidget)
            continue;

        // Subsystem, 인덱스, 표시용 이름 전달
        EntryWidget->InitEntry(GI, Index, OwnerName);

        // 패널에 추가
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
    USteamSessionSubsystem* GI = GetSteamSessionSubsystem();
    if (GI)
    {
        GI->SetSessionFilterMode(ESessionFilterMode::All);
        RebuildSessionList();
    }
}

void UMainMenuWidget::OnFilterFriendsClicked()
{
    USteamSessionSubsystem* GI = GetSteamSessionSubsystem();
    if (GI)
    {
        GI->SetSessionFilterMode(ESessionFilterMode::FriendsOnly);
        RebuildSessionList();
    }
}

/* ========================= GI 접근 & 헬퍼 ========================= */

USteamSessionSubsystem* UMainMenuWidget::GetSteamSessionSubsystem() const
{
    if (CachedSubsystem)
    {
        return CachedSubsystem;
    }

    if (UGameInstance* GI = GetGameInstance())
    {
        return GI->GetSubsystem<USteamSessionSubsystem>();
    }

    return nullptr;
}


int32 UMainMenuWidget::GetSessionResultCount() const
{
    USteamSessionSubsystem* GI = GetSteamSessionSubsystem();
    return GI ? GI->GetLastSessionSearchResultCount() : 0;
}

FString UMainMenuWidget::GetSessionOwnerNameBP(int32 Index) const
{
    USteamSessionSubsystem* GI = GetSteamSessionSubsystem();
    return GI ? GI->GetSessionOwnerName(Index) : TEXT("");
}

void UMainMenuWidget::SetSelectedSessionIndex(int32 Index)
{
    SelectedSessionIndex = Index;
}

void UMainMenuWidget::UpdateSteamNickname()
{
    USteamSessionSubsystem* GI = GetSteamSessionSubsystem();
    if (GI && NicknameText)
    {
        NicknameText->SetText(FText::FromString(GI->GetSteamNickname()));
    }
}