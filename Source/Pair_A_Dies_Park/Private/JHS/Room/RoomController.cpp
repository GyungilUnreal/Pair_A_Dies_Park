// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/Room/RoomController.h"
#include "JHS/GameControll/GameControlFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "JHS/GameControll/MyGameMode.h"
#include "JHS/GameControll/MyGameInstance.h"
#include "JHS/Puzzle/Trigger/PuzzleTriggerBase.h"
#include "JHS/Puzzle/Trigger/PresenceTrigger.h"
#include "JHS/Puzzle/Action/PuzzleActionBase.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ARoomController::ARoomController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	// 네트워크 복제 활성화
	bReplicates = true;
	SetReplicateMovement(true);
	
	// 활성화된 퍼즐 인덱스 배열 초기화
	_activePuzzleIndices.Empty();
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

void ARoomController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// 복제할 속성 등록
	DOREPLIFETIME(ARoomController, _roomData);
	DOREPLIFETIME(ARoomController, _roomType);
	DOREPLIFETIME(ARoomController, _puzzleDataArray);
	DOREPLIFETIME(ARoomController, _roomClearDoor);
	DOREPLIFETIME(ARoomController, _activePuzzleIndices);
}

void ARoomController::Server_ChangePuzzleTriggerState_Implementation(int32 PuzzleKey, bool IsTriggered)
{
	ChangePuzzleTriggerState(PuzzleKey, IsTriggered);
}

void ARoomController::ChangePuzzleTriggerState(int32 PuzzleKey, bool IsTriggered)
{
	// 서버에서만 실행되도록 체크
	if (!HasAuthority())
	{
		Server_ChangePuzzleTriggerState(PuzzleKey, IsTriggered);
		return;
	}
	
	// 룸 클리어 문
	if (PuzzleKey == ROOM_CLEAR_DOOR_KEY)
	{
		if (!IsTriggered)
			return;

		TObjectPtr<URoomSubsystem> _roomSubSystem = nullptr;
		if (!UGameControlFunctionLibrary::TryGetRoomSubSystem(nullptr, _roomSubSystem))
			return;

		_roomSubSystem->OnCompletedRoom((int32)_roomType);
		return;
	}

	// 트리거 상태 갱신
	bool* _isTriggered = nullptr;
	if (!TryGetValue(PuzzleKey, _isTriggered))
		return;

	*_isTriggered = IsTriggered;

	// 트리거 완료 체크
	bool _isActivate = true;
	int32 _puzzleIndex = PuzzleKey / PUZZLE_DATA_RATE;
	FPuzzleData& _puzzleData = _puzzleDataArray[_puzzleIndex];
	int32 _triggerNum = _puzzleData.PuzzleTriggerArray.Num();

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
	if (!_isActivate && _puzzleData.IsToggleTrigger)
		return;

	ChangePuzzleActionState(_puzzleIndex, _isActivate);
}

void ARoomController::InitializeRoomController()
{
	// 룸 클리어 문 트리거
	_puzzleTriggerMap.Add(ROOM_CLEAR_DOOR_KEY, false);
	
	// _roomClearDoor가 null인지 확인
	if (_roomClearDoor != nullptr)
	{
		_roomClearDoor->InitializePuzzleTrigger(this, ROOM_CLEAR_DOOR_KEY);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("_roomClearDoor is nullptr"));
	}

	if (_puzzleDataArray.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Trigger is not set"));
		return;
	}

	// 퍼즐 데이터 초기화
	for (int32 i = 0; i < _puzzleDataArray.Num(); i++)
	{
		FPuzzleData& _puzzleData = _puzzleDataArray[i];

		// 퍼즐 트리거 초기화
		for (int32 j = 0; j < _puzzleData.PuzzleTriggerArray.Num(); j++)
		{
			int32 _puzzleKey = i * PUZZLE_DATA_RATE + j;
			TObjectPtr<APuzzleTriggerBase> _trigger = _puzzleData.PuzzleTriggerArray[j];
			if (_trigger == nullptr)
				continue;

			_trigger->InitializePuzzleTrigger(this, _puzzleKey);
			_puzzleTriggerMap.Add(_puzzleKey, false);
		}

		// 퍼즐 액션 초기화
		for (TObjectPtr<APuzzleActionBase> _puzzleAction : _puzzleData.PuzzleActionArray)
		{
			if (_puzzleAction == nullptr)
				continue;

			_puzzleAction->InitializePuzzleAction();
		}
	}
}

bool ARoomController::TryGetValue(int32 PuzzleKey, bool*& OutValue)
{
	bool* _found = _puzzleTriggerMap.Find(PuzzleKey);
	if (_found == nullptr)
	{
		int32 _puzzleIndex = PuzzleKey / PUZZLE_DATA_RATE;
		int32 _triggerIndex = PuzzleKey % PUZZLE_DATA_RATE;
		UE_LOG(LogTemp, Error, TEXT("Invalid puzzle trigger key : [%d] - PuzzleIndex : [%d], TriggerIndex : [%d]"), PuzzleKey, _puzzleIndex, _triggerIndex);
		return false;
	}

	OutValue = _found;
	return true;
}

void ARoomController::Server_ChangePuzzleActionState_Implementation(int32 PuzzleIndex, bool IsActive)
{
	ChangePuzzleActionState(PuzzleIndex, IsActive);
}

bool ARoomController::IsPuzzleActive(int32 PuzzleIndex) const
{
	return _activePuzzleIndices.Contains(PuzzleIndex);
}

void ARoomController::OnRep_ActivePuzzleIndices()
{
	// 클라이언트에서 실행되는 코드
	if (HasAuthority())
		return; // 서버에서는 이미 처리됨
	
	// 활성화된 모든 퍼즐 인덱스에 대해 액션 실행
	for (int32 PuzzleIndex : _activePuzzleIndices)
	{
		if (PuzzleIndex < 0 || PuzzleIndex >= _puzzleDataArray.Num())
			continue;
			
		FPuzzleData& _puzzleData = _puzzleDataArray[PuzzleIndex];
		
		// 액션 활성화 - 클라이언트에서 실행
		for (TObjectPtr<APuzzleActionBase> _puzzleAction : _puzzleData.PuzzleActionArray)
		{
			if (_puzzleAction)
			{
				_puzzleAction->ActivatePuzzleAction();
			}
		}
		
		// 트리거 비활성화
		if (_puzzleData.IsToggleTrigger)
		{
			for (TObjectPtr<APuzzleTriggerBase> _puzzleTrigger : _puzzleData.PuzzleTriggerArray)
			{
				if (_puzzleTrigger)
				{
					_puzzleTrigger->DeactiveTrigger();
				}
			}
		}
	}
	
	// 비활성화된 퍼즐에 대한 처리
	for (int32 i = 0; i < _puzzleDataArray.Num(); i++)
	{
		if (!_activePuzzleIndices.Contains(i))
		{
			FPuzzleData& _puzzleData = _puzzleDataArray[i];
			
			// 액션 비활성화 - 클라이언트에서 실행
			for (TObjectPtr<APuzzleActionBase> _puzzleAction : _puzzleData.PuzzleActionArray)
			{
				if (_puzzleAction)
				{
					_puzzleAction->DeactivatePuzzleAction();
				}
			}
		}
	}
}

void ARoomController::ChangePuzzleActionState(int32 PuzzleIndex, bool IsActive)
{
	// 서버에서만 실행되도록 체크
	if (!HasAuthority())
	{
		Server_ChangePuzzleActionState(PuzzleIndex, IsActive);
		return;
	}
	
	if (PuzzleIndex < 0 || PuzzleIndex >= _puzzleDataArray.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("ChangePuzzleActionState: Invalid puzzle index %d"), PuzzleIndex);
		return;
	}
	
	FPuzzleData& _puzzleData = _puzzleDataArray[PuzzleIndex];

	if (IsActive)
	{
		// 액션 활성화 상태를 배열에 추가 (복제를 위함)
		if (!_activePuzzleIndices.Contains(PuzzleIndex))
		{
			_activePuzzleIndices.Add(PuzzleIndex);
			// 복제 변수가 변경되었음을 알림
			OnRep_ActivePuzzleIndices();
		}
		
		// 액션 활성화 - 서버에서만 실행
		for (TObjectPtr<APuzzleActionBase> _puzzleAction : _puzzleData.PuzzleActionArray)
		{
			if (_puzzleAction)
			{
				_puzzleAction->ActivatePuzzleAction();
			}
		}

		// 트리거 비활성화
		if (_puzzleData.IsToggleTrigger)
		{
			for (TObjectPtr<APuzzleTriggerBase> _puzzleTrigger : _puzzleData.PuzzleTriggerArray)
			{
				if (_puzzleTrigger)
				{
					_puzzleTrigger->DeactiveTrigger();
				}
			}
		}
	}
	else
	{
		// 액션 비활성화 상태를 배열에서 제거
		if (_activePuzzleIndices.Contains(PuzzleIndex))
		{
			_activePuzzleIndices.Remove(PuzzleIndex);
			// 복제 변수가 변경되었음을 알림
			OnRep_ActivePuzzleIndices();
		}
		
		// 액션 비활성화 - 서버에서만 실행
		for (TObjectPtr<APuzzleActionBase> _puzzleAction : _puzzleData.PuzzleActionArray)
		{
			if (_puzzleAction)
			{
				_puzzleAction->DeactivatePuzzleAction();
			}
		}
	}
}