// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "RoomController.generated.h"

class APuzzleTriggerBase;
class APuzzleActionBase;
class URoomManager;

USTRUCT(BlueprintType)
struct FPuzzleData
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle Data")
	int32 puzzleIndex = -1;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Puzzle Data")
	bool isCompleted = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle Data")
	TArray<TObjectPtr<APuzzleTriggerBase>> puzzleTriggerArray = TArray<TObjectPtr<APuzzleTriggerBase>>();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle Data")
	TArray<TObjectPtr<APuzzleActionBase>> puzzleActionArray = TArray<TObjectPtr<APuzzleActionBase>>();
};

UCLASS()
class ARoomController : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARoomController();

private:
	TObjectPtr<class URoomManager> _roomManager = nullptr;

	int32 _roomSequence = -1;

	const int32 PUZZLE_DATA_RATE = 100;

	TMap<int32, bool> _puzzleTriggerMap = TMap<int32, bool>();
	
	int32 _currentPuzzleIndex = 0;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Room Controller|Puzzle Data")
	TArray<FPuzzleData> _puzzleDataArray = TArray<FPuzzleData>();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	void InitializeRoomController();

public:
	void ChangePuzzleTriggerState(int32 PuzzleKey, bool IsTriggered);

	void ActivateNextPuzzle();
};
