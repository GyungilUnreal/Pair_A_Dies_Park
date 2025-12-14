#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineFriendsInterface.h"
#include "Engine/Texture2D.h"
#include "steam/steam_api.h"
#include "SteamSessionSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSessionListUpdated);

// 스팀 아바타가 준비되면 UI에 알려주기 위한 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSteamAvatarReady, FString, SteamId, UTexture2D*, AvatarTexture);

// Join 버튼 연타 방지용: 어떤 인덱스 Join이 끝났는지 UI에 알려줌
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSessionJoinFinished, int32, SessionIndex, bool, bSuccess);

class IOnlineSubsystem;

/**
 * Steam/OnlineSubsystem 기반 세션 기능을 GameInstanceSubsystem으로 분리한 버전
 * - 세션 생성
 * - 세션 검색
 * - 세션 참가
 * - 스팀 닉네임 가져오기
 * - (선택) 친구 목록 읽고 친구 세션 여부 판단
 */

UENUM()
enum class EPendingSessionOp : uint8
{
	None,
	Host,
	Join,
};

UENUM(BlueprintType)
enum class ESessionFilterMode : uint8
{
	All         UMETA(DisplayName = "All Sessions"),
	FriendsOnly UMETA(DisplayName = "Friends Only")
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

	/* ========================= 블루프린트에서 호출할 함수들 ========================= */

	// 스팀 세션(방) 생성
	// NumPublicConnections : 최대 플레이어 수
	// bIsLAN : LAN 게임 여부 (Steam 사용 시 대부분 false)
	UFUNCTION(BlueprintCallable, Category = "Steam|Session")
	void HostSession(int32 NumPublicConnections = 4, bool bIsLAN = false);

	// 세션(방) 검색 시작
	UFUNCTION(BlueprintCallable, Category = "Steam|Session")
	void FindSessions(bool bIsLAN = false);

	// FindSessions 결과 중 Index번째 세션에 참가
	UFUNCTION(BlueprintCallable, Category = "Steam|Session")
	void JoinSessionByIndex(int32 SessionIndex);

	// 외부(UI)에서 "나가기"에 쓰기
	UFUNCTION(BlueprintCallable, Category = "Steam|Session")
	void LeaveSession();

	// 마지막으로 검색된 세션 리스트 개수 반환 (UI에서 사용)
	UFUNCTION(BlueprintPure, Category = "Steam|Session")
	int32 GetLastSessionSearchResultCount() const;

	// 인덱스로 세션의 호스트 이름(닉네임) 가져오기 (UI용)
	UFUNCTION(BlueprintPure, Category = "Steam|Session")
	FString GetSessionOwnerName(int32 SessionIndex) const;

	// 현재 로컬 플레이어의 스팀 닉네임 반환
	UFUNCTION(BlueprintPure, Category = "Steam|SteamInfo")
	FString GetSteamNickname() const;

	// 친구 목록 새로고침 (게임 시작 시 한 번 호출해두면 좋음)
	UFUNCTION(BlueprintCallable, Category = "Steam|Friends")
	void RefreshFriendsList();

	// 인덱스 기준으로 이 세션이 친구가 만든 세션인지 여부
	UFUNCTION(BlueprintPure, Category = "Steam|Session")
	bool IsSessionOwnedByFriendByIndex(int32 SessionIndex) const;

	// 필터 모드 Getter / Setter
	UFUNCTION(BlueprintPure, Category = "Steam|Session")
	ESessionFilterMode GetSessionFilterMode() const { return SessionFilterMode; }

	UFUNCTION(BlueprintCallable, Category = "Steam|Session")
	void SetSessionFilterMode(ESessionFilterMode NewMode) { SessionFilterMode = NewMode; }

	UFUNCTION(BlueprintCallable, Category = "Steam|Session")
	void HostSessionWithMap(FName InMapName, int32 NumPublicConnections = 4, bool bIsLAN = false);

public:
	// 세션 검색 리스트가 갱신되었을 때 브로드캐스트되는 델리게이트 (UI에서 바인딩하여 리스트 갱신용으로 사용)
	UPROPERTY(BlueprintAssignable, Category = "Steam|Session")
	FOnSessionListUpdated OnSessionListUpdated;

	UPROPERTY(BlueprintAssignable, Category = "Steam|Session")
	FOnSessionJoinFinished OnSessionJoinFinished;

private:
	// OnlineSubsystem 포인터 (Steam, NULL 등)
	IOnlineSubsystem* OnlineSubsystem;

	// 세션 인터페이스 포인터
	IOnlineSessionPtr SessionInterface;

	// 스팀 친구 인터페이스
	IOnlineFriendsPtr FriendsInterface;

	// 마지막 세션 검색 결과
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	// 세션 이름 (호스트/클라이언트가 동일한 이름 사용)
	FName GameSessionName;

	// 델리게이트 핸들 (Deinitialize에서 언바인딩)
	FDelegateHandle CreateSessionCompleteDelegateHandle;
	FDelegateHandle FindSessionsCompleteDelegateHandle;
	FDelegateHandle JoinSessionCompleteDelegateHandle;
	FDelegateHandle DestroySessionCompleteDelegateHandle;

	// Pending op 상태
	EPendingSessionOp PendingOp = EPendingSessionOp::None;

	// Pending Host params
	int32 PendingHostPublicConnections = 4;
	bool  PendingHostIsLAN = false;
	FName PendingHostMapName;

	// Pending Join params
	int32 PendingJoinIndex = INDEX_NONE;

	// Join 중인 인덱스 저장용 멤버
	int32 ActiveJoinIndex = INDEX_NONE;

	// Host/Join/Destroy 중복 호출 방지
	bool  bBusySessionOp = false;  

	UPROPERTY(Transient)
	bool bDestroyInProgress = false;

	// 친구들의 Steam ID 문자열 목록 (빠른 비교용)
	TSet<FString> FriendIdSet;

	// 현재 필터 모드 (기본값: 전체)
	UPROPERTY(BlueprintReadWrite, Category = "Steam|Session", meta = (AllowPrivateAccess = "true"))
	ESessionFilterMode SessionFilterMode = ESessionFilterMode::All;

	// Host 시 열릴 맵 (Blueprint에서 설정)
	UPROPERTY()
	FName HostMapName;

private:
	/* ========================= 내부 유틸 ========================= */

	// 로컬 유저의 UniqueNetId 얻기 (없으면 nullptr)
	TSharedPtr<const FUniqueNetId> GetLocalUserId() const;

	bool HasExistingGameSession() const;
	void DestroyExistingSessionThen(EPendingSessionOp NextOp);
	void ExecutePendingOp();

	/* ========================= 내부 델리게이트 콜백 ========================= */

	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void OnFindSessionsComplete(bool bWasSuccessful);
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	void OnReadFriendsListCompleted(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& Error);

public:
	// 아바타 준비 콜백 (UI에서 바인딩)
	UPROPERTY(BlueprintAssignable, Category = "Steam|Avatar")
	FOnSteamAvatarReady OnSteamAvatarReady;

	// 로컬 유저(나) 아바타 요청
	UFUNCTION(BlueprintCallable, Category = "Steam|Avatar")
	void RequestLocalUserAvatar(bool bLarge = true);

	// 특정 SteamId(문자열: 7656119...) 아바타 요청
	UFUNCTION(BlueprintCallable, Category = "Steam|Avatar")
	void RequestAvatarBySteamId(const FString& SteamId, bool bLarge = true);

	// 캐시된 아바타 반환 (없으면 nullptr)
	UFUNCTION(BlueprintPure, Category = "Steam|Avatar")
	UTexture2D* GetCachedAvatarBySteamId(const FString& SteamId) const;

	// PlayerState(다른 플레이어)로부터 SteamID를 뽑아서 아바타 요청
	UFUNCTION(BlueprintCallable, Category = "Steam|Avatar")
	void RequestAvatarForPlayerState(APlayerState* PlayerState, bool bLarge = true);

	UFUNCTION(BlueprintPure, Category = "Steam|Identity")
	FString GetMySteamIdString() const;

	UFUNCTION(BlueprintPure, Category = "Steam|Identity")
	FString GetSteamNicknameFromPlayerState(const APlayerState* PlayerState) const;

private:
	// SteamId -> Texture 캐시
	UPROPERTY(Transient)
	TMap<FString, TObjectPtr<UTexture2D>> AvatarCache;

	// 비동기 로딩 대기중인 SteamId 집합
	TSet<FString> PendingAvatarRequests;

	// -1(로딩중)일 때 재시도해야 하는 SteamID 목록
	TSet<FString> RetryAvatarRequests;

private:
	// Steam 콜백 펌프용 ticker
	FTSTicker::FDelegateHandle SteamCallbackTickHandle;
	bool TickSteamCallbacks(float DeltaTime);

	// Steamworks: 아바타 비동기 로드 완료 콜백
	STEAM_CALLBACK_MANUAL(USteamSessionSubsystem, OnAvatarImageLoaded, AvatarImageLoaded_t, AvatarImageLoadedCallback);

	// Steam 이미지 핸들(int) -> UTexture2D 변환
	UTexture2D* CreateTextureFromSteamImage(int ImageHandle);

	// PlayerState로부터 SteamID 받아오기
	bool TryGetSteamIdFromPlayerState(APlayerState* PlayerState, FString& OutSteamId) const;

	// SteamId 문자열이 숫자인지 간단 검증
	bool IsValidSteamIdString(const FString& SteamId) const;

	// SteamId에 대해 (Large/Small) 핸들 요청 후 처리
	void RequestSteamAvatarHandle_Internal(const FString& SteamId, bool bLarge);

};