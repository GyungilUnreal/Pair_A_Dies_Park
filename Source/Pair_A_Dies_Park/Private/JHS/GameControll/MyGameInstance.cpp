// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/GameControll/MyGameInstance.h"
#include "JHS/Room/RoomDataTable.h"
#include "Kismet/GameplayStatics.h"
#include "JHS/GameControll/MyGameMode.h"
#include "JHS/Room/RoomManager.h"

UMyGameInstance::UMyGameInstance()
{
	
}

void UMyGameInstance::Init()
{
	if (!LoadRoomDataTable(_roomDataArray))
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load Room Data Table"));
		return;
	}
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

void UMyGameInstance::RegistRoomSequence(const TArray<int32>& RoomSequenceArray)
{
	_roomSequenceArray = RoomSequenceArray;
	
	// 로그 출력
	for (int32 i = 0; i < _roomSequenceArray.Num(); i++)
	{
		UE_LOG(LogTemp, Warning, TEXT("Room Index: %d"), _roomSequenceArray[i]);
	}
}

void UMyGameInstance::ChangeRoomSequence(int32 CompletedRoomSequence)
{
	TObjectPtr<AMyGameMode> _gameMode = Cast<AMyGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (!_gameMode)
	{
		UE_LOG(LogTemp, Error, TEXT("GameMode is nullptr"));
		return;
	}

	// 튜토리얼 레벨 시작
	if (CompletedRoomSequence <= -1)
	{
		_currentRoomSequence = -1;
	}
	// 일반 레벨 시작
	else if (CompletedRoomSequence == 0)
	{
		_currentRoomSequence = 0;
	}
	else
	{
		if (CompletedRoomSequence != _currentRoomSequence)
		{
			UE_LOG(LogTemp, Error, TEXT("Completed sequence [%d] is not current sequence"), CompletedRoomSequence);
			return;
		}

		_currentRoomSequence++;
		if (_currentRoomSequence == _roomSequenceArray.Num())
		{
			UE_LOG(LogTemp, Error, TEXT("Completed all sequence"));

			_gameMode->OnGameEnd();
		}
	}

	UE_LOG(LogTemp, Error, TEXT("Next sequence is [%d]"), _currentRoomSequence);

	TObjectPtr<URoomManager> _roomManager = _gameMode->GetRoomManager();
	if (_roomManager)
	{
		UE_LOG(LogTemp, Error, TEXT("RoomManager is nullptr"));
		return;
	}

	// 다음 시퀀스 룸 시작
	int32 _nextRoomIndex = _roomSequenceArray[_currentRoomSequence];
	FRoomData _nextRoomData = _roomDataArray[_nextRoomIndex];
	_roomManager->LoadLevel(_nextRoomData);
}