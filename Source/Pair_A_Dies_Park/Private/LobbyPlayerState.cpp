#include "LobbyPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Pawn.h"

ALobbyPlayerState::ALobbyPlayerState()
{
	bReplicates = true;
}

void ALobbyPlayerState::ServerSetSteamIdentity(const FString& InSteamId, const FString& InSteamNickname)
{
	// 서버 권한 체크 (GameMode/PostLogin에서 호출할 거라 보통 서버임)
	if (!HasAuthority())
	{
		return;
	}

	SteamId = InSteamId;
	SteamNickname = InSteamNickname;

	// 서버에서도 즉시 반응이 필요하면 OnRep를 직접 호출해도 됨(선택)
	OnRep_SteamId();
	OnRep_SteamNickname();
}

void ALobbyPlayerState::OnRep_SteamId()
{
	// 바뀌었음을 알리는 용도 (UI가 PlayerState를 보고 갱신)
	OnSteamIdChanged.Broadcast(SteamId);
}

void ALobbyPlayerState::OnRep_SteamNickname()
{
	OnSteamNicknameChanged.Broadcast(SteamNickname);
}

void ALobbyPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALobbyPlayerState, SteamId);
	DOREPLIFETIME(ALobbyPlayerState, SteamNickname);
}