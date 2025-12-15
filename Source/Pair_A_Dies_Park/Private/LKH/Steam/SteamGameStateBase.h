#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "SteamGameStateBase.generated.h"

class UTexture2D;
class APlayerState;

UCLASS()
class ASteamGameStateBase : public AGameStateBase
{
	GENERATED_BODY()

public:
	// GameState에서 PlayerState -> Steam Avatar(Texture) 가져오기
	// - 캐시에 있으면 즉시 반환
	// - 없으면 (bRequestIfMissing=true) 비동기 요청하고 nullptr 반환
	UFUNCTION(BlueprintCallable, Category = "Steam|Avatar")
	UTexture2D* GetSteamAvatarFromPlayerState(APlayerState* TargetPlayerState, bool bRequestIfMissing = true, bool bLarge = true) const;

	// 필요하면 BP에서 SteamID 문자열만 쓰고 싶을 때
	UFUNCTION(BlueprintPure, Category = "Steam|Avatar")
	FString GetSteamIdStringFromPlayerState(APlayerState* TargetPlayerState) const;

	// GameState에 있는 모든 PlayerState에 대해 아바타 요청(캐시 있으면 즉시 브로드캐스트)
	UFUNCTION(BlueprintCallable, Category = "Steam|Avatar")
	void PrefetchAllPlayerAvatars(bool bLarge = true) const;
};