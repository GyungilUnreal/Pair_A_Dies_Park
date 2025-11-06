// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/Room/RoomController.h"
#include "Kismet/GameplayStatics.h"
#include "JHS/GameControll/MyGameMode.h"
#include "JHS/GameControll/MyGameInstance.h"
#include "JHS/Room/RoomManager.h"
#include "JHS/Puzzle/Trigger/PuzzleTriggerBase.h"
#include "JHS/Puzzle/Action/PuzzleActionBase.h"

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

void ARoomController::ChangePuzzleTriggerState(int32 PuzzleKey, bool IsTriggered)
{
	// 트리거 상태 갱신
	bool* _isTriggered = nullptr;
	if (!TryGetValue(PuzzleKey, _isTriggered))
		return;

	*_isTriggered = IsTriggered;

	// 트리거 완료 체크
	bool _isActivate = true;
	int32 _puzzleIndex = PuzzleKey / PUZZLE_DATA_RATE;
	FPuzzleData& _puzzleData = _puzzleDataArray[_puzzleIndex];
	int _triggerNum = _puzzleData.puzzleTriggerArray.Num();

	// 모든 트리거가 활성화되었는지 확인
	for (int32 i = 0; i < _triggerNum; i++)
	{
		int32 _puzzleTriggerKey = _puzzleIndex * PUZZLE_DATA_RATE + i;
		bool* _triggerState = nullptr;
		
		if (!TryGetValue(_puzzleTriggerKey, _triggerState))
			return;

		if (*_triggerState == false)
		{
			_isActivate = false;
			break;
		}
	}

	// 액션 상태 변경
	if (!_isActivate && _puzzleData.isToggleTrigger)
		return;

	ChangePuzzleActionState(_puzzleIndex, _isActivate);
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

		// 퍼즐 트리거 초기화
		for (int32 j = 0; j < _puzzleData.puzzleTriggerArray.Num(); j++)
		{
			int32 _puzzleKey = i * PUZZLE_DATA_RATE + j;
			_puzzleTriggerMap.Add(_puzzleKey, false);
			_puzzleData.puzzleTriggerArray[j]->InitializePuzzleTrigger(this, _puzzleKey);
		}

		// 퍼즐 액션 초기화
		for (TObjectPtr<APuzzleActionBase> _puzzleAction : _puzzleData.puzzleActionArray)
		{
			_puzzleAction->DeactivatePuzzleAction();
		}
	}
}

bool ARoomController::TryGetValue(int32 PuzzleKey, bool*& OutValue)
{
	bool* _found = _puzzleTriggerMap.Find(PuzzleKey);
	if (_found == nullptr)
	{
		int puzzleIndex = PuzzleKey / PUZZLE_DATA_RATE;
		int triggerIndex = PuzzleKey % PUZZLE_DATA_RATE;
		UE_LOG(LogTemp, Error, TEXT("Invalid puzzle trigger key : [%d] - PuzzleIndex : [%d], TriggerIndex : [%d]"), PuzzleKey, puzzleIndex, triggerIndex);
		return false;
	}

	OutValue = _found;
	return true;
}

void ARoomController::ChangePuzzleActionState(int32 PuzzleKey, bool IsActive)
{
	int32 _puzzleIndex = PuzzleKey / PUZZLE_DATA_RATE;
	FPuzzleData& _puzzleData = _puzzleDataArray[_puzzleIndex];
	for (TObjectPtr<APuzzleActionBase> _puzzleAction : _puzzleData.puzzleActionArray)
	{
		// 활성화
		if (IsActive)
		{
			_puzzleAction->ActivatePuzzleAction();
		}
		// 비활성화
		else
		{
			_puzzleAction->DeactivatePuzzleAction();
		}
	}
}