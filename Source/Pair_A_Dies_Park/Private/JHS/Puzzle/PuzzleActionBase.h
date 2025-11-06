// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PuzzleActionBase.generated.h"

UCLASS()
class APuzzleActionBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APuzzleActionBase();

private:
	bool _isActive = false;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Puzzle|Debug")
	bool _isDebug = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Puzzle|Debug")
	int32 _actionIndex = -1;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	void ActivatePuzzleAction();

	void DeactivatePuzzleAction();

protected:
	// 상속 구현
	virtual void OnActivatePuzzleAction() { }

	virtual void OnDeactivatePuzzleAction() { }
};
