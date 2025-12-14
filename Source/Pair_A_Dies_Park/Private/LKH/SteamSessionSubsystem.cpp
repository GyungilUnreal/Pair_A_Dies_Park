#include "SteamSessionSubsystem.h"

#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Online/OnlineSessionNames.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerState.h"

USteamSessionSubsystem::USteamSessionSubsystem()
	: OnlineSubsystem(nullptr)
	, GameSessionName(TEXT("GameSession"))
	, HostMapName(TEXT("/Game/Collaborators/LKH/Split/Lobby/Lobby"))
{
	// 생성자에서는 기본값만 세팅
}

void USteamSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// OnlineSubsystem 가져오기 (Steam, NULL 등)
	// DefaultEngine.ini 에서 DefaultPlatformService=Steam 으로 설정되어 있으면 Steam 서브시스템이 옴
	OnlineSubsystem = IOnlineSubsystem::Get();

	if (!OnlineSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnlineSubsystem 이 없습니다. (Steam 초기화 실패 또는 설정 문제)"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("OnlineSubsystem 이름: %s"), *OnlineSubsystem->GetSubsystemName().ToString());

	// 세션/친구 인터페이스 획득
	SessionInterface = OnlineSubsystem->GetSessionInterface();
	FriendsInterface = OnlineSubsystem->GetFriendsInterface();

	// (선택) 시작할 때 바로 친구 목록 읽기
	RefreshFriendsList();

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("세션 인터페이스를 가져오지 못했습니다."));
		return;
	}

	if (SteamAPI_Init())
	{
		AvatarImageLoadedCallback.Register(this, &USteamSessionSubsystem::OnAvatarImageLoaded);

		// Steam 콜백이 필요한 경우(아바타 -1 로딩중 처리) ticker로 콜백 펌프
		SteamCallbackTickHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &USteamSessionSubsystem::TickSteamCallbacks),
			0.0f
		);
	}

	// 세션 관련 델리게이트 바인딩
	CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &USteamSessionSubsystem::OnCreateSessionComplete));

	FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &USteamSessionSubsystem::OnFindSessionsComplete));

	JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &USteamSessionSubsystem::OnJoinSessionComplete));

	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &USteamSessionSubsystem::OnDestroySessionComplete));
}

void USteamSessionSubsystem::Deinitialize()
{
	if (SteamCallbackTickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(SteamCallbackTickHandle);
		SteamCallbackTickHandle.Reset();
	}

	AvatarCache.Empty();
	PendingAvatarRequests.Empty();
	RetryAvatarRequests.Empty();

	// Subsystem이 내려갈 때 델리게이트 언바인딩(중복 바인딩/댕글링 방지)
	if (SessionInterface.IsValid())
	{
		if (CreateSessionCompleteDelegateHandle.IsValid())
		{
			SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
			CreateSessionCompleteDelegateHandle.Reset();
		}
		if (FindSessionsCompleteDelegateHandle.IsValid())
		{
			SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
			FindSessionsCompleteDelegateHandle.Reset();
		}
		if (JoinSessionCompleteDelegateHandle.IsValid())
		{
			SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
			JoinSessionCompleteDelegateHandle.Reset();
		}
		if (DestroySessionCompleteDelegateHandle.IsValid())
		{
			SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
			DestroySessionCompleteDelegateHandle.Reset();
		}
	}

	LastSessionSearch.Reset();
	FriendIdSet.Empty();
	FriendsInterface.Reset();
	SessionInterface.Reset();
	OnlineSubsystem = nullptr;

	Super::Deinitialize();
}

TSharedPtr<const FUniqueNetId> USteamSessionSubsystem::GetLocalUserId() const
{
	// GameInstanceSubsystem은 GameInstance에 붙어있으므로 LocalPlayer 접근은 GameInstance 통해서 안전하게 처리
	const UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return nullptr;
	}

	const ULocalPlayer* LocalPlayer = GI->GetFirstGamePlayer();
	if (!LocalPlayer)
	{
		return nullptr;
	}

	return LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId();
}

void USteamSessionSubsystem::HostSession(int32 NumPublicConnections, bool bIsLAN)
{
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("HostSession 실패: SessionInterface 가 유효하지 않음"));
		return;
	}

	// 기존 세션 있으면 먼저 파괴 후 재시도
	if (HasExistingGameSession())
	{
		PendingHostPublicConnections = NumPublicConnections;
		PendingHostIsLAN = bIsLAN;
		PendingHostMapName = HostMapName;
		DestroyExistingSessionThen(EPendingSessionOp::Host);
		return;
	}

	TSharedPtr<const FUniqueNetId> UserId = GetLocalUserId();
	if (!UserId.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("HostSession 실패: UserId 없음"));
		return;
	}

	FOnlineSessionSettings SessionSettings;

	// LAN 여부
	SessionSettings.bIsLANMatch = bIsLAN;

	// 온라인 공개 여부 (Steam 사용 시 주로 true)
	SessionSettings.bIsDedicated = false;
	SessionSettings.bShouldAdvertise = true;

	// 여기 두 줄이 포인트
	SessionSettings.bUsesPresence = true;           // 친구 초대/참여
	SessionSettings.bUseLobbiesIfAvailable = true;  // Presence랑 값 맞춰줘야 함 (둘 다 true)

	SessionSettings.NumPublicConnections = NumPublicConnections;
	SessionSettings.NumPrivateConnections = 0;

	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowJoinViaPresence = true;

	// 로비 필터링용 설정
	// SessionSettings.Set("PROJECT_ID", "Pair_A_Dies_Park", EOnlineDataAdvertisementType::ViaOnlineService);
	// SessionSettings.Set("BUILD_TAG", "DEV", EOnlineDataAdvertisementType::ViaOnlineService);

	// "세션이 광고하는 맵”을 실제 HostMapName과 일치시키기
	SessionSettings.Set(SETTING_MAPNAME, HostMapName.ToString(), EOnlineDataAdvertisementType::ViaOnlineService);

	// 테스트용 ID 통일
	// SessionSettings.BuildUniqueId = 1;

	UE_LOG(LogTemp, Log, TEXT("세션 생성 시도: %s"), *GameSessionName.ToString());
	SessionInterface->CreateSession(*UserId, GameSessionName, SessionSettings);
}

void USteamSessionSubsystem::FindSessions(bool bIsLAN)
{
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("FindSessions 실패: SessionInterface 가 유효하지 않음"));
		return;
	}

	TSharedPtr<const FUniqueNetId> UserId = GetLocalUserId();
	if (!UserId.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("FindSessions 실패: UserId 없음"));
		return;
	}

	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->bIsLanQuery = bIsLAN;
	LastSessionSearch->MaxSearchResults = 200;

	// UE 5.6 기준: 로비 기반 세션을 찾으려면 이 필터가 중요
	LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

	// (선택) 아직 Presence 기반 쿼리가 필요한 환경이 있어 같이 넣어줌
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
		LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

		UE_LOG(LogTemp, Log, TEXT("세션 검색 시작 (LAN=%d, Lobbies=1, Presence=1)"), bIsLAN ? 1 : 0);
	SessionInterface->FindSessions(*UserId, LastSessionSearch.ToSharedRef());
}

void USteamSessionSubsystem::JoinSessionByIndex(int32 SessionIndex)
{
	UE_LOG(LogTemp, Log, TEXT("[JoinSessionByIndex] Request Index=%d"), SessionIndex);

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[JoinSessionByIndex] SessionInterface invalid"));
		OnSessionJoinFinished.Broadcast(SessionIndex, false);
		return;
	}

	if (!LastSessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[JoinSessionByIndex] LastSessionSearch invalid"));
		OnSessionJoinFinished.Broadcast(SessionIndex, false);
		return;
	}

	// Subsystem 레벨에서 중복 호출 방지
	if (ActiveJoinIndex != INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[JoinSessionByIndex] Join already in progress. ActiveJoinIndex=%d"), ActiveJoinIndex);
		OnSessionJoinFinished.Broadcast(SessionIndex, false);
		return;
	}

	// 기존 세션 있으면 먼저 파괴 후 재시도
	if (HasExistingGameSession())
	{
		UE_LOG(LogTemp, Warning, TEXT("[JoinSessionByIndex] Existing GameSession detected. Will Destroy then re-join. Index=%d"), SessionIndex);
		PendingJoinIndex = SessionIndex;
		DestroyExistingSessionThen(EPendingSessionOp::Join);
		return;
	}

	if (!LastSessionSearch->SearchResults.IsValidIndex(SessionIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[JoinSessionByIndex] Invalid index %d (Num=%d)"),
			SessionIndex, LastSessionSearch->SearchResults.Num());
		OnSessionJoinFinished.Broadcast(SessionIndex, false);
		return;
	}

	TSharedPtr<const FUniqueNetId> UserId = GetLocalUserId();
	if (!UserId.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[JoinSessionByIndex] UserId invalid"));
		OnSessionJoinFinished.Broadcast(SessionIndex, false);
		return;
	}

	const FOnlineSessionSearchResult& ResultToJoin = LastSessionSearch->SearchResults[SessionIndex];

	UE_LOG(LogTemp, Log, TEXT("[JoinSessionByIndex] Try Join: Index=%d Owner=%s Ping=%d BuildId=0x%08x"),
		SessionIndex,
		*ResultToJoin.Session.OwningUserName,
		ResultToJoin.PingInMs,
		ResultToJoin.Session.SessionSettings.BuildUniqueId);

	ActiveJoinIndex = SessionIndex;

	const bool bJoinCalled = SessionInterface->JoinSession(*UserId, GameSessionName, ResultToJoin);
	UE_LOG(LogTemp, Log, TEXT("[JoinSessionByIndex] JoinSession called=%d"), bJoinCalled ? 1 : 0);

	if (!bJoinCalled)
	{
		UE_LOG(LogTemp, Warning, TEXT("[JoinSessionByIndex] JoinSession returned false immediately"));
		ActiveJoinIndex = INDEX_NONE;
		OnSessionJoinFinished.Broadcast(SessionIndex, false);
	}
}

int32 USteamSessionSubsystem::GetLastSessionSearchResultCount() const
{
	return LastSessionSearch.IsValid() ? LastSessionSearch->SearchResults.Num() : 0;
}

FString USteamSessionSubsystem::GetSessionOwnerName(int32 SessionIndex) const
{
	if (!LastSessionSearch.IsValid() || !LastSessionSearch->SearchResults.IsValidIndex(SessionIndex))
	{
		return TEXT("");
	}

	return LastSessionSearch->SearchResults[SessionIndex].Session.OwningUserName;
}

FString USteamSessionSubsystem::GetSteamNickname() const
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem)
	{
		return TEXT("NoSubsystem");
	}

	IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();
	if (!Identity.IsValid())
	{
		return TEXT("NoIdentity");
	}

	const int32 LocalUserNum = 0;
	TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(LocalUserNum);
	if (!UserId.IsValid())
	{
		return TEXT("NoUserId");
	}

	return Identity->GetPlayerNickname(*UserId);
}

/* ========================= 델리게이트 콜백 구현부 ========================= */

void USteamSessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Log, TEXT("OnCreateSessionComplete: %s, Success=%d"),
		*SessionName.ToString(), bWasSuccessful ? 1 : 0);

	if (!bWasSuccessful)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (HostMapName.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("HostMapName이 설정되지 않았습니다."));
		return;
	}

	// listen은 Options로
	UGameplayStatics::OpenLevel(World, HostMapName, true, TEXT("listen"));
}

void USteamSessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	UE_LOG(LogTemp, Log, TEXT("OnFindSessionsComplete: 성공 여부: %s"), bWasSuccessful ? TEXT("true") : TEXT("false"));

	if (!bWasSuccessful || !LastSessionSearch.IsValid())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Raw SearchResults: %d"), LastSessionSearch->SearchResults.Num());

	for (int32 i = 0; i < LastSessionSearch->SearchResults.Num(); ++i)
	{
		const FOnlineSessionSearchResult& Result = LastSessionSearch->SearchResults[i];
		const FString OwnerName = Result.Session.OwningUserName;
		const int32 BuildId = Result.Session.SessionSettings.BuildUniqueId;

		UE_LOG(LogTemp, Log, TEXT("Index %d: Owner=%s, BuildId=0x%08x"), i, *OwnerName, BuildId);
	}

	UE_LOG(LogTemp, Log, TEXT("찾은 세션 개수: %d"), LastSessionSearch->SearchResults.Num());
	OnSessionListUpdated.Broadcast();
}

void USteamSessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG(LogTemp, Log, TEXT("OnJoinSessionComplete: %s, Result=%d, ActiveJoinIndex=%d"),
		*SessionName.ToString(), (int32)Result, ActiveJoinIndex);

	const bool bSuccess = (Result == EOnJoinSessionCompleteResult::Success);
	const int32 FinishedIndex = ActiveJoinIndex;
	ActiveJoinIndex = INDEX_NONE;

	// UI 버튼 복구
	OnSessionJoinFinished.Broadcast(FinishedIndex, bSuccess);
	UE_LOG(LogTemp, Log, TEXT("[Session] OnSessionJoinFinished Broadcast: Index=%d Success=%d"),
		FinishedIndex, bSuccess ? 1 : 0);

	if (!bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Session] Join failed -> NO travel."));
		// 꼬인 named session 정리
		if (HasExistingGameSession())
		{
			UE_LOG(LogTemp, Warning, TEXT("[Session] Join failed but named session exists -> DestroySession"));
			SessionInterface->DestroySession(GameSessionName);
		}
		return;
	}

	FString ConnectString;
	const bool bResolved = SessionInterface->GetResolvedConnectString(SessionName, ConnectString);
	UE_LOG(LogTemp, Log, TEXT("[Session] GetResolvedConnectString=%d, URL=%s"), bResolved ? 1 : 0, *ConnectString);

	if (!bResolved)
	{
		return;
	}

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Session] PlayerController null -> cannot travel"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Session] ClientTravel -> %s"), *ConnectString);
	PC->ClientTravel(ConnectString, TRAVEL_Absolute);
}

void USteamSessionSubsystem::RefreshFriendsList()
{
	if (!FriendsInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("FriendsInterface 가 유효하지 않음"));
		return;
	}

	const int32 LocalUserNum = 0;
	FriendsInterface->ReadFriendsList(
		LocalUserNum,
		TEXT(""),
		FOnReadFriendsListComplete::CreateUObject(this, &USteamSessionSubsystem::OnReadFriendsListCompleted));
}

void USteamSessionSubsystem::OnReadFriendsListCompleted(int32 LocalUserNum, bool bWasSuccessful, const FString& ListName, const FString& Error)
{
	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("ReadFriendsList 실패: %s"), *Error);
		return;
	}

	if (!FriendsInterface.IsValid())
	{
		return;
	}

	TArray<TSharedRef<FOnlineFriend>> FriendList;
	FriendsInterface->GetFriendsList(LocalUserNum, ListName, FriendList);

	FriendIdSet.Empty();
	for (const TSharedRef<FOnlineFriend>& Friend : FriendList)
	{
		TSharedPtr<const FUniqueNetId> FriendId = Friend->GetUserId();
		if (FriendId.IsValid())
		{
			FriendIdSet.Add(FriendId->ToString());
		}
	}

	UE_LOG(LogTemp, Log, TEXT("친구 목록 갱신: %d명"), FriendIdSet.Num());
}

bool USteamSessionSubsystem::IsSessionOwnedByFriendByIndex(int32 SessionIndex) const
{
	if (!LastSessionSearch.IsValid() || !LastSessionSearch->SearchResults.IsValidIndex(SessionIndex))
	{
		return false;
	}

	const FOnlineSessionSearchResult& Result = LastSessionSearch->SearchResults[SessionIndex];
	if (!Result.Session.OwningUserId.IsValid())
	{
		return false;
	}

	const FString OwnerIdStr = Result.Session.OwningUserId->ToString();
	return FriendIdSet.Contains(OwnerIdStr);
}

bool USteamSessionSubsystem::IsValidSteamIdString(const FString& SteamId) const
{
	if (SteamId.Len() < 10) return false;
	for (TCHAR C : SteamId)
	{
		if (!FChar::IsDigit(C)) return false;
	}
	return true;
}

UTexture2D* USteamSessionSubsystem::GetCachedAvatarBySteamId(const FString& SteamId) const
{
	if (const TObjectPtr<UTexture2D>* Found = AvatarCache.Find(SteamId))
	{
		return Found->Get();
	}
	return nullptr;
}

void USteamSessionSubsystem::RequestLocalUserAvatar(bool bLarge)
{
	if (!SteamAPI_IsSteamRunning() || !SteamUser())
	{
		UE_LOG(LogTemp, Warning, TEXT("Steam이 실행중이 아니거나 SteamUser 접근 실패"));
		return;
	}

	const CSteamID MyId = SteamUser()->GetSteamID();
	RequestAvatarBySteamId(FString::Printf(TEXT("%llu"), (uint64)MyId.ConvertToUint64()), bLarge);
}

void USteamSessionSubsystem::RequestAvatarBySteamId(const FString& SteamId, bool bLarge)
{
	if (!IsValidSteamIdString(SteamId))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Avatar] 잘못된 SteamId: %s"), *SteamId);
		return;
	}

	// 캐시에 있으면 즉시 이벤트
	if (UTexture2D* Cached = GetCachedAvatarBySteamId(SteamId))
	{
		OnSteamAvatarReady.Broadcast(SteamId, Cached);
		return;
	}

	// 중복 요청 방지
	if (PendingAvatarRequests.Contains(SteamId))
	{
		return;
	}

	PendingAvatarRequests.Add(SteamId);

	// Steam이 아니면 그냥 실패 처리(에디터/PIE에서 자주 false)
	if (!SteamAPI_IsSteamRunning())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Avatar] Steam이 실행중이 아님. SteamId=%s"), *SteamId);
		PendingAvatarRequests.Remove(SteamId);
		return;
	}

	// NOTE: SteamAPI_Init을 여기서 매번 하지 말고, 프로젝트 구조상 “이미 초기화된 상태”를 전제로 쓰는 걸 추천.
	// (GameInstance에서 직접 Init을 관리하는 구조면 그 플래그로 막는게 안정적)

	RequestSteamAvatarHandle_Internal(SteamId, bLarge);
}

void USteamSessionSubsystem::OnAvatarImageLoaded(AvatarImageLoaded_t* Param)
{
	const uint64 Id64 = Param->m_steamID.ConvertToUint64();
	const FString SteamIdStr = FString::Printf(TEXT("%llu"), (uint64)Id64);

	// 우리가 요청한 대상이 아니면 무시(원하면 캐시만 해도 됨)
	if (!PendingAvatarRequests.Contains(SteamIdStr))
	{
		return;
	}

	const int ImageHandle = Param->m_iImage;
	if (ImageHandle <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("AvatarImageLoaded: invalid image handle. SteamId=%s"), *SteamIdStr);
		PendingAvatarRequests.Remove(SteamIdStr);
		return;
	}

	if (UTexture2D* Tex = CreateTextureFromSteamImage(ImageHandle))
	{
		AvatarCache.Add(SteamIdStr, Tex);
		PendingAvatarRequests.Remove(SteamIdStr);
		OnSteamAvatarReady.Broadcast(SteamIdStr, Tex);
	}
	else
	{
		PendingAvatarRequests.Remove(SteamIdStr);
	}
}

UTexture2D* USteamSessionSubsystem::CreateTextureFromSteamImage(int ImageHandle)
{
	uint32 Width = 0, Height = 0;
	if (!SteamUtils()->GetImageSize(ImageHandle, &Width, &Height) || Width == 0 || Height == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("GetImageSize 실패 또는 0 크기"));
		return nullptr;
	}

	TArray<uint8> RGBA;
	RGBA.SetNumUninitialized(Width * Height * 4);

	if (!SteamUtils()->GetImageRGBA(ImageHandle, RGBA.GetData(), RGBA.Num()))
	{
		UE_LOG(LogTemp, Warning, TEXT("GetImageRGBA 실패"));
		return nullptr;
	}

	// UTexture2D 생성 (BGRA가 아니라 RGBA 그대로 넣어도 UE가 대체로 문제없이 표시되지만,
	// 플랫폼/머티리얼에 따라 색이 뒤집히면 R/B 스왑을 추가해줘야 함)
	UTexture2D* Tex = UTexture2D::CreateTransient((int32)Width, (int32)Height, PF_R8G8B8A8);
	if (!Tex) return nullptr;
#if WITH_EDITORONLY_DATA
	Tex->MipGenSettings = TMGS_NoMipmaps;
#endif
	Tex->SRGB = true;

	// 픽셀 데이터 복사
	FTexture2DMipMap& Mip = Tex->GetPlatformData()->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Data, RGBA.GetData(), RGBA.Num());
	Mip.BulkData.Unlock();

	Tex->UpdateResource();
	return Tex;
}

bool USteamSessionSubsystem::TickSteamCallbacks(float DeltaTime)
{
	// Steamworks 콜백은 RunCallbacks를 “주기적으로” 호출해야 들어옴
	if (SteamAPI_IsSteamRunning())
	{
		SteamAPI_RunCallbacks();
	}

	// -1로 대기중인 애들은 주기적으로 재시도(콜백이 안 오거나 늦는 케이스 대비)
	// 너무 빡세게 하면 부담이니, 여기서는 간단히 매 틱 돌리지만 필요하면 타이머로 줄여도 됨.
	if (RetryAvatarRequests.Num() > 0)
	{
		TArray<FString> Copy;
		Copy.Reserve(RetryAvatarRequests.Num());
		for (const FString& Id : RetryAvatarRequests) Copy.Add(Id);

		for (const FString& Id : Copy)
		{
			// Large로 통일(원하면 요청 시 bLarge를 저장해서 그대로 재시도)
			RequestSteamAvatarHandle_Internal(Id, true);
		}
	}

	return true;
}

bool USteamSessionSubsystem::TryGetSteamIdFromPlayerState(APlayerState* PlayerState, FString& OutSteamId) const
{
	if (!PlayerState) return false;

	const FUniqueNetIdRepl& NetId = PlayerState->GetUniqueId();
	if (!NetId.IsValid()) return false;

	OutSteamId = NetId->ToString();
	return IsValidSteamIdString(OutSteamId);
}

void USteamSessionSubsystem::RequestAvatarForPlayerState(APlayerState* PlayerState, bool bLarge)
{
	FString SteamId;
	if (!TryGetSteamIdFromPlayerState(PlayerState, SteamId))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Avatar] PlayerState에서 SteamID를 얻지 못함"));
		return;
	}

	RequestAvatarBySteamId(SteamId, bLarge);
}

void USteamSessionSubsystem::RequestSteamAvatarHandle_Internal(const FString& SteamId, bool bLarge)
{
	const uint64 Id64 = FCString::Strtoui64(*SteamId, nullptr, 10);
	const CSteamID TargetId((uint64)Id64);

	int ImageHandle = -1;
	if (bLarge)
		ImageHandle = SteamFriends()->GetLargeFriendAvatar(TargetId);
	else
		ImageHandle = SteamFriends()->GetSmallFriendAvatar(TargetId);

	// >0 즉시 사용 가능 / 0 실패 / -1 로딩중(콜백 또는 재시도 필요)
	if (ImageHandle > 0)
	{
		if (UTexture2D* Tex = CreateTextureFromSteamImage(ImageHandle))
		{
			AvatarCache.Add(SteamId, Tex);
			PendingAvatarRequests.Remove(SteamId);
			RetryAvatarRequests.Remove(SteamId);

			OnSteamAvatarReady.Broadcast(SteamId, Tex);
		}
		else
		{
			PendingAvatarRequests.Remove(SteamId);
		}
	}
	else if (ImageHandle == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Avatar] 아바타 없음/실패(0). SteamId=%s"), *SteamId);
		PendingAvatarRequests.Remove(SteamId);
		RetryAvatarRequests.Remove(SteamId);
	}
	else
	{
		// -1: 로딩중
		UE_LOG(LogTemp, Log, TEXT("[Avatar] 로딩중(-1). 콜백/재시도 대기. SteamId=%s"), *SteamId);

		// 콜백이 안 오는 환경도 있으니 재시도 목록에 넣어둠
		RetryAvatarRequests.Add(SteamId);
		// Pending은 유지 (요청중 상태)
	}
}

FString USteamSessionSubsystem::GetMySteamIdString() const
{
	// GameInstanceSubsystem이므로 GameInstance에서 LocalPlayer를 얻는 게 안전함
	const UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return TEXT("");
	}

	const ULocalPlayer* LP = GI->GetFirstGamePlayer();
	if (!LP)
	{
		return TEXT("");
	}

	// UE 표준: LocalPlayer가 들고 있는 PreferredUniqueNetId (Steam/Null/EOS 등)
	const FUniqueNetIdRepl NetIdRepl = LP->GetPreferredUniqueNetId();
	if (!NetIdRepl.IsValid())
	{
		return TEXT("");
	}

	// Steam이면 보통 SteamID64 문자열이 나옴 (예: "7656119...")
	return NetIdRepl->ToString();
}

FString USteamSessionSubsystem::GetSteamNicknameFromPlayerState(const APlayerState* PlayerState) const
{
	if (!PlayerState)
	{
		return TEXT("");
	}

	// OSS Steam 기준: PlayerName == Steam 닉네임
	return PlayerState->GetPlayerName();
}

void USteamSessionSubsystem::HostSessionWithMap(FName InMapName, int32 NumPublicConnections, bool bIsLAN)
{
	HostMapName = InMapName;
	HostSession(NumPublicConnections, bIsLAN);
}

bool USteamSessionSubsystem::HasExistingGameSession() const
{
	return SessionInterface.IsValid() && SessionInterface->GetNamedSession(GameSessionName) != nullptr;
}

void USteamSessionSubsystem::DestroyExistingSessionThen(EPendingSessionOp NextOp)
{
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Session] DestroyExistingSessionThen: SessionInterface invalid"));
		return;
	}

	PendingOp = NextOp;

	if (!HasExistingGameSession())
	{
		UE_LOG(LogTemp, Log, TEXT("[Session] DestroyExistingSessionThen: no existing session -> execute op=%d"), (int32)NextOp);
		ExecutePendingOp(); // 이미 없으면 바로 다음 op 실행
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Session] Existing session found. Destroying '%s' first... NextOp=%d"),
		*GameSessionName.ToString(), (int32)NextOp);

	const bool bCalled = SessionInterface->DestroySession(GameSessionName);
	UE_LOG(LogTemp, Log, TEXT("[Session] DestroySession called=%d"), bCalled ? 1 : 0);
}

void USteamSessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	bDestroyInProgress = false;

	UE_LOG(LogTemp, Log, TEXT("OnDestroySessionComplete: %s, Success=%d, PendingOp(before)=%d"),
		*SessionName.ToString(), bWasSuccessful ? 1 : 0, (int32)PendingOp);

	// Destroy가 실패했더라도(이미 없었거나 타이밍 이슈) 다음 op를 시도해봄
	ExecutePendingOp();
}

void USteamSessionSubsystem::ExecutePendingOp()
{
	const EPendingSessionOp Op = PendingOp;
	PendingOp = EPendingSessionOp::None;

	switch (Op)
	{
	case EPendingSessionOp::Host:
		HostMapName = PendingHostMapName.IsNone() ? HostMapName : PendingHostMapName;
		HostSession(PendingHostPublicConnections, PendingHostIsLAN);
		break;

	case EPendingSessionOp::Join:
		JoinSessionByIndex(PendingJoinIndex);
		break;

	default:
		break;
	}
}

void USteamSessionSubsystem::LeaveSession()
{
	if (bDestroyInProgress)
	{
		UE_LOG(LogTemp, Warning, TEXT("LeaveSession ignored: destroy already in progress"));
		return;
	}

	if (!SessionInterface.IsValid())
	{
		return;
	}

	if (!HasExistingGameSession())
	{
		UE_LOG(LogTemp, Log, TEXT("[Session] LeaveSession: no existing session."));
		return;
	}

	bDestroyInProgress = true;
	UE_LOG(LogTemp, Log, TEXT("[Session] LeaveSession: destroying '%s'."), *GameSessionName.ToString());
	SessionInterface->DestroySession(GameSessionName);
}
