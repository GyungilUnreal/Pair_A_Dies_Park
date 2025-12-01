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

void APuzzleTriggerBase::OnTriggerEnter()
{
	OnChangeTriggered(true);

	if (_isChangeImmediately)
	{
		ChangeTriggerVisibility(false);
	}

	TriggerEnterEffect();
}

void APuzzleTriggerBase::OnTriggerExit()
{
	if (_isLockOnTrigger && _isTriggered)
		return;

	OnChangeTriggered(false);

	TriggerExitEffect();
}

void APuzzleTriggerBase::OnChangeTriggered(bool IsTriggered)
{
	_isTriggered = IsTriggered;
	if (_roomController == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("OnChangeTriggered: _roomController is nullptr"));
		return;
	}

	_roomController->ChangePuzzleTriggerState(_puzzleKey, _isTriggered);
}

void APuzzleTriggerBase::InitializePuzzleTrigger(TObjectPtr<ARoomController> RoomController, int32 PuzzleKey)
{
	_roomController = RoomController;
	_puzzleKey = PuzzleKey;
	_isTriggered = false;
	ChangeTriggerVisibility(true);
}

void APuzzleTriggerBase::ChangeTriggerVisibility(bool IsVisible)
{
	if (!IsVisible && !_isDeactiveOnTrigger)
		return;

	SetActorHiddenInGame(!IsVisible);
	SetActorEnableCollision(IsVisible);
	SetActorTickEnabled(IsVisible);
}