#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "steam/steam_api.h"
#include "SteamSessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSessionListUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSteamAvatarReady, FString, SteamId, UTexture2D*, AvatarTexture);

class IOnlineSubsystem;

/**
 * GameInstanceSubsystem 기반 Steam 세션/친구/아바타 관리 Subsystem
 * - Host / Find / Join / Destroy
 * - Steam Nickname / Friends / Avatar
 */
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

UCLASS()
class USteamSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	USteamSessionSubsystem();

	// Subsystem 생명주기
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/* ========================= Blueprint API ========================= */

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

	UFUNCTION(BlueprintPure, Category = "Steam|SteamInfo")
	FString GetSteamNickname() const;

	UPROPERTY(BlueprintAssignable, Category = "Steam|Session")
	FOnSessionListUpdated OnSessionListUpdated;

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

private:
	// OnlineSubsystem
	IOnlineSubsystem* OnlineSubsystem = nullptr;
	IOnlineSessionPtr SessionInterface;
	IOnlineFriendsPtr FriendsInterface;

	// Session
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;
	FName GameSessionName = TEXT("GameSession");

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
};