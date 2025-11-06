// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/Puzzle/PuzzleTriggerBase.h"
#include "JHS/Puzzle/PuzzleBase.h"
#include "JHS/Room/RoomController.h"

// Sets default values
APuzzleTriggerBase::APuzzleTriggerBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APuzzleTriggerBase::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void APuzzleTriggerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APuzzleTriggerBase::InitializePuzzleTrigger(TObjectPtr<class ARoomController> RoomController, int32 PuzzleKey)
{
	_roomController = RoomController;
	_puzzleKey = PuzzleKey;

}

void APuzzleTriggerBase::OnTriggerEnter()
{
	_roomController->ChangePuzzleTriggerState(_puzzleKey, true);
}

void APuzzleTriggerBase::OnTriggerExit()
{
	_roomController->ChangePuzzleTriggerState(_puzzleKey, false);
}