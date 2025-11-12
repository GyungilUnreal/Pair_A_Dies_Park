// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MyGameMode.generated.h"

class URoomManager;
class UMyGameInstance;

UCLASS()
class AMyGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	AMyGameMode();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Mode|Room Manager")
	TObjectPtr<URoomManager> _roomManager;

protected:
	virtual void BeginPlay() override;

public:
	TObjectPtr<URoomManager> GetRoomManager() const { return _roomManager; }

	UFUNCTION(BlueprintCallable, Category = "Game Mode|Start Game")
	void StartGame(bool IsTutorial = false);

	void OnGameEnd();
};
