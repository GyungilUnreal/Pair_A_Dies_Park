// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "Lobby_GameMode.generated.h"

/**
 * 
 */
UCLASS()
class ALobby_GameMode : public AGameModeBase
{
    GENERATED_BODY()

public:

    // 블루프린트에서 부를 수 있는 서버 트래블 래퍼
    UFUNCTION(BlueprintCallable, Category = "Travel")
    void BP_ServerTravel(const FString& MapPath, bool bListen = true, bool bAbsolute = false);
};