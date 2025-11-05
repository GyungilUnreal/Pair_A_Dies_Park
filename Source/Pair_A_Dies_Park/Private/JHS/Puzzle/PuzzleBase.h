// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PuzzleBase.generated.h"

class APuzzleTriggerBase;
class APuzzleActionBase;

UCLASS()
class APuzzleBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APuzzleBase();

private:
	TObjectPtr<class ARoomController> _roomController = nullptr;

	int32 _puzzleIndex = -1;

	bool _isCompleted = false;

#pragma region Puzzle Trigger
	TMap<TObjectPtr<APuzzleTriggerBase>, bool> _puzzleTriggerMap = TMap<TObjectPtr<APuzzleTriggerBase>, bool>();
#pragma endregion Puzzle Trigger

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Delay")
	float _completeDelay = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Trigger")
	TArray<TObjectPtr<APuzzleTriggerBase>> _puzzleTriggerArray = TArray<TObjectPtr<APuzzleTriggerBase>>();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Action")
	TArray<TObjectPtr<APuzzleActionBase>> _puzzleActionArray = TArray<TObjectPtr<APuzzleActionBase>>();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	void InitializePuzzle(TObjectPtr<ARoomController> RoomController);

	void OnChangeTriggerState(TObjectPtr<APuzzleTriggerBase> PuzzleTrigger, bool IsTriggered);

	bool IsCompletedPuzzle() { return _isCompleted; }
};
