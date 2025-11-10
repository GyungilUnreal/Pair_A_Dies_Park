// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Pair_A_Dies_Park/Pair_A_Dies_ParkCharacter.h"

#include "PlayerBase.generated.h"

UCLASS()
class APlayerBase : public APair_A_Dies_ParkCharacter
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APlayerBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void SetPlayerScale(float PlayerScale);
};
