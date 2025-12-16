#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "SteamSessionSubsystem.generated.h"

USTRUCT(BlueprintType)
struct FSessionResultBP
{
    GENERATED_BODY()

    // Blueprint에서 직접 FOnlineSessionSearchResult를 쓰기 어렵기 때문에,
    // Index 기반 Join을 하거나, 표시용 정보만 담는 구조를 많이 씁니다.
    UPROPERTY(BlueprintReadOnly) int32 Index = -1;
    UPROPERTY(BlueprintReadOnly) FString ServerName;
    UPROPERTY(BlueprintReadOnly) int32 CurrentPlayers = 0;
    UPROPERTY(BlueprintReadOnly) int32 MaxPlayers = 0;
    UPROPERTY(BlueprintReadOnly) int32 PingMs = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHostSessionCompleteBP, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFindSessionsCompleteBP, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnJoinSessionCompleteBP, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDestroySessionCompleteBP, bool, bSuccess);

UCLASS()
class USteamSessionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

protected:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
    // ===== Blueprint Events =====
    UPROPERTY(BlueprintAssignable) FOnHostSessionCompleteBP OnHostSessionComplete;
    UPROPERTY(BlueprintAssignable) FOnFindSessionsCompleteBP OnFindSessionsComplete;
    UPROPERTY(BlueprintAssignable) FOnJoinSessionCompleteBP OnJoinSessionComplete;
    UPROPERTY(BlueprintAssignable) FOnDestroySessionCompleteBP OnDestroySessionComplete;

    // Find 결과 (BP에서 목록 UI 만들 때 사용)
    UPROPERTY(BlueprintReadOnly) TArray<FSessionResultBP> CachedResultsBP;

    // ===== Blueprint APIs =====
    UFUNCTION(BlueprintCallable, Category = "Steam|Session")
    void HostSession(const FString& InServerName, int32 MaxPublicConnections, bool bIsLAN, bool bUsePresence);

    UFUNCTION(BlueprintCallable, Category = "Steam|Session")
    void FindSessions(int32 MaxResults, bool bIsLAN, bool bUsePresence);

    UFUNCTION(BlueprintCallable, Category = "Steam|Session")
    void JoinSessionByIndex(int32 Index);

    UFUNCTION(BlueprintCallable, Category = "Steam|Session")
    void DestroySession();

    UFUNCTION(BlueprintCallable, Category = "Steam|Session")
    void StartGameSession();

    // 선택: UI에서 선택한 방 정보 표시용
    UFUNCTION(BlueprintPure, Category = "Steam|Session")
    const TArray<FSessionResultBP>& GetCachedResults() const { return CachedResultsBP; }

private:
    IOnlineSessionPtr GetSessionInterface() const;
    void BindDelegates();
    void ClearDelegates();

    // ===== Delegate handlers =====
    void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
    void HandleFindSessionsComplete(bool bWasSuccessful);
    void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
    void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
    void HandleStartSessionComplete(FName SessionName, bool bWasSuccessful);

    void OnPostLoadMapWithWorld(UWorld* LoadedWorld);

private:
    // Online handle
    FDelegateHandle CreateHandle;
    FDelegateHandle FindHandle;
    FDelegateHandle JoinHandle;
    FDelegateHandle DestroyHandle;
    FDelegateHandle StartHandle;

    // Search / Settings cache
    TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
    TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

    // Join을 Index로 하기 위해 원본 결과도 캐싱
    TArray<FOnlineSessionSearchResult> CachedSearchResultsNative;

    // 커스텀 키
    static const FName KEY_ServerName; // "SERVER_NAME" 같은 키

    bool bPendingStartSessionAfterTravel = false;
};