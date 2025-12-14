#include "SessionSubsystem.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"

USessionSubsystem::USessionSubsystem()
{
}

void USessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (!IsLegacyEnabled())
	{
		UE_LOG(LogTemp, Log, TEXT("[LegacySessionSubsystem] Disabled by config. Skipping init."));
		return;
	}

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		SessionInterface = OnlineSubsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &USessionSubsystem::OnCreateSessionComplete);
			SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &USessionSubsystem::OnFindSessionsComplete);
			SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &USessionSubsystem::OnJoinSessionComplete);
			SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &USessionSubsystem::OnDestroySessionComplete);
		}
	}
}

void USessionSubsystem::Deinitialize()
{
	// 델리게이트 해제
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegates(this);
		SessionInterface->ClearOnFindSessionsCompleteDelegates(this);
		SessionInterface->ClearOnJoinSessionCompleteDelegates(this);
		SessionInterface->ClearOnDestroySessionCompleteDelegates(this);
	}

	Super::Deinitialize();
}

void USessionSubsystem::CreateSession(int32 NumPublicConnections, bool bIsLANMatch, FString ServerName)
{
	if (!IsLegacyEnabled())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LegacySessionSubsystem] CreateSession ignored (disabled)."));
		return;
	}

	if (!SessionInterface.IsValid())
	{
		OnCreateSessionCompleteEvent.Broadcast(false);
		return;
	}

	// 기존 세션이 있다면 삭제
	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		SessionInterface->DestroySession(NAME_GameSession);
	}

	// 세션 설정
	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	LastSessionSettings->NumPublicConnections = NumPublicConnections;
	LastSessionSettings->bShouldAdvertise = true;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bIsLANMatch = bIsLANMatch;
	LastSessionSettings->bUsesPresence = true;
	LastSessionSettings->bAllowJoinViaPresence = true;
	LastSessionSettings->bUseLobbiesIfAvailable = true;

	// 커스텀 서버 이름 설정
	LastSessionSettings->Set(FName("SERVER_NAME"), ServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// 로컬 플레이어 가져오기
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer)
	{
		OnCreateSessionCompleteEvent.Broadcast(false);
		return;
	}

	// 세션 생성
	if (!SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
	{
		OnCreateSessionCompleteEvent.Broadcast(false);
	}
}

void USessionSubsystem::FindSessions(int32 MaxSearchResults, bool bIsLANMatch)
{
	if (!IsLegacyEnabled())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LegacySessionSubsystem] FindSessions ignored (disabled)."));
		return;
	}

	if (!SessionInterface.IsValid())
	{
		OnFindSessionsCompleteEvent.Broadcast(false);
		return;
	}

	// 세션 검색 설정
	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	LastSessionSearch->bIsLanQuery = bIsLANMatch;
	LastSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	// 로컬 플레이어 가져오기
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer)
	{
		OnFindSessionsCompleteEvent.Broadcast(false);
		return;
	}

	// 세션 검색 시작
	if (!SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		OnFindSessionsCompleteEvent.Broadcast(false);
	}
}

void USessionSubsystem::JoinSession(int32 SessionIndex)
{
	if (!IsLegacyEnabled())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LegacySessionSubsystem] JoinSession ignored (disabled)."));
		return;
	}

	if (!SessionInterface.IsValid() || !LastSessionSearch.IsValid())
	{
		OnJoinSessionCompleteEvent.Broadcast(false);
		return;
	}

	// 검색 결과에서 세션 선택
	if (!LastSessionSearch->SearchResults.IsValidIndex(SessionIndex))
	{
		OnJoinSessionCompleteEvent.Broadcast(false);
		return;
	}

	// 로컬 플레이어 가져오기
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer)
	{
		OnJoinSessionCompleteEvent.Broadcast(false);
		return;
	}

	// 세션 참가
	if (!SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, LastSessionSearch->SearchResults[SessionIndex]))
	{
		OnJoinSessionCompleteEvent.Broadcast(false);
	}
}

void USessionSubsystem::DestroySession()
{
	if (!IsLegacyEnabled())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LegacySessionSubsystem] DestroySession ignored (disabled)."));
		return;

	}
	if (!SessionInterface.IsValid())
	{
		OnDestroySessionCompleteEvent.Broadcast(false);
		return;
	}

	// 세션 삭제
	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		OnDestroySessionCompleteEvent.Broadcast(false);
	}
}

TArray<FString> USessionSubsystem::GetSessionSearchResults()
{
	if (!IsLegacyEnabled())
	{
		UE_LOG(LogTemp, Warning, TEXT("[LegacySessionSubsystem] GetSessionSearchResults ignored (disabled)."));
		return TArray<FString>();
	}

	TArray<FString> SessionNames;

	if (!LastSessionSearch.IsValid())
	{
		return SessionNames;
	}

	for (const FOnlineSessionSearchResult& Result : LastSessionSearch->SearchResults)
	{
		FString ServerName;
		Result.Session.SessionSettings.Get(FName("SERVER_NAME"), ServerName);

		if (ServerName.IsEmpty())
		{
			ServerName = FString::Printf(TEXT("Session %d"), SessionNames.Num() + 1);
		}

		FString SessionInfo = FString::Printf(TEXT("%s (%d/%d)"),
			*ServerName,
			Result.Session.SessionSettings.NumPublicConnections - Result.Session.NumOpenPublicConnections,
			Result.Session.SessionSettings.NumPublicConnections);

		SessionNames.Add(SessionInfo);
	}

	return SessionNames;
}

void USessionSubsystem::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("Session created successfully: %s"), *SessionName.ToString());

		// 호스트로서 맵을 로드할 수 있음
		GetWorld()->ServerTravel("/Game/Collaborators/LKH/Split/Lobby/Lobby?listen");
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to create session"));
	}

	OnCreateSessionCompleteEvent.Broadcast(bWasSuccessful);
}

void USessionSubsystem::OnFindSessionsComplete(bool bWasSuccessful)
{
	if (bWasSuccessful && LastSessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("Found %d sessions"), LastSessionSearch->SearchResults.Num());

		for (const FOnlineSessionSearchResult& Result : LastSessionSearch->SearchResults)
		{
			FString ServerName;
			Result.Session.SessionSettings.Get(FName("SERVER_NAME"), ServerName);
			UE_LOG(LogTemp, Log, TEXT("Session: %s, Ping: %d"), *ServerName, Result.PingInMs);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to find sessions or no sessions found"));
	}

	OnFindSessionsCompleteEvent.Broadcast(bWasSuccessful);
}

void USessionSubsystem::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	bool bWasSuccessful = (Result == EOnJoinSessionCompleteResult::Success);

	if (bWasSuccessful)
	{
		// 연결 문자열 가져오기
		FString ConnectString;
		if (SessionInterface->GetResolvedConnectString(SessionName, ConnectString))
		{
			UE_LOG(LogTemp, Log, TEXT("Joining session at: %s"), *ConnectString);

			// 클라이언트로 서버에 접속
			APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
			if (PlayerController)
			{
				PlayerController->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to join session"));
	}

	OnJoinSessionCompleteEvent.Broadcast(bWasSuccessful);
}

void USessionSubsystem::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("Session destroyed successfully: %s"), *SessionName.ToString());

		UWorld* World = GetWorld();
		if (World)
		{
			APlayerController* PlayerController = World->GetFirstPlayerController();
			if (PlayerController)
			{
				// 호스트(리스너 서버)인지, 클라이언트인지에 따라 처리
				if (PlayerController->HasAuthority())
				{
					// 호스트: 서버 트래블 (listen 옵션 포함)
					World->ServerTravel(TEXT("/Game/Collaborators/LKH/Split/Main/Main?listen"));
				}
				else
				{
					// 클라이언트: 클라이언트 트래블
					PlayerController->ClientTravel(TEXT("/Game/Collaborators/LKH/Split/Main/Main"), ETravelType::TRAVEL_Absolute);
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to destroy session"));
	}

	OnDestroySessionCompleteEvent.Broadcast(bWasSuccessful);
}