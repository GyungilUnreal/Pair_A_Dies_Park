#include "SteamAvatarListWidget.h"

#include "Components/VerticalBox.h"
#include "SteamAvatarRowWidget.h"
#include "SteamGameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Engine/Texture2D.h"
#include "Engine/GameInstance.h"
#include "SteamSessionSubsystem.h"

void USteamAvatarListWidget::BindAndBuildList(bool bLarge)
{
	bWantsLarge = bLarge;

	UWorld* World = GetWorld();
	if (!World) return;

	UGameInstance* GI = World->GetGameInstance();
	if (!GI) return;

	CachedSessionSubsystem = GI->GetSubsystem<USteamSessionSubsystem>();
	if (!CachedSessionSubsystem) return;

	// 1) 델리게이트 바인딩 (아바타 오면 UI 갱신)
	CachedSessionSubsystem->OnSteamAvatarReady.AddDynamic(this, &USteamAvatarListWidget::HandleAvatarReady);

	// 2) 현재 플레이어 목록으로 Row 생성
	RebuildRowsFromGameState();

	// 3) GameState에서 전원 아바타 프리페치
	if (ASteamGameStateBase* GS = World->GetGameState<ASteamGameStateBase>())
	{
		GS->PrefetchAllPlayerAvatars(bWantsLarge);
	}
}

void USteamAvatarListWidget::NativeDestruct()
{
	if (CachedSessionSubsystem)
	{
		CachedSessionSubsystem->OnSteamAvatarReady.RemoveDynamic(this, &USteamAvatarListWidget::HandleAvatarReady);
	}
	Super::NativeDestruct();
}

void USteamAvatarListWidget::RebuildRowsFromGameState()
{
	if (!PlayerListBox || !RowWidgetClass) return;

	PlayerListBox->ClearChildren();
	RowBySteamId.Empty();

	UWorld* World = GetWorld();
	if (!World) return;

	ASteamGameStateBase* GS = World->GetGameState<ASteamGameStateBase>();
	if (!GS) return;

	for (APlayerState* PS : GS->PlayerArray)
	{
		if (!PS) continue;

		const FString SteamId = GS->GetSteamIdStringFromPlayerState(PS);
		if (SteamId.IsEmpty()) continue;

		USteamAvatarRowWidget* Row = CreateWidget<USteamAvatarRowWidget>(World, RowWidgetClass);
		if (!Row) continue;

		Row->SetPlayerInfo(SteamId, PS->GetPlayerName());

		// 캐시에 이미 있으면 즉시 꽂기 :contentReference[oaicite:6]{index=6}
		if (CachedSessionSubsystem)
		{
			if (UTexture2D* CachedTex = CachedSessionSubsystem->GetCachedAvatarBySteamId(SteamId))
			{
				Row->SetAvatarTexture(CachedTex);
			}
		}

		PlayerListBox->AddChild(Row);
		RowBySteamId.Add(SteamId, Row);
	}
}

void USteamAvatarListWidget::HandleAvatarReady(FString SteamId, UTexture2D* AvatarTexture)
{
	if (!AvatarTexture) return;

	if (TObjectPtr<USteamAvatarRowWidget>* Found = RowBySteamId.Find(SteamId))
	{
		if (USteamAvatarRowWidget* Row = Found->Get())
		{
			Row->SetAvatarTexture(AvatarTexture);
		}
	}
}