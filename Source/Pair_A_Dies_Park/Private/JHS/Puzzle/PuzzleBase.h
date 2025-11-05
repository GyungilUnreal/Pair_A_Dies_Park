// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PuzzleBase.generated.h"

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

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Delay")
	float _completeDelay = 1.0f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	void InitializePuzzle(TObjectPtr<class ARoomController> RoomController, int32 PuzzleIndex);

	bool IsCompletedPuzzle() { return _isCompleted; }

private:
	void CompletePuzzle();
};
