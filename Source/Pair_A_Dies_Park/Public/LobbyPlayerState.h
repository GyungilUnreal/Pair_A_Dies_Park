#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "LobbyPlayerState.generated.h"

/** SteamID 변경 알림 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSteamIdChanged, const FString&, SteamId);

/** SteamNickname 변경 알림 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSteamNicknameChanged, const FString&, SteamNickname);

UCLASS()
class ALobbyPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ALobbyPlayerState();

	/** SteamID64 문자열 (예: 7656119...) */
	UPROPERTY(ReplicatedUsing = OnRep_SteamId, BlueprintReadOnly, Category = "Steam")
	FString SteamId;

	/** Steam 닉네임(표시 이름) */
	UPROPERTY(ReplicatedUsing = OnRep_SteamNickname, BlueprintReadOnly, Category = "Steam")
	FString SteamNickname;

	/** 서버에서만 호출: Steam 정보 세팅 */
	UFUNCTION(BlueprintCallable, Category = "Steam")
	void ServerSetSteamIdentity(const FString& InSteamId, const FString& InSteamNickname);

	/** SteamID 변경 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "Steam")
	FOnSteamIdChanged OnSteamIdChanged;

	/** SteamNickname 변경 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "Steam")
	FOnSteamNicknameChanged OnSteamNicknameChanged;

protected:
	UFUNCTION()
	void OnRep_SteamId();

	UFUNCTION()
	void OnRep_SteamNickname();

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
