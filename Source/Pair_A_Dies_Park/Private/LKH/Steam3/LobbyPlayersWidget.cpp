#include "LobbyPlayersWidget.h"
#include "PlayerRowWidget.h"
#include "GameFramework/GameStateBase.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Kismet/GameplayStatics.h"
#include "SteamPlayerState.h"

void ULobbyPlayersWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 처음 한 번 생성
    RebuildPlayerList();

    // 로비에서는 간단하게 “플레이어 수 변하면 리빌드”가 제일 안전/빠름
    GetWorld()->GetTimerManager().SetTimer(
        RefreshTimerHandle, this, &ULobbyPlayersWidget::TickRefresh, 0.5f, true
    );
}

void ULobbyPlayersWidget::NativeDestruct()
{
    if (RefreshTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(RefreshTimerHandle);
        RefreshTimerHandle.Invalidate();
    }
    Super::NativeDestruct();
}

void ULobbyPlayersWidget::TickRefresh()
{
    AGameStateBase* GS = UGameplayStatics::GetGameState(this);
    if (!GS) return;

    const int32 NewCount = GS->PlayerArray.Num();
    if (NewCount != LastPlayerCount)
    {
        RebuildPlayerList();
    }
}

void ULobbyPlayersWidget::RebuildPlayerList()
{
    if (!VerticalBox_List || !PlayerRowClass) return;

    VerticalBox_List->ClearChildren();

    AGameStateBase* GS = UGameplayStatics::GetGameState(this);
    if (!GS) return;

    LastPlayerCount = GS->PlayerArray.Num();

    for (APlayerState* PS : GS->PlayerArray)
    {
        if (!PS) continue;

        UPlayerRowWidget* Row = CreateWidget<UPlayerRowWidget>(GetOwningPlayer(), PlayerRowClass);
        if (!Row) continue;

        // 네가 만든 PlayerState 타입으로 캐스트
        if (ASteamPlayerState* MyPS = Cast<ASteamPlayerState>(PS))
        {
            Row->BindPlayerState(MyPS);
        }

        VerticalBox_List->AddChild(Row);
    }
}