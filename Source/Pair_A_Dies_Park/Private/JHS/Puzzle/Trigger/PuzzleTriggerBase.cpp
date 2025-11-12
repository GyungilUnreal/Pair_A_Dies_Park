// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/Puzzle/Trigger/PuzzleTriggerBase.h"
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

void APuzzleTriggerBase::InitializePuzzleTrigger(TObjectPtr<ARoomController> RoomController, int32 PuzzleKey)
{
	_roomController = RoomController;
	_puzzleKey = PuzzleKey;
}

void APuzzleTriggerBase::DeactiveTrigger()
{
	if (_isDeactiveOnTrigger)
	{
		SetActorHiddenInGame(true);
		SetActorEnableCollision(false);
		SetActorTickEnabled(false);
	}
}

void APuzzleTriggerBase::OnTriggerEnter()
{
	if (_roomController == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("OnTriggerEnter: _roomController is nullptr"));
		return;
	}

	OnChangeTriggered(true);
}

void APuzzleTriggerBase::OnTriggerExit()
{
	if (_roomController == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("OnTriggerExit: _roomController is nullptr"));
		return;
	}

	OnChangeTriggered(false);
}

void APuzzleTriggerBase::OnChangeTriggered(bool IsTriggered)
{
	_isTriggered = IsTriggered;
	_roomController->ChangePuzzleTriggerState(_puzzleKey, _isTriggered);
}