// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MyGameMode.generated.h"

class URoomManager;

UCLASS()
class AMyGameMode : public AGameMode
{
	GENERATED_BODY()
	
public:
	AMyGameMode();

private:
	TObjectPtr<URoomManager> _roomManager;

protected:
	virtual void BeginPlay() override;

public:
	TObjectPtr<URoomManager> GetRoomManager() const { return _roomManager; }

	UFUNCTION(BlueprintCallable, Category = "GameMode|Start Game")
	void StartGame(bool IsTutorial = false);

	void OnGameEnd();
};
