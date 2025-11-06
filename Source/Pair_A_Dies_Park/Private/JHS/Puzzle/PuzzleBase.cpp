// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/Puzzle/PuzzleBase.h"
#include "JHS/Room/RoomController.h"
#include "JHS/Puzzle/Trigger/PuzzleTriggerBase.h"
#include "JHS/Puzzle/Action/PuzzleActionBase.h"

// Sets default values
APuzzleBase::APuzzleBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APuzzleBase::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void APuzzleBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APuzzleBase::InitializePuzzle(TObjectPtr<ARoomController> RoomController)
{
	_roomController = RoomController;
	_isCompleted = false;

	// Initialize Puzzle Trigger
	for (TObjectPtr<APuzzleTriggerBase> _puzzleTrigger : _puzzleTriggerArray)
	{
		_puzzleTriggerMap.Add(_puzzleTrigger, false);
		//_puzzleTrigger->InitializePuzzleTrigger(this);
	}

	if (_puzzleTriggerMap.Num() <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Puzzle trigger is not set"));
		return;
	}

	// Initialize Puzzle Action
	for (TObjectPtr<APuzzleActionBase> _puzzleAction : _puzzleActionArray)
	{
		//_puzzleAction->InitializePuzzleAction();
	}
}

void APuzzleBase::OnChangeTriggerState(TObjectPtr<APuzzleTriggerBase> PuzzleTrigger, bool IsTriggered)
{
	// Unity c#�� Dictionay.TryGet(Key, out Value)
	auto _found = _puzzleTriggerMap.Find(PuzzleTrigger);
	if (_found != nullptr)
	{
		*_found = IsTriggered;
	}
	
	for (TObjectPtr<APuzzleTriggerBase> _puzzleTrigger : _puzzleTriggerArray)
	{
		if (!_puzzleTriggerMap.Contains(_puzzleTrigger) || !_puzzleTriggerMap.FindRef(_puzzleTrigger))
			return;
	}

	_isCompleted = true;
	for (TObjectPtr<APuzzleActionBase> _puzzleAction : _puzzleActionArray)
	{
		//_puzzleAction->ExecutePuzzleAction();
	}
	
	//_roomController->OnCompletePuzzle(_puzzleIndex);
}