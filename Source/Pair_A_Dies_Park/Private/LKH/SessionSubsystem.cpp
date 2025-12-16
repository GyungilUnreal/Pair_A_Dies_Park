#include "SessionSubsystem.h"
#include "SteamSessionSubsystem.h"
#include "Engine/GameInstance.h"

USessionSubsystem::USessionSubsystem()
{
}

void USessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SteamSubsystem = ResolveSteamSubsystem();
	if (USteamSessionSubsystem* Steam = SteamSubsystem.Get())
	{
		// Steam 쪽 completion 델리게이트를 레거시 델리게이트로 브리지
		Steam->OnHostSessionCompleteEvent.AddDynamic(this, &USessionSubsystem::HandleHostComplete);
		Steam->OnFindSessionsCompleteEvent.AddDynamic(this, &USessionSubsystem::HandleFindComplete);
		Steam->OnJoinSessionCompleteEvent.AddDynamic(this, &USessionSubsystem::HandleJoinComplete);
		Steam->OnDestroySessionCompleteEvent.AddDynamic(this, &USessionSubsystem::HandleDestroyComplete);
	}
}

void USessionSubsystem::Deinitialize()
{
	if (USteamSessionSubsystem* Steam = SteamSubsystem.Get())
	{
		Steam->OnHostSessionCompleteEvent.RemoveDynamic(this, &USessionSubsystem::HandleHostComplete);
		Steam->OnFindSessionsCompleteEvent.RemoveDynamic(this, &USessionSubsystem::HandleFindComplete);
		Steam->OnJoinSessionCompleteEvent.RemoveDynamic(this, &USessionSubsystem::HandleJoinComplete);
		Steam->OnDestroySessionCompleteEvent.RemoveDynamic(this, &USessionSubsystem::HandleDestroyComplete);
	}

	SteamSubsystem.Reset();
	Super::Deinitialize();
}

USteamSessionSubsystem* USessionSubsystem::ResolveSteamSubsystem() const
{
	if (const UGameInstance* GI = GetGameInstance())
	{
		return GI->GetSubsystem<USteamSessionSubsystem>();
	}
	return nullptr;
}

void USessionSubsystem::CreateSession(int32 NumPublicConnections, bool bIsLANMatch, FString ServerName)
{
	if (USteamSessionSubsystem* Steam = ResolveSteamSubsystem())
	{
		Steam->HostSessionWithServerName(NumPublicConnections, bIsLANMatch, ServerName);
		return;
	}
	OnCreateSessionCompleteEvent.Broadcast(false);
}

void USessionSubsystem::FindSessions(int32 MaxSearchResults, bool bIsLANMatch)
{
	if (USteamSessionSubsystem* Steam = ResolveSteamSubsystem())
	{
		Steam->FindSessionsWithMaxResults(bIsLANMatch, MaxSearchResults);
		return;
	}
	OnFindSessionsCompleteEvent.Broadcast(false);
}

void USessionSubsystem::JoinSession(int32 SessionIndex)
{
	if (USteamSessionSubsystem* Steam = ResolveSteamSubsystem())
	{
		Steam->JoinSessionByIndex(SessionIndex);
		return;
	}
	OnJoinSessionCompleteEvent.Broadcast(false);
}

void USessionSubsystem::DestroySession()
{
	if (USteamSessionSubsystem* Steam = ResolveSteamSubsystem())
	{
		Steam->DestroySessionIfExists();
		return;
	}
	OnDestroySessionCompleteEvent.Broadcast(false);
}

TArray<FString> USessionSubsystem::GetSessionSearchResults()
{
	if (USteamSessionSubsystem* Steam = ResolveSteamSubsystem())
	{
		return Steam->GetSessionSearchResults();
	}
	return {};
}

void USessionSubsystem::HandleHostComplete(bool bWasSuccessful)
{
	OnCreateSessionCompleteEvent.Broadcast(bWasSuccessful);
}

void USessionSubsystem::HandleFindComplete(bool bWasSuccessful)
{
	OnFindSessionsCompleteEvent.Broadcast(bWasSuccessful);
}

void USessionSubsystem::HandleJoinComplete(bool bWasSuccessful)
{
	OnJoinSessionCompleteEvent.Broadcast(bWasSuccessful);
}

void USessionSubsystem::HandleDestroyComplete(bool bWasSuccessful)
{
	OnDestroySessionCompleteEvent.Broadcast(bWasSuccessful);
}