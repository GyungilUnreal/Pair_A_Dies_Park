#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "OnlineSessionSettings.h"
#include "steam/steam_api.h"
#include "SteamSessionSubsystem.generated.h"

class IOnlineSubsystem;
class UTexture2D;

/* ========================= 기존 + 확장 델리게이트 ========================= */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSessionListUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSteamAvatarReady, FString, SteamId, UTexture2D*, AvatarTexture);

// (레거시와 매칭되는) 완료 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSteamHostSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSteamFindSessionsComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSteamJoinSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSteamDestroySessionComplete, bool, bWasSuccessful);

UENUM(BlueprintType)
enum class ESessionFilterMode : uint8
{
	All         UMETA(DisplayName = "All Sessions"),
	FriendsOnly UMETA(DisplayName = "Friends Only")
};

UENUM()
enum class EPendingSessionAction : uint8
{
	None,
	Host,
	Find,
	Join
};

/**
 * GameInstanceSubsystem 기반 Steam 세션/친구/아바타 관리 Subsystem
 * - Host / Find / Join / Destroy
 * - ServerName(SESSION SETTING) / 검색 결과 문자열 포맷 제공
 * - Steam Nickname / Friends / Avatar
 */
UCLASS()
class USteamSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	USteamSessionSubsystem();

	// Subsystem 생명주기
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/* ========================= Blueprint API (권장) ========================= */

	UFUNCTION(BlueprintCallable, Category = "Steam|Session")
	void HostSession(int32 NumPublicConnections = 4, bool bIsLAN = false);

	UFUNCTION(BlueprintCallable, Category = "Steam|Session")
	void FindSessions(bool bIsLAN = false);

	UFUNCTION(BlueprintCallable, Category = "Steam|Session")
	void JoinSessionByIndex(int32 SessionIndex);

	UFUNCTION(BlueprintCallable, Category = "Steam|Session")
	void DestroySessionIfExists();

	UFUNCTION(BlueprintPure, Category = "Steam|Session")
	int32 GetLastSessionSearchResultCount() const;

	UFUNCTION(BlueprintPure, Category = "Steam|Session")
	FString GetSessionOwnerName(int32 SessionIndex) const;

	// 레거시(혹은 UI)에서 쓰기 좋은 표시 문자열 목록
	UFUNCTION(BlueprintCallable, Category = "Steam|Session")
	TArray<FString> GetSessionSearchResults() const;

	UFUNCTION(BlueprintPure, Category = "Steam|SteamInfo")
	FString GetSteamNickname() const;

	// 세션 목록 갱신 (UI refresh 용)
	UPROPERTY(BlueprintAssignable, Category = "Steam|Session")
	FOnSessionListUpdated OnSessionListUpdated;

	// 레거시 호환 완료 델리게이트
	UPROPERTY(BlueprintAssignable, Category = "Steam|Session")
	FSteamHostSessionComplete OnHostSessionCompleteEvent;

	UPROPERTY(BlueprintAssignable, Category = "Steam|Session")
	FSteamFindSessionsComplete OnFindSessionsCompleteEvent;

	UPROPERTY(BlueprintAssignable, Category = "Steam|Session")
	FSteamJoinSessionComplete OnJoinSessionCompleteEvent;

	UPROPERTY(BlueprintAssignable, Category = "Steam|Session")
	FSteamDestroySessionComplete OnDestroySessionCompleteEvent;

	/* ========================= 레거시 포워딩 API =========================
	 * - USessionSubsystem이 호출할 함수들
	 */
	UFUNCTION(BlueprintCallable, Category = "Steam|Session")
	void HostSessionWithServerName(int32 NumPublicConnections, bool bIsLAN, const FString& ServerName);

	UFUNCTION(BlueprintCallable, Category = "Steam|Session")
	void FindSessionsWithMaxResults(bool bIsLAN, int32 MaxSearchResults);

	/* ========================= Friends / Filter ========================= */

	UFUNCTION(BlueprintCallable, Category = "Steam|Friends")
	void RefreshFriendsList();

	UFUNCTION(BlueprintPure, Category = "Steam|Session")
	bool IsSessionOwnedByFriendByIndex(int32 SessionIndex) const;

	UFUNCTION(BlueprintPure, Category = "Steam|Session")
	ESessionFilterMode GetSessionFilterMode() const { return SessionFilterMode; }

	UFUNCTION(BlueprintCallable, Category = "Steam|Session")
	void SetSessionFilterMode(ESessionFilterMode NewMode) { SessionFilterMode = NewMode; }

	/* ========================= Avatar ========================= */

	UPROPERTY(BlueprintAssignable, Category = "Steam|Avatar")
	FOnSteamAvatarReady OnSteamAvatarReady;

	UFUNCTION(BlueprintCallable, Category = "Steam|Avatar")
	void RequestLocalUserAvatar(bool bLarge = true);

	UFUNCTION(BlueprintCallable, Category = "Steam|Avatar")
	void RequestAvatarBySteamId(const FString& SteamId, bool bLarge = true);

	UFUNCTION(BlueprintPure, Category = "Steam|Avatar")
	UTexture2D* GetCachedAvatarBySteamId(const FString& SteamId) const;

	/* ========================= Travel Settings ========================= */

	// Host 성공 시 이동할 레벨(리스너 서버). 비워두면 이동하지 않음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Steam|Travel")
	FString HostTravelMapPath = TEXT("/Game/Collaborators/LKH/Split/Lobby/Lobby?listen");

	// Destroy 완료 시 이동할 레벨. 비워두면 이동하지 않음
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Steam|Travel")
	FString DestroyTravelMapPath = TEXT("/Game/Collaborators/LKH/Split/Main/Main");

private:
	// OnlineSubsystem
	IOnlineSubsystem* OnlineSubsystem = nullptr;
	IOnlineSessionPtr SessionInterface;
	IOnlineFriendsPtr FriendsInterface;

	// Session
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;
	FName GameSessionName = TEXT("GameSession");

	// Host/Find 파라미터 캐시
	FString PendingServerName;
	int32 PendingMaxSearchResults = 200;

	// Delegates
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;

	// Pending action after destroy
	EPendingSessionAction PendingSessionAction = EPendingSessionAction::None;
	int32 PendingHostPublicConnections = 4;
	bool  bPendingHostIsLAN = false;
	bool  bPendingFindIsLAN = false;
	int32 PendingJoinIndex = INDEX_NONE;

	void DoHostSession(int32 NumPublicConnections, bool bIsLAN);
	void DoFindSessions(bool bIsLAN);
	void DoJoinSessionByIndex(int32 SessionIndex);

	// Friends
	TSet<FString> FriendIdSet;

	UPROPERTY(BlueprintReadWrite, Category = "Steam|Session", meta = (AllowPrivateAccess = "true"))
	ESessionFilterMode SessionFilterMode = ESessionFilterMode::All;

	void OnReadFriendsListCompleted(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& Error);

	// Avatar cache
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UTexture2D>> AvatarCache;

	TSet<FString> PendingAvatarRequests;

	UTexture2D* CreateTextureFromSteamImage(int ImageHandle);
	bool IsValidSteamIdString(const FString& SteamId) const;

	STEAM_CALLBACK_MANUAL(USteamSessionSubsystem, OnAvatarImageLoaded, AvatarImageLoaded_t, AvatarImageLoadedCallback);

	FTSTicker::FDelegateHandle SteamCallbackTickHandle;
	bool TickSteamCallbacks(float DeltaTime);

	// helpers
	bool EnsureOnlineInterfaces();
	const ULocalPlayer* GetFirstLocalPlayer() const;
	UWorld* GetWorldSafe() const;

	// Session setting keys
	static const FName KEY_SERVER_NAME;
};