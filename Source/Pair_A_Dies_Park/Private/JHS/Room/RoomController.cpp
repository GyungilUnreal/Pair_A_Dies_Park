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
	DOREPLIFETIME(ARoomController, _puzzleCategoryArray);
	DOREPLIFETIME(ARoomController, _roomClearDoor);
	DOREPLIFETIME(ARoomController, _activePuzzleIndices);
}

#pragma region Change Trigger State
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

	FPuzzleGroup* _puzzleGroupPtr = nullptr;
	int32 _puzzleIndex = -1;
	int32 _triggerIndex = -1;
	if (!TryGetPuzzleGroup(PuzzleKey, _puzzleGroupPtr, _puzzleIndex, _triggerIndex))
		return;

	// 트리거 타이머 처리 (서버 RPC 함수 호출)
	/*if (IsTriggered && _puzzleGroupPtr->PuzzleTriggerArray[_triggerIndex]->IsTimerTrigger())
	{
		SetTriggerTimer(_puzzleIndex, _triggerIndex);
	}*/

	// 모든 트리거가 활성화되었는지 확인
	bool _isActivate = true;
	int32 _triggerNum = _puzzleGroupPtr->PuzzleTriggerArray.Num();
	int32 _nullptrTriggerCount = 0;
	for (int32 i = 0; i < _triggerNum; i++)
	{
		TObjectPtr<APuzzleTriggerBase> _trigger = _puzzleGroupPtr->PuzzleTriggerArray[i];
		if (_trigger == nullptr)
		{
			_nullptrTriggerCount++;
			continue;
		}

		// 모든 트리거 공백
		if (_nullptrTriggerCount == _triggerNum)
		{
			_isActivate = false;
			break;
		}

		if (!_trigger->IsTriggered())
		{
			_isActivate = false;
		}
	}

	// 모든 트리거 만족 시 트리거 타이머 해제 (서버 RPC 함수 호출)
	/*if (_isActivate && CheckAllTimerTriggerValue(_puzzleDataPtr, true))
	{
		ClearTriggerTimer(_puzzleIndex);
	}*/

	ChangePuzzleActionState(_puzzleIndex, _isActivate);
}

void ARoomController::Server_ChangePuzzleTriggerState_Implementation(int32 PuzzleKey, bool IsTriggered)
{
	ChangePuzzleTriggerState(PuzzleKey, IsTriggered);
}
#pragma endregion Change Trigger State

#pragma region Change Action State
void ARoomController::ChangePuzzleActionState(int32 PuzzleIndex, bool IsActivate)
{
	// 서버에서만 실행되도록 체크
	if (!HasAuthority())
	{
		Server_ChangePuzzleActionState(PuzzleIndex, IsActivate);
		return;
	}

	if (PuzzleIndex < 0 || PuzzleIndex >= _puzzleGroupArray.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("ChangePuzzleActionState: Invalid puzzle index %d"), PuzzleIndex);
		return;
	}

	FPuzzleGroup* _puzzleGroupPtr = &_puzzleGroupArray[PuzzleIndex];
	
	// 서버에서 먼저 액션 결과 변경
	_puzzleGroupPtr->IsPuzzleActivated = IsActivate;
	
	// 서버에서 실제 액션 상태 적용
	ApplyPuzzleActionState(PuzzleIndex, IsActivate);

	// 액션 활성화 상태 변경 (복제를 위함)
	bool _stateChanged = false;

	if (IsActivate && !_activePuzzleIndices.Contains(PuzzleIndex))
	{
		_activePuzzleIndices.Add(PuzzleIndex);
		_stateChanged = true;
	}
	else if (!IsActivate && _activePuzzleIndices.Contains(PuzzleIndex))
	{
		_activePuzzleIndices.Remove(PuzzleIndex);
		_stateChanged = true;
	}

	// 상태가 변경된 경우에만 복제 변수 변경 알림 (클라이언트에 전파)
	if (_stateChanged)
	{
		OnRep_ActivePuzzleIndices();
	}
}

void ARoomController::Server_ChangePuzzleActionState_Implementation(int32 PuzzleIndex, bool IsActivate)
{
	ChangePuzzleActionState(PuzzleIndex, IsActivate);
}

void ARoomController::OnRep_ActivePuzzleIndices()
{
	// 클라이언트에서 실행되는 코드
	if (HasAuthority())
		return; // 서버에서는 이미 처리됨

	// 활성화된 모든 퍼즐 인덱스에 대해 액션 실행
	for (int32 PuzzleIndex : _activePuzzleIndices)
	{
		if (PuzzleIndex < 0 || PuzzleIndex >= _puzzleGroupArray.Num())
			continue;

		ApplyPuzzleActionState(PuzzleIndex, true);
	}

	// 비활성화된 퍼즐에 대한 처리
	for (int32 i = 0; i < _puzzleGroupArray.Num(); i++)
	{
		if (!_activePuzzleIndices.Contains(i))
		{
			ApplyPuzzleActionState(i, false);
		}
	}
}

void ARoomController::ApplyPuzzleActionState(int32 PuzzleIndex, bool IsActivate)
{
	if (PuzzleIndex < 0 || PuzzleIndex >= _puzzleGroupArray.Num())
		return;

	// 배열 요소를 포인터로 캐싱하여 직접 접근 가능하게 함
	FPuzzleGroup* _puzzleGroupPtr = &_puzzleGroupArray[PuzzleIndex];

	// 액션 상태 적용
	for (TObjectPtr<APuzzleActionBase> _puzzleAction : _puzzleGroupPtr->PuzzleActionArray)
	{
		if (!_puzzleAction)
			continue;

		if (IsActivate)
			_puzzleAction->ActivatePuzzleAction();
		else
			_puzzleAction->DeactivatePuzzleAction();
	}

	// 트리거 상태 적용
	for (TObjectPtr<APuzzleTriggerBase> _puzzleTrigger : _puzzleGroupPtr->PuzzleTriggerArray)
	{
		if (_puzzleTrigger == nullptr)
			continue;

		_puzzleTrigger->OnChangeAction(IsActivate);
	}

	// 활성화 상태 직접 변경 (포인터를 통해 원본 배열 요소 수정)
	_puzzleGroupPtr->IsPuzzleActivated = IsActivate;
}
#pragma endregion Change Action State

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

	if (_puzzleCategoryArray.Num() <= 0)
		return;

	// _puzzleGroupArray 초기화
	_puzzleGroupArray.Empty();

	// 카테고리 배열을 기반으로 퍼즐 그룹 배열 생성
	for (const FPuzzleCategory& _puzzleCategory : _puzzleCategoryArray)
	{
		// 각 카테고리의 PuzzleInfoArray 순회
		for (const FPuzzleInfo& _puzzleInfo : _puzzleCategory.PuzzleInfoArray)
		{
			// 퍼즐 그룹 복사
			FPuzzleGroup _newPuzzleGroup = _puzzleInfo.PuzzleGroup;
			_newPuzzleGroup.IsPuzzleActivated = false;
			_newPuzzleGroup.TimerTriggerMap.Empty();

			int32 _puzzleGroupIndex = _puzzleGroupArray.Num();
			_puzzleGroupArray.Add(_newPuzzleGroup);

			// 퍼즐 그룹 포인터 가져오기
			FPuzzleGroup* _puzzleGroupPtr = &_puzzleGroupArray[_puzzleGroupIndex];

			// 퍼즐 트리거 초기화
			for (int32 j = 0; j < _puzzleGroupPtr->PuzzleTriggerArray.Num(); j++)
			{
				int32 _puzzleKey = _puzzleGroupIndex * PUZZLE_DATA_RATE + j;
				TObjectPtr<APuzzleTriggerBase> _trigger = _puzzleGroupPtr->PuzzleTriggerArray[j];
				if (_trigger == nullptr)
					continue;

				_trigger->InitializePuzzleTrigger(this, _puzzleKey);

				if (_trigger->IsTimerTrigger())
				{
					_puzzleGroupPtr->TimerTriggerMap.Add(j, false);
				}
			}

			// 퍼즐 액션 초기화
			for (TObjectPtr<APuzzleActionBase> _puzzleAction : _puzzleGroupPtr->PuzzleActionArray)
			{
				if (_puzzleAction == nullptr)
					continue;

				_puzzleAction->InitializePuzzleAction();
			}
		}
	}
}

bool ARoomController::TryGetPuzzleGroup(int32 PuzzleKey, FPuzzleGroup*& OutPuzzleGroup, int32& OutPuzzleIndex, int32& OutRawIndex)
{
	OutPuzzleIndex = PuzzleKey / PUZZLE_DATA_RATE;
	OutRawIndex = PuzzleKey % PUZZLE_DATA_RATE;
	
	if (OutPuzzleIndex < 0 || OutPuzzleIndex >= _puzzleGroupArray.Num())
	{
		UE_LOG(LogTemp, Error, TEXT("TryGetPuzzleData: Invalid puzzle index %d"), OutPuzzleIndex);
		return false;
	}

	OutPuzzleGroup = &_puzzleGroupArray[OutPuzzleIndex];
	return true;
}

#pragma region Timer trigger
//void ARoomController::SetTriggerTimer(int32 PuzzleIndex, int32 TriggerIndex)
//{
//	// 서버에서만 실행되도록 체크
//	if (!HasAuthority())
//	{
//		Server_SetTriggerTimer(PuzzleIndex, TriggerIndex);
//		return;
//	}
//	
//	if (PuzzleIndex < 0 || PuzzleIndex >= _puzzleGroupArray.Num())
//		return;
//	
//	FPuzzleGroup* _puzzleGroupPtr = &_puzzleGroupArray[PuzzleIndex];
//	if (!_puzzleGroupPtr->TimerTriggerMap.Contains(TriggerIndex))
//		return;
//	
//	// 모든 타이머 트리거가 false인지 확인
//	if (CheckAllTimerTriggerValue(_puzzleGroupPtr, false))
//	{
//		_puzzleGroupPtr->TimerTriggerMap[TriggerIndex] = true;
//	}
//	
//	// 타이머 설정
//	GetWorld()->GetTimerManager().SetTimer(
//		_puzzleGroupPtr->TriggerTimerHandle, 
//		FTimerDelegate::CreateUObject(this, &ARoomController::OnPuzzleTriggerTimeOut, _puzzleGroupPtr), 
//		_puzzleGroupPtr->TimerLimit, 
//		false
//	);
//}

//void ARoomController::Server_SetTriggerTimer_Implementation(int32 PuzzleIndex, int32 TriggerIndex)
//{
//	SetTriggerTimer(PuzzleIndex, TriggerIndex);
//}
#pragma endregion Timer trigger

//void ARoomController::OnActionDeactivated(int32 PuzzleIndex)
//{
//	FPuzzleGroup* _puzzleData = nullptr;
//	int32 _puzzleIndex = -1;
//	int32 _rawIndex = -1;
//	if (!TryGetPuzzleData(PuzzleIndex, _puzzleData, _puzzleIndex, _rawIndex))
//		return;
//
//	bool _isAllActionDeactivated = true;
//	for (TObjectPtr<APuzzleActionBase> _puzzleAction : _puzzleData->PuzzleActionArray)
//	{
//		if (_puzzleAction == nullptr)
//			continue;
//
//		if (_puzzleAction->IsActivate())
//		{
//			_isAllActionDeactivated = false;
//			break;
//		}
//	}
//
//	if (_isAllActionDeactivated)
//	{
//		for (TObjectPtr<APuzzleTriggerBase> _puzzleTrigger : _puzzleData->PuzzleTriggerArray)
//		{
//			if (_puzzleTrigger == nullptr)
//				continue;
//
//			_puzzleTrigger->ChangeTriggerVisibility(true);
//		}
//	}
//}
//
//bool ARoomController::CheckAllTimerTriggerValue(FPuzzleGroup* PuzzleDataPtr, bool hopeResult)
//{
//	if (PuzzleDataPtr->TimerTriggerMap.Num() <= 0)
//		return false;
//
//	for (auto& _pair : PuzzleDataPtr->TimerTriggerMap)
//	{
//		if (_pair.Value != hopeResult)
//			return false;
//	}
//
//	return true;
//}
//

//
//void ARoomController::Server_ClearTriggerTimer_Implementation(int32 PuzzleIndex)
//{
//	ClearTriggerTimer(PuzzleIndex);
//}
//
//void ARoomController::ClearTriggerTimer(int32 PuzzleIndex)
//{
//	// 서버에서만 실행되도록 체크
//	if (!HasAuthority())
//	{
//		Server_ClearTriggerTimer(PuzzleIndex);
//		return;
//	}
//	
//	if (PuzzleIndex < 0 || PuzzleIndex >= _puzzleDataArray.Num())
//		return;
//	
//	FPuzzleGroup* _puzzleDataPtr = &_puzzleDataArray[PuzzleIndex];
//	ClearTriggerTimerHandle(_puzzleDataPtr);
//}
//
//void ARoomController::OnPuzzleTriggerTimeOut(FPuzzleGroup* PuzzleDataPtr)
//{
//	// 서버에서만 실행되는지 확인
//	if (!HasAuthority())
//		return;
//		
//	// 타이머 핸들 초기화
//	ClearTriggerTimerHandle(PuzzleDataPtr);
//	
//	// 모든 타이머 트리거 리셋
//	for (auto& _pair : PuzzleDataPtr->TimerTriggerMap)
//	{
//		// 트리거가 유효한지 확인
//		if (PuzzleDataPtr->PuzzleTriggerArray.IsValidIndex(_pair.Key) && 
//			PuzzleDataPtr->PuzzleTriggerArray[_pair.Key] != nullptr)
//		{
//			// ResetTrigger 호출 (내부적으로 Multicast_ResetTrigger를 호출)
//			PuzzleDataPtr->PuzzleTriggerArray[_pair.Key]->ResetTrigger();
//		}
//	}
//}
//
//void ARoomController::ClearTriggerTimerHandle(FPuzzleGroup* PuzzleDataPtr)
//{
//	GetWorld()->GetTimerManager().ClearTimer(PuzzleDataPtr->TriggerTimerHandle);
//	// 모든 타이머 트리거 초기화
//	for (auto& _pair : PuzzleDataPtr->TimerTriggerMap)
//	{
//		_pair.Value = false;
//	}
//}

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