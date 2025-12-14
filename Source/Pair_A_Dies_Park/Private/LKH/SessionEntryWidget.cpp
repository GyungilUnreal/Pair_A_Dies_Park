#include "SessionEntryWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "SteamSessionSubsystem.h"

void USessionEntryWidget::InitEntry(USteamSessionSubsystem* InSubsystem, int32 InSessionIndex, const FString& InOwnerName)
{
    Subsystem = InSubsystem;
    SessionIndex = InSessionIndex;

    CachedOwnerName = InOwnerName;

    // 위젯이 이미 Construct 된 상태라면 텍스트를 바로 갱신
    if (SessionText)
    {
        SessionText->SetText(FText::FromString(InOwnerName));
    }

    // InitEntry 시점에 확실히 바인딩
    if (Subsystem)
    {
        Subsystem->OnSessionJoinFinished.RemoveDynamic(this, &USessionEntryWidget::HandleJoinFinished);
        Subsystem->OnSessionJoinFinished.AddDynamic(this, &USessionEntryWidget::HandleJoinFinished);
    }
}

void USessionEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 버튼 델리게이트 바인딩
    if (JoinButton)
    {
        JoinButton->OnClicked.AddDynamic(this, &USessionEntryWidget::OnJoinButtonClicked);
    }

    // Join 결과를 받아 버튼 다시 켜기
    if (Subsystem)
    {
        Subsystem->OnSessionJoinFinished.RemoveDynamic(this, &USessionEntryWidget::HandleJoinFinished);
        Subsystem->OnSessionJoinFinished.AddDynamic(this, &USessionEntryWidget::HandleJoinFinished);
    }
}

void USessionEntryWidget::OnJoinButtonClicked()
{
    UE_LOG(LogTemp, Log, TEXT("[SessionEntryWidget] Join clicked. Index=%d bJoinInProgress=%d"),
        SessionIndex, bJoinInProgress ? 1 : 0);

    if (!Subsystem || !JoinButton)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SessionEntryWidget] Subsystem/JoinButton invalid"));
        return;
    }

    if (bJoinInProgress)
    {
        UE_LOG(LogTemp, Warning, TEXT("[SessionEntryWidget] Join suppressed (spam click). Index=%d"), SessionIndex);
        return;
    }

    // MainMenu에 "Join 시도"를 알림 (StatusText 안내용)
    OnEntryJoinClicked.Broadcast(SessionIndex, CachedOwnerName);

    bJoinInProgress = true;
    JoinButton->SetIsEnabled(false);

    UE_LOG(LogTemp, Log, TEXT("[SessionEntryWidget] JoinButton disabled. Calling JoinSessionByIndex(%d)"), SessionIndex);
    Subsystem->JoinSessionByIndex(SessionIndex);
}

void USessionEntryWidget::HandleJoinFinished(int32 FinishedIndex, bool bSuccess)
{
    UE_LOG(LogTemp, Log, TEXT("[SessionEntryWidget] HandleJoinFinished: FinishedIndex=%d Success=%d (Mine=%d)"),
        FinishedIndex, bSuccess ? 1 : 0, SessionIndex);

    if (FinishedIndex != SessionIndex)
    {
        return;
    }

    bJoinInProgress = false;

    if (JoinButton)
    {
        JoinButton->SetIsEnabled(true);
    }

    UE_LOG(LogTemp, Log, TEXT("[SessionEntryWidget] JoinButton re-enabled. Index=%d"), SessionIndex);
}