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

	FPuzzleData* _puzzleDataPtr = nullptr;
	int32 _puzzleIndex = -1;
	int32 _triggerIndex = -1;
	if (!TryGetPuzzleData(PuzzleKey, _puzzleDataPtr, _puzzleIndex, _triggerIndex))
		return;

	// 트리거 타이머 처리 (서버 RPC 함수 호출)
	if (IsTriggered && _puzzleDataPtr->PuzzleTriggerArray[_triggerIndex]->IsTimerTrigger())
	{
		SetTriggerTimer(_puzzleIndex, _triggerIndex);
	}

	bool _isActivate = true;
	// 모든 트리거가 활성화되었는지 확인
	for (int32 i = 0; i < _puzzleDataPtr->PuzzleTriggerArray.Num(); i++)
	{
		TObjectPtr<APuzzleTriggerBase> _trigger = _puzzleDataPtr->PuzzleTriggerArray[i];
		if (!_trigger->IsTriggered())
		{
			_isActivate = false;
		}
	}

	// 모든 트리거 만족 시 트리거 타이머 해제 (서버 RPC 함수 호출)
	if (_isActivate && CheckAllTimerTriggerValue(_puzzleDataPtr, true))
	{
		ClearTriggerTimer(_puzzleIndex);
	}

	ChangePuzzleActionState(_puzzleIndex, _isActivate);
}

void ARoomController::OnActionDeactivated(int32 PuzzleIndex)
{
	FPuzzleData* _puzzleData = nullptr;
	int32 _puzzleIndex = -1;
	int32 _rawIndex = -1;
	if (!TryGetPuzzleData(PuzzleIndex, _puzzleData, _puzzleIndex, _rawIndex))
		return;

	bool _isAllActionDeactivated = true;
	for (TObjectPtr<APuzzleActionBase> _puzzleAction : _puzzleData->PuzzleActionArray)
	{
		if (_puzzleAction == nullptr)
			continue;

		if (_puzzleAction->IsActivate())
		{
			_isAllActionDeactivated = false;
			break;
		}
	}

	if (_isAllActionDeactivated)
	{
		for (TObjectPtr<APuzzleTriggerBase> _puzzleTrigger : _puzzleData->PuzzleTriggerArray)
		{
			if (_puzzleTrigger == nullptr)
				continue;

			_puzzleTrigger->ChangeTriggerVisibility(true);
		}
	}
}

void ARoomController::InitializeRoomController()
{
	if (_roomClearDoor == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("_roomClearDoor is nullptr"));
	}
	else
	{
		_roomClearDoor->InitializePuzzleTrigger(this, ROOM_CLEAR_DOOR_KEY);
	}

	if (_puzzleDataArray.Num() <= 0)
		return;

	// 퍼즐 데이터 초기화
	for (int32 i = 0; i < _puzzleDataArray.Num(); i++)
	{
		FPuzzleData* _puzzleDataPtr = &_puzzleDataArray[i];
		_puzzleDataPtr->IsPuzzleActivated = false;
		_puzzleDataPtr->TimerTriggerMap.Empty();

		// 퍼즐 트리거 초기화
		for (int32 j = 0; j < _puzzleDataPtr->PuzzleTriggerArray.Num(); j++)
		{
			int32 _puzzleKey = i * PUZZLE_DATA_RATE + j;
			TObjectPtr<APuzzleTriggerBase> _trigger = _puzzleDataPtr->PuzzleTriggerArray[j];
			if (_trigger == nullptr)
				continue;

			_trigger->InitializePuzzleTrigger(this, _puzzleKey);

			if (_trigger->IsTimerTrigger())
			{
				_puzzleDataPtr->TimerTriggerMap.Add(j, false);
			}
		}

		ClearTriggerTimerHandle(_puzzleDataPtr);

		// 퍼즐 액션 초기화
		for (TObjectPtr<APuzzleActionBase> _puzzleAction : _puzzleDataPtr->PuzzleActionArray)
		{
			if (_puzzleAction == nullptr)
				continue;

			_puzzleAction->InitializePuzzleAction();
		}
	}
}

void ARoomController::Server_ChangePuzzleActionState_Implementation(int32 PuzzleIndex, bool IsActive)
{
	ChangePuzzleActionState(PuzzleIndex, IsActive);
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
	
	// 액션 활성화 상태 변경 (복제를 위함)
	bool _stateChanged = false;
	
	if (IsActive && !_activePuzzleIndices.Contains(PuzzleIndex))
	{
		_activePuzzleIndices.Add(PuzzleIndex);
		_stateChanged = true;
	}
	else if (!IsActive && _activePuzzleIndices.Contains(PuzzleIndex))
	{
		_activePuzzleIndices.Remove(PuzzleIndex);
		_stateChanged = true;
	}
	
	// 상태가 변경된 경우에만 복제 변수 변경 알림
	if (_stateChanged)
	{
		OnRep_ActivePuzzleIndices();
	}
	
	// 실제 액션 상태 적용
	ApplyPuzzleActionState(PuzzleIndex, IsActive);
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

		ApplyPuzzleActionState(PuzzleIndex, true);
	}

	// 비활성화된 퍼즐에 대한 처리
	for (int32 i = 0; i < _puzzleDataArray.Num(); i++)
	{
		if (!_activePuzzleIndices.Contains(i))
		{
			ApplyPuzzleActionState(i, false);
		}
	}
}

void ARoomController::ApplyPuzzleActionState(int32 PuzzleIndex, bool IsActive)
{
	if (PuzzleIndex < 0 || PuzzleIndex >= _puzzleDataArray.Num())
		return;

	// 배열 요소를 포인터로 캐싱하여 직접 접근 가능하게 함
	FPuzzleData* _puzzleDataPtr = &_puzzleDataArray[PuzzleIndex];
	
	// 이미 활성화된 퍼즐이고 잠금 설정이 되어있다면 비활성화 방지
	if (!IsActive && _puzzleDataPtr->IsPuzzleActivated && _puzzleDataPtr->IsLockActivatedAction)
		return;

	// 액션 상태 적용
	for (TObjectPtr<APuzzleActionBase> _puzzleAction : _puzzleDataPtr->PuzzleActionArray)
	{
		if (!_puzzleAction)
			continue;

		if (IsActive)
			_puzzleAction->ActivatePuzzleAction();
		else
			_puzzleAction->DeactivatePuzzleAction();
	}

	// 트리거 상태 적용
	for (TObjectPtr<APuzzleTriggerBase> _puzzleTrigger : _puzzleDataPtr->PuzzleTriggerArray)
	{
		if (_puzzleTrigger == nullptr)
			continue;

		_puzzleTrigger->ChangeTriggerVisibility(!IsActive);
	}

	// 활성화 상태 직접 변경 (포인터를 통해 원본 배열 요소 수정)
	_puzzleDataPtr->IsPuzzleActivated = IsActive;
}

bool ARoomController::TryGetPuzzleData(int32 PuzzleKey, FPuzzleData*& OutPuzzleData, int32& OutPuzzleIndex, int32& OutRawIndex)
{
	OutPuzzleIndex = PuzzleKey / PUZZLE_DATA_RATE;
	OutRawIndex = PuzzleKey % PUZZLE_DATA_RATE;
	
	if (OutPuzzleIndex < 0 || OutPuzzleIndex >= _puzzleDataArray.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("TryGetPuzzleData: Invalid puzzle index %d"), OutPuzzleIndex);
		return false;
	}

	OutPuzzleData = &_puzzleDataArray[OutPuzzleIndex];
	return true;
}

bool ARoomController::CheckAllTimerTriggerValue(FPuzzleData* PuzzleDataPtr, bool hopeResult)
{
	if (PuzzleDataPtr->TimerTriggerMap.Num() <= 0)
		return false;

	for (auto& _pair : PuzzleDataPtr->TimerTriggerMap)
	{
		if (_pair.Value != hopeResult)
			return false;
	}

	return true;
}

void ARoomController::Server_SetTriggerTimer_Implementation(int32 PuzzleIndex, int32 TriggerIndex)
{
	SetTriggerTimer(PuzzleIndex, TriggerIndex);
}

void ARoomController::SetTriggerTimer(int32 PuzzleIndex, int32 TriggerIndex)
{
	// 서버에서만 실행되도록 체크
	if (!HasAuthority())
	{
		Server_SetTriggerTimer(PuzzleIndex, TriggerIndex);
		return;
	}
	
	if (PuzzleIndex < 0 || PuzzleIndex >= _puzzleDataArray.Num())
		return;
	
	FPuzzleData* _puzzleDataPtr = &_puzzleDataArray[PuzzleIndex];
	
	// 타이머 트리거 맵에 해당 트리거 인덱스가 없으면 리턴
	if (!_puzzleDataPtr->TimerTriggerMap.Contains(TriggerIndex))
		return;
	
	// 모든 타이머 트리거가 false인지 확인
	if (CheckAllTimerTriggerValue(_puzzleDataPtr, false))
	{
		_puzzleDataPtr->TimerTriggerMap[TriggerIndex] = true;
	}
	
	// 타이머 설정
	GetWorld()->GetTimerManager().SetTimer(
		_puzzleDataPtr->TriggerTimerHandle, 
		FTimerDelegate::CreateUObject(this, &ARoomController::OnPuzzleTriggerTimeOut, _puzzleDataPtr), 
		_puzzleDataPtr->TimerLimit, 
		false
	);
}

void ARoomController::Server_ClearTriggerTimer_Implementation(int32 PuzzleIndex)
{
	ClearTriggerTimer(PuzzleIndex);
}

void ARoomController::ClearTriggerTimer(int32 PuzzleIndex)
{
	// 서버에서만 실행되도록 체크
	if (!HasAuthority())
	{
		Server_ClearTriggerTimer(PuzzleIndex);
		return;
	}
	
	if (PuzzleIndex < 0 || PuzzleIndex >= _puzzleDataArray.Num())
		return;
	
	FPuzzleData* _puzzleDataPtr = &_puzzleDataArray[PuzzleIndex];
	ClearTriggerTimerHandle(_puzzleDataPtr);
}

void ARoomController::OnPuzzleTriggerTimeOut(FPuzzleData* PuzzleDataPtr)
{
	// 서버에서만 실행되는지 확인
	if (!HasAuthority())
		return;
		
	// 타이머 핸들 초기화
	ClearTriggerTimerHandle(PuzzleDataPtr);
	
	// 모든 타이머 트리거 리셋
	for (auto& _pair : PuzzleDataPtr->TimerTriggerMap)
	{
		// 트리거가 유효한지 확인
		if (PuzzleDataPtr->PuzzleTriggerArray.IsValidIndex(_pair.Key) && 
			PuzzleDataPtr->PuzzleTriggerArray[_pair.Key] != nullptr)
		{
			// ResetTrigger 호출 (내부적으로 Multicast_ResetTrigger를 호출)
			PuzzleDataPtr->PuzzleTriggerArray[_pair.Key]->ResetTrigger();
		}
	}
}

void ARoomController::ClearTriggerTimerHandle(FPuzzleData* PuzzleDataPtr)
{
	GetWorld()->GetTimerManager().ClearTimer(PuzzleDataPtr->TriggerTimerHandle);
	// 모든 타이머 트리거 초기화
	for (auto& _pair : PuzzleDataPtr->TimerTriggerMap)
	{
		_pair.Value = false;
	}
}

//bool ARoomController::TryGetValue(int32 PuzzleKey, bool*& OutValue)
//{
//	bool* _found = _puzzleTriggerMap.Find(PuzzleKey);
//	if (_found == nullptr)
//	{
//		int32 _puzzleIndex = PuzzleKey / PUZZLE_DATA_RATE;
//		int32 _triggerIndex = PuzzleKey % PUZZLE_DATA_RATE;
//		UE_LOG(LogTemp, Error, TEXT("Invalid puzzle trigger key : [%d] - PuzzleIndex : [%d], TriggerIndex : [%d]"), PuzzleKey, _puzzleIndex, _triggerIndex);
//		return false;
//	}
//
//	OutValue = _found;
//	return true;
//}