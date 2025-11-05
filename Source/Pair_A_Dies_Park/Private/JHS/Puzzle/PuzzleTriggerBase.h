// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PuzzleTriggerBase.generated.h"

UCLASS()
class APuzzleTriggerBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APuzzleTriggerBase();

private:
	TObjectPtr<class APuzzleBase> _puzzle = nullptr;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Puzzle|Delay")
	float _triggerDelay = 1.0f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	void InitializePuzzleTrigger(TObjectPtr<class APuzzleBase> Puzzle);

protected:
	void OnTrigger();
};
