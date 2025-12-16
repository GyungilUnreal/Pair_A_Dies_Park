#include "SteamSessionSubsystem.h"

#include "OnlineSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Online/OnlineSessionNames.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Texture2D.h"

const FName USteamSessionSubsystem::KEY_SERVER_NAME(TEXT("SERVER_NAME"));

USteamSessionSubsystem::USteamSessionSubsystem()
{
}

void USteamSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	EnsureOnlineInterfaces();

	// SteamAPI_Init 성공 이후에 콜백 등록
	if (SteamAPI_Init())
	{
		AvatarImageLoadedCallback.Register(this, &USteamSessionSubsystem::OnAvatarImageLoaded);

		SteamCallbackTickHandle = FTSTicker::GetCoreTicker().AddTicker(
			FTickerDelegate::CreateUObject(this, &USteamSessionSubsystem::TickSteamCallbacks),
			0.0f
		);
	}
}

void USteamSessionSubsystem::Deinitialize()
{
	// Ticker 정리
	if (SteamCallbackTickHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(SteamCallbackTickHandle);
		SteamCallbackTickHandle.Reset();
	}

	// 세션 델리게이트 정리
	if (SessionInterface.IsValid())
	{
		if (CreateSessionCompleteDelegateHandle.IsValid())
			SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionCompleteDelegateHandle);
		if (FindSessionsCompleteDelegateHandle.IsValid())
			SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsCompleteDelegateHandle);
		if (JoinSessionCompleteDelegateHandle.IsValid())
			SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionCompleteDelegateHandle);
		if (DestroySessionCompleteDelegateHandle.IsValid())
			SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}

	Super::Deinitialize();
}

bool USteamSessionSubsystem::EnsureOnlineInterfaces()
{
	OnlineSubsystem = IOnlineSubsystem::Get();
	if (!OnlineSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnlineSubsystem 이 없습니다. (Steam 초기화 실패 또는 설정 문제)"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("OnlineSubsystem 이름: %s"), *OnlineSubsystem->GetSubsystemName().ToString());

	SessionInterface = OnlineSubsystem->GetSessionInterface();
	FriendsInterface = OnlineSubsystem->GetFriendsInterface();

	if (SessionInterface.IsValid())
	{
		// 델리게이트 등록 (1회)
		CreateSessionCompleteDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
			FOnCreateSessionCompleteDelegate::CreateUObject(this, &USteamSessionSubsystem::OnCreateSessionComplete));

		FindSessionsCompleteDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
			FOnFindSessionsCompleteDelegate::CreateUObject(this, &USteamSessionSubsystem::OnFindSessionsComplete));

		JoinSessionCompleteDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
			FOnJoinSessionCompleteDelegate::CreateUObject(this, &USteamSessionSubsystem::OnJoinSessionComplete));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("세션 인터페이스를 가져오지 못했습니다."));
	}

	RefreshFriendsList();
	return true;
}

UWorld* USteamSessionSubsystem::GetWorldSafe() const
{
	if (const UGameInstance* GI = GetGameInstance())
	{
		return GI->GetWorld();
	}
	return nullptr;
}

const ULocalPlayer* USteamSessionSubsystem::GetFirstLocalPlayer() const
{
	if (const UGameInstance* GI = GetGameInstance())
	{
		return GI->GetFirstGamePlayer();
	}
	return nullptr;
}

/* =====================================================================================
 *  Public API
 * ===================================================================================== */

void USteamSessionSubsystem::HostSession(int32 NumPublicConnections, bool bIsLAN)
{
	// 기본 ServerName은 Steam 닉네임
	HostSessionWithServerName(NumPublicConnections, bIsLAN, GetSteamNickname());
}

void USteamSessionSubsystem::HostSessionWithServerName(int32 NumPublicConnections, bool bIsLAN, const FString& ServerName)
{
	PendingHostPublicConnections = NumPublicConnections;
	bPendingHostIsLAN = bIsLAN;
	PendingServerName = ServerName;
	PendingSessionAction = EPendingSessionAction::Host;
	DestroySessionIfExists();
}

void USteamSessionSubsystem::FindSessions(bool bIsLAN)
{
	FindSessionsWithMaxResults(bIsLAN, 200);
}

void USteamSessionSubsystem::FindSessionsWithMaxResults(bool bIsLAN, int32 MaxSearchResults)
{
	bPendingFindIsLAN = bIsLAN;
	PendingMaxSearchResults = FMath::Clamp(MaxSearchResults, 1, 5000);
	PendingSessionAction = EPendingSessionAction::Find;
	DestroySessionIfExists();
}

void USteamSessionSubsystem::JoinSessionByIndex(int32 SessionIndex)
{
	PendingJoinIndex = SessionIndex;
	PendingSessionAction = EPendingSessionAction::Join;
	DestroySessionIfExists();
}

/* =====================================================================================
 *  Session Destroy (safe)
 * ===================================================================================== */

void USteamSessionSubsystem::DestroySessionIfExists()
{
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("DestroySessionIfExists 실패: SessionInterface 가 유효하지 않음"));
		OnDestroySessionCompleteEvent.Broadcast(false);
		return;
	}

	FNamedOnlineSession* Existing = SessionInterface->GetNamedSession(GameSessionName);
	if (!Existing)
	{
		// 실제로 지울 게 없으면 성공으로 간주하고 다음 동작 진행
		OnDestroySessionComplete(GameSessionName, true);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("기존 세션 발견(%s) -> DestroySession 시도"), *GameSessionName.ToString());

	DestroySessionCompleteDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &USteamSessionSubsystem::OnDestroySessionComplete));

	SessionInterface->DestroySession(GameSessionName);
}

void USteamSessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionCompleteDelegateHandle);
	}

	UE_LOG(LogTemp, Log, TEXT("OnDestroySessionComplete: %s, Success=%d"), *SessionName.ToString(), bWasSuccessful ? 1 : 0);
	OnDestroySessionCompleteEvent.Broadcast(bWasSuccessful);

	// Destroy 단독 호출 시(=Pending None) 이동 옵션이 있으면 처리
	if (PendingSessionAction == EPendingSessionAction::None && bWasSuccessful)
	{
		if (!DestroyTravelMapPath.IsEmpty())
		{
			if (UWorld* World = GetWorldSafe())
			{
				UGameplayStatics::OpenLevel(World, FName(*DestroyTravelMapPath), true);
			}
		}
		return;
	}

	switch (PendingSessionAction)
	{
	case EPendingSessionAction::Host:
		PendingSessionAction = EPendingSessionAction::None;
		DoHostSession(PendingHostPublicConnections, bPendingHostIsLAN);
		break;

	case EPendingSessionAction::Find:
		PendingSessionAction = EPendingSessionAction::None;
		DoFindSessions(bPendingFindIsLAN);
		break;

	case EPendingSessionAction::Join:
	{
		const int32 JoinIndex = PendingJoinIndex;
		PendingJoinIndex = INDEX_NONE;
		PendingSessionAction = EPendingSessionAction::None;
		DoJoinSessionByIndex(JoinIndex);
		break;
	}

	default:
		break;
	}
}

/* =====================================================================================
 *  Internal implementations
 * ===================================================================================== */

void USteamSessionSubsystem::DoHostSession(int32 NumPublicConnections, bool bIsLAN)
{
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("HostSession 실패: SessionInterface 가 유효하지 않음"));
		OnHostSessionCompleteEvent.Broadcast(false);
		return;
	}

	FOnlineSessionSettings SessionSettings;
	SessionSettings.bIsLANMatch = bIsLAN;
	SessionSettings.bIsDedicated = false;
	SessionSettings.bShouldAdvertise = true;

	SessionSettings.bUsesPresence = true;
	SessionSettings.bUseLobbiesIfAvailable = true;

	SessionSettings.NumPublicConnections = NumPublicConnections;
	SessionSettings.NumPrivateConnections = 0;

	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowJoinViaPresence = true;
	SessionSettings.BuildUniqueId = 1;

	// 커스텀 서버 이름
	SessionSettings.Set(KEY_SERVER_NAME, PendingServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	const ULocalPlayer* LocalPlayer = GetFirstLocalPlayer();
	if (!LocalPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("HostSession 실패: LocalPlayer 없음"));
		OnHostSessionCompleteEvent.Broadcast(false);
		return;
	}

	TSharedPtr<const FUniqueNetId> UserId = LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId();
	if (!UserId.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("HostSession 실패: UserId 없음"));
		OnHostSessionCompleteEvent.Broadcast(false);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("세션 생성 시도: %s / ServerName=%s"), *GameSessionName.ToString(), *PendingServerName);
	const bool bStarted = SessionInterface->CreateSession(*UserId, GameSessionName, SessionSettings);
	if (!bStarted)
	{
		OnHostSessionCompleteEvent.Broadcast(false);
	}
}

void USteamSessionSubsystem::DoFindSessions(bool bIsLAN)
{
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("FindSessions 실패: SessionInterface 가 유효하지 않음"));
		OnFindSessionsCompleteEvent.Broadcast(false);
		return;
	}

	const ULocalPlayer* LocalPlayer = GetFirstLocalPlayer();
	if (!LocalPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("FindSessions 실패: LocalPlayer 없음"));
		OnFindSessionsCompleteEvent.Broadcast(false);
		return;
	}

	const TSharedPtr<const FUniqueNetId> UserId = LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId();
	if (!UserId.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("FindSessions 실패: UserId 없음"));
		OnFindSessionsCompleteEvent.Broadcast(false);
		return;
	}

	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->bIsLanQuery = bIsLAN;
	LastSessionSearch->MaxSearchResults = PendingMaxSearchResults;

	LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
		LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

		UE_LOG(LogTemp, Log, TEXT("세션 검색 시작 (LAN=%d, Max=%d, Lobbies=1, Presence=1)"), bIsLAN ? 1 : 0, PendingMaxSearchResults);
	const bool bStarted = SessionInterface->FindSessions(*UserId, LastSessionSearch.ToSharedRef());
	if (!bStarted)
	{
		OnFindSessionsCompleteEvent.Broadcast(false);
	}
}

void USteamSessionSubsystem::DoJoinSessionByIndex(int32 SessionIndex)
{
	if (!SessionInterface.IsValid() || !LastSessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinSession 실패: SessionInterface 또는 LastSessionSearch 가 유효하지 않음"));
		OnJoinSessionCompleteEvent.Broadcast(false);
		return;
	}

	if (!LastSessionSearch->SearchResults.IsValidIndex(SessionIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinSession 실패: 잘못된 인덱스 %d"), SessionIndex);
		OnJoinSessionCompleteEvent.Broadcast(false);
		return;
	}

	const ULocalPlayer* LocalPlayer = GetFirstLocalPlayer();
	if (!LocalPlayer)
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinSession 실패: LocalPlayer 없음"));
		OnJoinSessionCompleteEvent.Broadcast(false);
		return;
	}

	const TSharedPtr<const FUniqueNetId> UserId = LocalPlayer->GetPreferredUniqueNetId().GetUniqueNetId();
	if (!UserId.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("JoinSession 실패: UserId 없음"));
		OnJoinSessionCompleteEvent.Broadcast(false);
		return;
	}

	FOnlineSessionSearchResult ResultToJoin = LastSessionSearch->SearchResults[SessionIndex];
	ResultToJoin.Session.SessionSettings.bUsesPresence = true;
	ResultToJoin.Session.SessionSettings.bUseLobbiesIfAvailable = true;
	ResultToJoin.Session.SessionSettings.bAllowJoinViaPresence = true;

	const FString OwnerName = ResultToJoin.Session.OwningUserName;
	const int32 BuildId = ResultToJoin.Session.SessionSettings.BuildUniqueId;

	UE_LOG(LogTemp, Log, TEXT("세션 참가 시도: Index %d, Owner=%s, BuildId=0x%08x"),
		SessionIndex, *OwnerName, BuildId);

	const bool bStarted = SessionInterface->JoinSession(*UserId, GameSessionName, ResultToJoin);
	if (!bStarted)
	{
		OnJoinSessionCompleteEvent.Broadcast(false);
	}
}

/* =====================================================================================
 *  Callbacks
 * ===================================================================================== */

void USteamSessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Log, TEXT("OnCreateSessionComplete: %s, 성공 여부: %s"),
		*SessionName.ToString(), bWasSuccessful ? TEXT("true") : TEXT("false"));

	OnHostSessionCompleteEvent.Broadcast(bWasSuccessful);

	if (!bWasSuccessful)
	{
		return;
	}

	if (!HostTravelMapPath.IsEmpty())
	{
		if (UWorld* World = GetWorldSafe())
		{
			UGameplayStatics::OpenLevel(World, FName(*HostTravelMapPath), true);
		}
	}
}

void USteamSessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	UE_LOG(LogTemp, Log, TEXT("OnFindSessionsComplete: 성공 여부: %s"), bWasSuccessful ? TEXT("true") : TEXT("false"));

	if (!bWasSuccessful || !LastSessionSearch.IsValid())
	{
		OnFindSessionsCompleteEvent.Broadcast(false);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Raw SearchResults: %d"), LastSessionSearch->SearchResults.Num());

	for (int32 i = 0; i < LastSessionSearch->SearchResults.Num(); ++i)
	{
		const FOnlineSessionSearchResult& Result = LastSessionSearch->SearchResults[i];
		UE_LOG(LogTemp, Log, TEXT("Index %d: Owner=%s, BuildId=0x%08x"),
			i, *Result.Session.OwningUserName, Result.Session.SessionSettings.BuildUniqueId);
	}

	UE_LOG(LogTemp, Log, TEXT("찾은 세션 개수: %d"), LastSessionSearch->SearchResults.Num());

	OnSessionListUpdated.Broadcast();
	OnFindSessionsCompleteEvent.Broadcast(true);
}

void USteamSessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG(LogTemp, Log, TEXT("OnJoinSessionComplete: %s, 결과: %d"), *SessionName.ToString(), (int32)Result);

	const bool bOk = (Result == EOnJoinSessionCompleteResult::Success);
	OnJoinSessionCompleteEvent.Broadcast(bOk);

	if (!bOk || !SessionInterface.IsValid())
	{
		return;
	}

	FString ConnectString;
	if (SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
	{
		if (UWorld* World = GetWorldSafe())
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
			if (PC)
			{
				PC->ClientTravel(ConnectString, TRAVEL_Absolute);
			}
		}
	}
}

/* =====================================================================================
 *  Getters
 * ===================================================================================== */

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

TArray<FString> USteamSessionSubsystem::GetSessionSearchResults() const
{
	TArray<FString> Out;
	if (!LastSessionSearch.IsValid())
	{
		return Out;
	}

	for (const FOnlineSessionSearchResult& Result : LastSessionSearch->SearchResults)
	{
		FString ServerName;
		Result.Session.SessionSettings.Get(KEY_SERVER_NAME, ServerName);

		if (ServerName.IsEmpty())
		{
			ServerName = Result.Session.OwningUserName;
			if (ServerName.IsEmpty())
			{
				ServerName = TEXT("Unnamed Session");
			}
		}

		const int32 MaxPub = Result.Session.SessionSettings.NumPublicConnections;
		const int32 OpenPub = Result.Session.NumOpenPublicConnections;
		const int32 UsedPub = FMath::Max(0, MaxPub - OpenPub);

		Out.Add(FString::Printf(TEXT("%s (%d/%d)"), *ServerName, UsedPub, MaxPub));
	}

	return Out;
}

FString USteamSessionSubsystem::GetSteamNickname() const
{
	IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
	if (!Subsystem) return TEXT("NoSubsystem");

	IOnlineIdentityPtr Identity = Subsystem->GetIdentityInterface();
	if (!Identity.IsValid()) return TEXT("NoIdentity");

	const int32 LocalUserNum = 0;
	TSharedPtr<const FUniqueNetId> UserId = Identity->GetUniquePlayerId(LocalUserNum);
	if (!UserId.IsValid()) return TEXT("NoUserId");

	return Identity->GetPlayerNickname(*UserId);
}

/* =====================================================================================
 *  Friends
 * ===================================================================================== */

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

/* =====================================================================================
 *  Avatar
 * ===================================================================================== */

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
		UE_LOG(LogTemp, Warning, TEXT("잘못된 SteamId 문자열: %s"), *SteamId);
		return;
	}

	if (UTexture2D* Cached = GetCachedAvatarBySteamId(SteamId))
	{
		OnSteamAvatarReady.Broadcast(SteamId, Cached);
		return;
	}

	if (PendingAvatarRequests.Contains(SteamId))
	{
		return;
	}

	PendingAvatarRequests.Add(SteamId);

	const uint64 Id64 = FCString::Strtoui64(*SteamId, nullptr, 10);
	const CSteamID TargetId((uint64)Id64);

	int ImageHandle = bLarge
		? SteamFriends()->GetLargeFriendAvatar(TargetId)
		: SteamFriends()->GetSmallFriendAvatar(TargetId);

	if (ImageHandle > 0)
	{
		if (UTexture2D* Tex = CreateTextureFromSteamImage(ImageHandle))
		{
			AvatarCache.Add(SteamId, Tex);
			PendingAvatarRequests.Remove(SteamId);
			OnSteamAvatarReady.Broadcast(SteamId, Tex);
		}
		else
		{
			PendingAvatarRequests.Remove(SteamId);
		}
	}
	else if (ImageHandle == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("아바타 요청 실패(0). SteamId=%s"), *SteamId);
		PendingAvatarRequests.Remove(SteamId);
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("아바타 로딩 대기중(-1). SteamId=%s"), *SteamId);
	}
}

void USteamSessionSubsystem::OnAvatarImageLoaded(AvatarImageLoaded_t* Param)
{
	const uint64 Id64 = Param->m_steamID.ConvertToUint64();
	const FString SteamIdStr = FString::Printf(TEXT("%llu"), (uint64)Id64);

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

	UTexture2D* Tex = UTexture2D::CreateTransient((int32)Width, (int32)Height, PF_R8G8B8A8);
	if (!Tex) return nullptr;

#if WITH_EDITORONLY_DATA
	Tex->MipGenSettings = TMGS_NoMipmaps;
#endif

	Tex->SRGB = true;

	FTexture2DMipMap& Mip = Tex->GetPlatformData()->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);
	FMemory::Memcpy(Data, RGBA.GetData(), RGBA.Num());
	Mip.BulkData.Unlock();

	Tex->UpdateResource();
	return Tex;
}

bool USteamSessionSubsystem::TickSteamCallbacks(float DeltaTime)
{
	if (SteamAPI_IsSteamRunning())
	{
		SteamAPI_RunCallbacks();
	}
	return true;
}