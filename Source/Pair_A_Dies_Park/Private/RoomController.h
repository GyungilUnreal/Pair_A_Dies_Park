// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#include "RoomController.generated.h"

UCLASS()
class ARoomController : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ARoomController();

private:
	TArray<TObjectPtr<class APuzzleBase>> _puzzleArray = TArray<TObjectPtr<class APuzzleBase>>();

	int32 _roomSequence = -1;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

public:
	void InitializeRoomController(int32 RoomSequence);

	void OnCompletePuzzle(int32 completedPuzzleIndex);

private:
	TArray<TObjectPtr<class APuzzleBase>> SearchPuzzle();

	TObjectPtr<class URoomManager> GetRoomManager() const;



	// Debug
protected:
	UFUNCTION(BlueprintCallable)
	void StartSecondRoom();

	void StartNextRoom();
};
