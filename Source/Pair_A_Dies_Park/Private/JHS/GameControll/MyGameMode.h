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

protected:
	virtual void BeginPlay() override;

public:
};
