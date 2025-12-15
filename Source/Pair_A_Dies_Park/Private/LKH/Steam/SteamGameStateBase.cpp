#include "SteamGameStateBase.h"

#include "GameFramework/PlayerState.h"
#include "Engine/World.h"
#include "SteamSessionSubsystem.h"
#include "Engine/GameInstance.h"

FString ASteamGameStateBase::GetSteamIdStringFromPlayerState(APlayerState* TargetPlayerState) const
{
	if (!TargetPlayerState)
	{
		return TEXT("");
	}

	const FUniqueNetIdRepl& UniqueIdRepl = TargetPlayerState->GetUniqueId();
	if (!UniqueIdRepl.IsValid())
	{
		return TEXT("");
	}

	TSharedPtr<const FUniqueNetId> NetId = UniqueIdRepl.GetUniqueNetId();
	if (!NetId.IsValid())
	{
		return TEXT("");
	}

	return NetId->ToString();
}

UTexture2D* ASteamGameStateBase::GetSteamAvatarFromPlayerState(
	APlayerState* TargetPlayerState,
	bool bRequestIfMissing,
	bool bLarge
) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GI = World->GetGameInstance();
	if (!GI)
	{
		return nullptr;
	}

	USteamSessionSubsystem* SSS = GI->GetSubsystem<USteamSessionSubsystem>();
	if (!SSS)
	{
		return nullptr;
	}

	const FString SteamIdStr = GetSteamIdStringFromPlayerState(TargetPlayerState);
	if (SteamIdStr.IsEmpty())
	{
		return nullptr;
	}

	// 1) 캐시에 있으면 바로 반환
	if (UTexture2D* Cached = SSS->GetCachedAvatarBySteamId(SteamIdStr))
	{
		return Cached;
	}

	// 2) 없으면 요청(비동기)
	if (bRequestIfMissing)
	{
		SSS->RequestAvatarBySteamId(SteamIdStr, bLarge);
	}

	return nullptr;
}

void ASteamGameStateBase::PrefetchAllPlayerAvatars(bool bLarge) const
{
	UWorld* World = GetWorld();
	if (!World) return;

	UGameInstance* GI = World->GetGameInstance();
	if (!GI) return;

	USteamSessionSubsystem* SSS = GI->GetSubsystem<USteamSessionSubsystem>();
	if (!SSS) return;

	for (APlayerState* PS : PlayerArray)
	{
		const FString SteamId = GetSteamIdStringFromPlayerState(PS);
		if (SteamId.IsEmpty())
		{
			continue;
		}

		SSS->RequestAvatarBySteamId(SteamId, bLarge);
	}
}