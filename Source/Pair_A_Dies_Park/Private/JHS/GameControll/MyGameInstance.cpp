// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/GameControll/MyGameInstance.h"
#include "JHS/GameControll/GameControlFunctionLibrary.h"
#include "JHS/Room/RoomDataTable.h"
#include "Kismet/GameplayStatics.h"
#include "JHS/GameControll/RoomSubsystem.h"

UMyGameInstance::UMyGameInstance()
{
	
}

void UMyGameInstance::Init()
{
	Super::Init();

	if (!LoadRoomDataTable(_roomDataArray))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load Room Data Table"));
		return;
	}
}

void UMyGameInstance::GameStart(bool IsTutorial)
{
	_roomSequenceArray.Empty();

	// 튜토리얼
	if (IsTutorial)
	{
		_currentRoomSequence = -1;
		LoadLevel((int32)E_ROOM_TYPE::Tutorial);
		return;
	}

	TObjectPtr<URoomSubsystem> _roomSubSystem = nullptr;
	if (!UGameControlFunctionLibrary::TryGetRoomSubSystem(this, _roomSubSystem))
		return;

	_roomSequenceArray = _roomSubSystem->CreateRoomSequence();
	if (_roomSequenceArray.Num() <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Room Sequence is empty"));
		return;
	}

	for (int32 i = 0; i < _roomSequenceArray.Num(); i++)
	{
		UE_LOG(LogTemp, Warning, TEXT("Room Index : %d"), _roomSequenceArray[i]);
	}

	_currentRoomSequence = 0;
	LoadLevel(_roomSequenceArray[_currentRoomSequence]);
}

void UMyGameInstance::GameEnd()
{
	UE_LOG(LogTemp, Warning, TEXT("Game end"));
}

FRoomData UMyGameInstance::GetCurrentRoomData()
{
	if (_currentRoomSequence == -1)
		return _roomDataArray[(int32)E_ROOM_TYPE::Tutorial];

	if (_roomSequenceArray.Num() <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Room sequence is empty"));
		return FRoomData();
	}

	if (_currentRoomSequence >= _roomSequenceArray.Num())
		return _roomDataArray[(int32)E_ROOM_TYPE::Boss];

	return _roomDataArray[_roomSequenceArray[_currentRoomSequence]];
}

void UMyGameInstance::ChangeRoom(int32 CompletedRoomIndex)
{
	// 튜토리얼
	if (CompletedRoomIndex == (int32)E_ROOM_TYPE::Tutorial)
	{
		// TODO : 로비 이동
		return;
	}

	// 보스 클리어
	if (CompletedRoomIndex == (int32)E_ROOM_TYPE::Boss)
	{
		_isGameClear = true;
		GameEnd();
		return;
	}

	int _roomSequenceCount = _roomSequenceArray.Num();
	if (_roomSequenceCount <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Room sequence is empty"));
		return;
	}

	int32 _currentRoomIndex = _roomSequenceArray[_currentRoomSequence];
	if (CompletedRoomIndex != _currentRoomIndex)
	{
		UE_LOG(LogTemp, Error, TEXT("Current room sequence : [%d]"), _currentRoomSequence);
		UE_LOG(LogTemp, Error, TEXT("[%d] is not prev room index, correct : [%d]"), CompletedRoomIndex, _currentRoomIndex);
		return;
	}

	_currentRoomSequence++;
	int32 _nextRoomIndex = -1;
	// 다음 레벨
	if (_currentRoomSequence < _roomSequenceCount)
	{
		_nextRoomIndex = _roomSequenceArray[_currentRoomSequence];
	}
	// 보스 레벨
	else
	{
		_nextRoomIndex = (int32)E_ROOM_TYPE::Boss;
	}

	LoadLevel(_nextRoomIndex);
}

void UMyGameInstance::LoadLevel(int32 RoomIndex)
{
	TObjectPtr<URoomSubsystem> _roomSubSystem = nullptr;
	if (!UGameControlFunctionLibrary::TryGetRoomSubSystem(this, _roomSubSystem))
		return;

	FRoomData _nextRoomData = _roomDataArray[RoomIndex];
	UE_LOG(LogTemp, Warning, TEXT("Next sequence : [%d], Room desc : [%s]"), _currentRoomSequence, *_nextRoomData.RoomDescription);
	_roomSubSystem->LoadLevel(_nextRoomData);
}

bool UMyGameInstance::LoadRoomDataTable(TArray<FRoomData>& OutRoomDataArray)
{
	// 결과 배열 초기화
	OutRoomDataArray.Empty();

	// 데이터 테이블 로드
	FSoftObjectPath _dataTablePath(_roomDataTablePath);
	TObjectPtr<UDataTable> _roomDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *_roomDataTablePath));

	// 로드 실패 처리
	if (!_roomDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load Room Data Table from path: %s"), *_roomDataTablePath);
		return false;
	}

	// 데이터 테이블에서 모든 행 가져오기
	TArray<FName> RowNames = _roomDataTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		FRoomData* RoomData = _roomDataTable->FindRow<FRoomData>(RowName, TEXT(""));
		if (RoomData)
		{
			OutRoomDataArray.Add(*RoomData);
		}
	}

	// 데이터 테이블이 비어있으면 실패
	if (OutRoomDataArray.Num() <= 0)
	{
		UE_LOG(LogTemp, Error, TEXT("Room Data Table is empty"));
		return false;
	}

	UE_LOG(LogTemp, Warning, TEXT("Room Data Table loaded successfully. %d rows loaded"), OutRoomDataArray.Num());
	return true;
}