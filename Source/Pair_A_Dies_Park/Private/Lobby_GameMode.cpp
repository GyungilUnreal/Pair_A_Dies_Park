// Fill out your copyright notice in the Description page of Project Settings.


#include "Lobby_GameMode.h"
#include "Engine/World.h"

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
