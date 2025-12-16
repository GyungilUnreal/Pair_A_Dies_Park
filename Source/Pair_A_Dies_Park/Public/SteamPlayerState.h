#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "SteamPlayerState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSteamAvatarReady, UTexture2D*, Avatar);

UCLASS()
class ASteamPlayerState : public APlayerState
{
    GENERATED_BODY()

public:
    ASteamPlayerState();

    // UI에서 바인딩하기 쉬운 이벤트
    UPROPERTY(BlueprintAssignable, Category = "Steam|Avatar")
    FOnSteamAvatarReady OnSteamAvatarReady;

    // 현재 로드된 아바타(로컬 클라이언트에만 존재해도 OK)
    UPROPERTY(BlueprintReadOnly, Category = "Steam|Avatar")
    TObjectPtr<UTexture2D> SteamAvatarTexture = nullptr;

    // SteamID(64bit)를 문자열로 복제
    UPROPERTY(ReplicatedUsing = OnRep_SteamId64, BlueprintReadOnly, Category = "Steam|Avatar")
    FString SteamId64;

    // 필요하면 BP에서도 강제 재요청 가능
    UFUNCTION(BlueprintCallable, Category = "Steam|Avatar")
    void RequestSteamAvatar();

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION()
    void OnRep_SteamId64();

private:
    void CacheMySteamIdIfLocal();
    void TryLoadSteamAvatarInternal();
    UTexture2D* CreateTextureFromSteamRGBA(const TArray<uint8>& RGBA, int32 Width, int32 Height);

    FTimerHandle RetryTimerHandle;
};