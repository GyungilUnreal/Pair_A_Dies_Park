// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PuzzleTriggerBase.generated.h"

class ARoomController;

UCLASS()
class APuzzleTriggerBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APuzzleTriggerBase();

private:
	TObjectPtr<class APuzzleBase> _puzzle = nullptr;
	TObjectPtr<class ARoomController> _roomController = nullptr;

protected:
	int32 _puzzleKey = -1;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	void InitializePuzzleTrigger(TObjectPtr<class ARoomController> RoomController, int32 PuzzleKey);

protected:
	void OnTrigger();
};
