// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/Room/RoomController.h"
#include "Kismet/GameplayStatics.h"
#include "JHS/GameControll/MyGameMode.h"
#include "JHS/GameControll/MyGameInstance.h"
#include "JHS/Room/RoomManager.h"
#include "JHS/Puzzle/PuzzleBase.h"
#include "JHS/Puzzle/PuzzleTriggerBase.h"
#include "JHS/Puzzle/PuzzleActionBase.h"

// Sets default values
ARoomController::ARoomController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ARoomController::BeginPlay()
{
	Super::BeginPlay();

	InitializeRoomController();
}

// Called every frame
void ARoomController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ARoomController::InitializeRoomController()
{
	TObjectPtr<AMyGameMode> _gameMode = Cast<AMyGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (!_gameMode)
	{
		UE_LOG(LogTemp, Error, TEXT("GameMode is nullptr"));
		return;
	}

	_roomManager = _gameMode->GetRoomManager();
	if (!_roomManager)
	{
		UE_LOG(LogTemp, Error, TEXT("RoomManager is nullptr"));
		return;
	}

	TObjectPtr<UMyGameInstance> _gameInstance = Cast<UMyGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (!_gameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("GameInstance is nullptr"));
		return;
	}

	_roomSequence = _gameInstance->GetCurrentRoomSequence();
	UE_LOG(LogTemp, Warning, TEXT("Initialize Room %d"), _roomSequence);

	// 퍼즐 데이터 초기화
	for (int32 i = 0; i < _puzzleDataArray.Num(); i++)
	{
		FPuzzleData& _puzzleData = _puzzleDataArray[i];
		_puzzleData.puzzleIndex = i;

		// 퍼즐 트리거 초기화
		for (int32 j = 0; j < _puzzleData.puzzleTriggerArray.Num(); j++)
		{
			int32 _puzzleKey = i * PUZZLE_DATA_RATE + j;
			_puzzleTriggerMap.Add(_puzzleKey, false);
			_puzzleData.puzzleTriggerArray[j]->InitializePuzzleTrigger(this, _puzzleKey);
		}

		// 퍼즐 액션 초기화
		for (int32 j = 0; j < _puzzleData.puzzleActionArray.Num(); j++)
		{
			_puzzleData.puzzleActionArray[j]->InitializePuzzleAction();
		}
	}
	
	// 첫 번째 퍼즐 활성화
	_currentPuzzleIndex = -1;
	ActivateNextPuzzle();
}

void ARoomController::ChangePuzzleTriggerState(int32 PuzzleKey, bool IsTriggered)
{
	int32 _puzzleIndex = PuzzleKey / PUZZLE_DATA_RATE;
	auto _found = _puzzleTriggerMap.Find(PuzzleKey);
	if (_found == nullptr)
	{
		int32 _triggerIndex = PuzzleKey % PUZZLE_DATA_RATE;

		UE_LOG(LogTemp, Error, TEXT("Invalid puzzle key : [%d] - PuzzleIndex: %d, TriggerIndex: %d"), PuzzleKey, _puzzleIndex, _triggerIndex);
		return;
	}

	*_found = IsTriggered;
	
	int _triggerNum = _puzzleDataArray[_puzzleIndex].puzzleTriggerArray.Num();
	for (int32 i = 0; i < _triggerNum; i++)
	{
		int32 _puzzleTriggerKey = _puzzleIndex * PUZZLE_DATA_RATE + i;
		_found = _puzzleTriggerMap.Find(_puzzleTriggerKey);
		if (_found == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("Invalid puzzle trigger key : [%d]"), _puzzleTriggerKey);
			return;
		}

		if (*_found != IsTriggered)
			return;
	}
	
	ActivateNextPuzzle();
}

void ARoomController::ActivateNextPuzzle()
{
	_currentPuzzleIndex++;
	if (_currentPuzzleIndex < 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Invalid puzzle index: %d"), _currentPuzzleIndex);
		return;
	}

	if (_currentPuzzleIndex >= _puzzleDataArray.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("All puzzles completed"));
		_roomManager->OnCompletedRoom(_roomSequence);
		return;
	}
	
	FPuzzleData& _puzzleData = _puzzleDataArray[_currentPuzzleIndex];
	for (TObjectPtr<APuzzleActionBase> _puzzleAction : _puzzleData.puzzleActionArray)
	{
		_puzzleAction->ExecutePuzzleAction();
	}
}