// Fill out your copyright notice in the Description page of Project Settings.

#include "Lobby_GameMode.h"
#include "Engine/World.h"
#include "LobbyPlayerState.h"
#include "OnlineSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"

void ALobby_GameMode::BP_ServerTravel(const FString& MapPath, bool bListen, bool bAbsolute)
{
    // 서버에서만 의미 있게
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // 맵 이름에 옵션 붙이기
    FString FinalURL = MapPath;

    // 이미 ?가 없고, listen을 원하면 붙여줌
    if (bListen)
    {
        // 사용자가 이미 ?를 넣었는지 체크
        if (!FinalURL.Contains(TEXT("?")))
        {
            FinalURL.Append(TEXT("?listen"));
        }
        else
        {
            // 이미 옵션이 있다면 뒤에 &listen 붙여도 됨
            FinalURL.Append(TEXT("&listen"));
        }
    }

    // 실제 서버 트래블 호출
    // bAbsolute = true 이면 절대 경로, 아니면 상대
    World->ServerTravel(FinalURL, bAbsolute);
}

void ALobby_GameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    ALobbyPlayerState* PS = NewPlayer ? NewPlayer->GetPlayerState<ALobbyPlayerState>() : nullptr;
    if (!PS) return;

    // 닉네임은 일단 PlayerState 기본값을 써도 됨(대부분 Steam 닉네임이 들어있음)
    const FString Nick = PS->GetPlayerName();

    // SteamID는 OSS에서 얻는 걸 추천 (환경에 따라 빈 문자열일 수 있음)
    FString SteamIdStr;

    if (IOnlineSubsystem* OSS = IOnlineSubsystem::Get())
    {
        if (IOnlineIdentityPtr Identity = OSS->GetIdentityInterface())
        {
            TSharedPtr<const FUniqueNetId> Id = Identity->GetUniquePlayerId(0);
            if (Id.IsValid())
            {
                SteamIdStr = Id->ToString();
            }
        }
    }

    PS->ServerSetSteamIdentity(SteamIdStr, Nick);
}