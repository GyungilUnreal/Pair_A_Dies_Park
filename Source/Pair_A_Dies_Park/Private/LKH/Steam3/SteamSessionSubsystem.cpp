#include "SteamSessionSubsystem.h"

#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

const FName USteamSessionSubsystem::KEY_ServerName = FName(TEXT("SERVER_NAME"));

void USteamSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &USteamSessionSubsystem::OnPostLoadMapWithWorld);
}

void USteamSessionSubsystem::Deinitialize()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	Super::Deinitialize();
}

IOnlineSessionPtr USteamSessionSubsystem::GetSessionInterface() const
{
	if (IOnlineSubsystem* OSS = IOnlineSubsystem::Get())
	{
		return OSS->GetSessionInterface();
	}
	return nullptr;
}

void USteamSessionSubsystem::BindDelegates()
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid()) return;

	CreateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &USteamSessionSubsystem::HandleCreateSessionComplete));

	FindHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &USteamSessionSubsystem::HandleFindSessionsComplete));

	JoinHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &USteamSessionSubsystem::HandleJoinSessionComplete));

	DestroyHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &USteamSessionSubsystem::HandleDestroySessionComplete));

	StartHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(
		FOnStartSessionCompleteDelegate::CreateUObject(this, &USteamSessionSubsystem::HandleStartSessionComplete));

}

void USteamSessionSubsystem::ClearDelegates()
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid()) return;

	if (CreateHandle.IsValid())  SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateHandle);
	if (FindHandle.IsValid())    SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindHandle);
	if (JoinHandle.IsValid())    SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinHandle);
	if (DestroyHandle.IsValid()) SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroyHandle);
	if (StartHandle.IsValid())   SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartHandle);

	CreateHandle.Reset();
	FindHandle.Reset();
	JoinHandle.Reset();
	DestroyHandle.Reset();
	StartHandle.Reset();
}

void USteamSessionSubsystem::HostSession(const FString& InServerName, int32 MaxPublicConnections, bool bIsLAN, bool bUsePresence)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnHostSessionComplete.Broadcast(false);
		return;
	}

	BindDelegates();

	// 이미 세션이 있으면 지우고 다시 만들 수도 있음(원하면 여기서 Destroy 후 재Host 플로우로 변경)
	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		SessionInterface->DestroySession(NAME_GameSession);
		// Destroy 완료 콜백에서 다시 Host하도록 만들 수도 있는데,
		// 여기서는 단순화(바로 실패 처리)하거나, 원하는 로직으로 확장하세요.
		OnHostSessionComplete.Broadcast(false);
		return;
	}

	LastSessionSettings = MakeShared<FOnlineSessionSettings>();
	LastSessionSettings->NumPublicConnections = MaxPublicConnections;
	LastSessionSettings->NumPrivateConnections = 0;
	LastSessionSettings->bIsLANMatch = bIsLAN;
	LastSessionSettings->bShouldAdvertise = true;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bAllowJoinViaPresence = bUsePresence;
	LastSessionSettings->bUsesPresence = bUsePresence;
	LastSessionSettings->bUseLobbiesIfAvailable = true; // Steam은 로비 기반이 일반적
	LastSessionSettings->bUseLobbiesVoiceChatIfAvailable = true;

	// 커스텀 서버 이름
	LastSessionSettings->Set(KEY_ServerName, InServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// 세션에 우리 게임 식별자 태그
	LastSessionSettings->Set(SEARCH_KEYWORDS, FString(TEXT("Pair_A_Dies_Park")), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// BuildUniqueId (세션 버전 분리)
	LastSessionSettings->BuildUniqueId = 1;

	// 로컬 유저
	ULocalPlayer* LP = GetGameInstance()->GetFirstGamePlayer();
	if (!LP)
	{
		OnHostSessionComplete.Broadcast(false);
		return;
	}

	const FUniqueNetIdRepl UserId = LP->GetPreferredUniqueNetId();
	if (!UserId.IsValid())
	{
		OnHostSessionComplete.Broadcast(false);
		return;
	}

	const bool bCreateStarted = SessionInterface->CreateSession(*UserId, NAME_GameSession, *LastSessionSettings);
	if (!bCreateStarted)
	{
		OnHostSessionComplete.Broadcast(false);
	}
}

void USteamSessionSubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	OnHostSessionComplete.Broadcast(bWasSuccessful);
	if (!bWasSuccessful) return;

	bPendingStartSessionAfterTravel = true;

	const FString TravelURL = TEXT("/Game/Collaborators/LKH/Split/Steam/SteamLobby?listen");
	UGameplayStatics::OpenLevel(GetWorld(), FName(*TravelURL), true);
}


void USteamSessionSubsystem::FindSessions(int32 MaxResults, bool bIsLAN, bool bUsePresence)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnFindSessionsComplete.Broadcast(false);
		return;
	}

	BindDelegates();

	CachedResultsBP.Reset();
	CachedSearchResultsNative.Reset();

	LastSessionSearch = MakeShared<FOnlineSessionSearch>();
	LastSessionSearch->MaxSearchResults = MaxResults;
	LastSessionSearch->bIsLanQuery = bIsLAN;

	// Presence/Lobbies 기반 검색을 원하면 이 쿼리가 흔히 필요합니다.
	if (bUsePresence)
	{
		// LastSessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

		PRAGMA_DISABLE_DEPRECATION_WARNINGS
			LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);
		PRAGMA_ENABLE_DEPRECATION_WARNINGS

		LastSessionSearch->QuerySettings.Set(SEARCH_KEYWORDS, FString(TEXT("Pair_A_Dies_Park")), EOnlineComparisonOp::Equals);
	}

	ULocalPlayer* LP = GetGameInstance()->GetFirstGamePlayer();
	if (!LP)
	{
		OnFindSessionsComplete.Broadcast(false);
		return;
	}

	const FUniqueNetIdRepl UserId = LP->GetPreferredUniqueNetId();
	if (!UserId.IsValid())
	{
		OnFindSessionsComplete.Broadcast(false);
		return;
	}

	const bool bFindStarted = SessionInterface->FindSessions(*UserId, LastSessionSearch.ToSharedRef());
	if (!bFindStarted)
	{
		OnFindSessionsComplete.Broadcast(false);
	}
}

void USteamSessionSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
	UE_LOG(LogTemp, Log, TEXT("[Find] Complete=%d Results=%d"),
		bWasSuccessful ? 1 : 0,
		LastSessionSearch.IsValid() ? LastSessionSearch->SearchResults.Num() : -1);

	if (!bWasSuccessful || !LastSessionSearch.IsValid())
	{
		OnFindSessionsComplete.Broadcast(false);
		return;
	}

	CachedSearchResultsNative = LastSessionSearch->SearchResults;

	for (int32 i = 0; i < CachedSearchResultsNative.Num(); i++)
	{
		const FOnlineSessionSearchResult& R = CachedSearchResultsNative[i];
		if (!R.IsValid()) continue;

		FSessionResultBP Out;
		Out.Index = i;

		FString ServerName;
		R.Session.SessionSettings.Get(KEY_ServerName, ServerName);
		Out.ServerName = ServerName.IsEmpty() ? TEXT("NoName") : ServerName;

		const int32 MaxP = R.Session.SessionSettings.NumPublicConnections;
		const int32 OpenP = R.Session.NumOpenPublicConnections;

		Out.MaxPlayers = MaxP;
		Out.CurrentPlayers = FMath::Max(0, MaxP - OpenP);
		Out.PingMs = R.PingInMs;

		CachedResultsBP.Add(Out);

		UE_LOG(LogTemp, Log, TEXT("[Find] %d: Ping=%d Valid=%d"), i, R.PingInMs, R.IsValid() ? 1 : 0);
	}

	OnFindSessionsComplete.Broadcast(true);
}

void USteamSessionSubsystem::JoinSessionByIndex(int32 Index)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	if (!CachedSearchResultsNative.IsValidIndex(Index) || !CachedSearchResultsNative[Index].IsValid())
	{
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	BindDelegates();

	ULocalPlayer* LP = GetGameInstance()->GetFirstGamePlayer();
	if (!LP)
	{
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	const FUniqueNetIdRepl UserId = LP->GetPreferredUniqueNetId();
	if (!UserId.IsValid())
	{
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	const bool bJoinStarted = SessionInterface->JoinSession(*UserId, NAME_GameSession, CachedSearchResultsNative[Index]);
	if (!bJoinStarted)
	{
		OnJoinSessionComplete.Broadcast(false);
	}
}

void USteamSessionSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();

	const bool bSuccess = (Result == EOnJoinSessionCompleteResult::Success);
	OnJoinSessionComplete.Broadcast(bSuccess);

	if (!bSuccess || !SessionInterface.IsValid()) return;

	FString ConnectString;
	if (!SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
	{
		return;
	}

	// 클라이언트 트래블
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
	}
}

void USteamSessionSubsystem::DestroySession()
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		OnDestroySessionComplete.Broadcast(false);
		return;
	}

	BindDelegates();

	const bool bDestroyStarted = SessionInterface->DestroySession(NAME_GameSession);
	if (!bDestroyStarted)
	{
		OnDestroySessionComplete.Broadcast(false);
	}
}

void USteamSessionSubsystem::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	OnDestroySessionComplete.Broadcast(bWasSuccessful);
}

void USteamSessionSubsystem::HandleStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Log, TEXT("[SteamSessionSubsystem] StartSessionComplete: %s, Success=%d"),
		*SessionName.ToString(), bWasSuccessful ? 1 : 0);
}

void USteamSessionSubsystem::OnPostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (!bPendingStartSessionAfterTravel || !LoadedWorld) return;

	// 리슨 서버 상태 확정일 때만 StartSession
	if (LoadedWorld->GetNetMode() == NM_ListenServer)
	{
		bPendingStartSessionAfterTravel = false;
		StartGameSession();
	}
}

void USteamSessionSubsystem::StartGameSession()
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid()) return;

	if (!SessionInterface->GetNamedSession(NAME_GameSession))
	{
		UE_LOG(LogTemp, Warning, TEXT("[SteamSessionSubsystem] StartGameSession: No GameSession found."));
		return;
	}
	UE_LOG(LogTemp, Log, TEXT("[SteamSessionSubsystem] StartSession requesting..."));

	const bool bStartRequested = SessionInterface->StartSession(NAME_GameSession);

	UE_LOG(LogTemp, Log, TEXT("[SteamSessionSubsystem] StartSession requested: %d"), bStartRequested ? 1 : 0);

	EOnlineSessionState::Type State = SessionInterface->GetSessionState(NAME_GameSession);
	UE_LOG(LogTemp, Log, TEXT("[SteamSessionSubsystem] SessionState after Start: %d"), (int32)State);
}