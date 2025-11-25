// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/GameControll/RoomSubsystem.h"
#include "JHS/GameControll/MyGameInstance.h"
#include "JHS/GameControll/GameControlFunctionLibrary.h"
#include "Net/UnrealNetwork.h"

void URoomSubsystem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// Register properties for replication
	DOREPLIFETIME(URoomSubsystem, _isDebugRoom);
	DOREPLIFETIME(URoomSubsystem, _debugRoomType);
	DOREPLIFETIME(URoomSubsystem, _maxRoomCount);
}

void URoomSubsystem::Server_CreateRoomSequence_Implementation()
{
	CreateRoomSequence();
}

TArray<int32> URoomSubsystem::CreateRoomSequence()
{
	TArray<int32> _originRoomTypeArray;

	// Create a single test room if in debug mode
	if (_isDebugRoom)
	{
		_originRoomTypeArray.Add((int32)_debugRoomType);
		return _originRoomTypeArray;
	}

	// Create array of all room types
	for (int32 i = (int32)E_ROOM_TYPE::Tutorial + 1; i < (int32)E_ROOM_TYPE::SIZE; i++)
	{
		_originRoomTypeArray.Add(i);
	}

	// Randomly select rooms
	TArray<int32> _resultRoomIndexArray;
	int32 _originRoomNum = _originRoomTypeArray.Num();
	int32 _roomCount = _maxRoomCount <= _originRoomNum ? _maxRoomCount : _originRoomNum;
	for (int32 i = 0; i < _roomCount; i++)
	{
		// Select randomly from remaining rooms
		if (_originRoomTypeArray.Num() <= 0)
			break;

		int32 _randomIndex = FMath::RandRange(0, _originRoomTypeArray.Num() - 1);
		int32 _randomRoomIndex = _originRoomTypeArray[_randomIndex];

		// Add to result array and remove from original array
		_resultRoomIndexArray.Add(_randomRoomIndex);
		_originRoomTypeArray.RemoveAt(_randomIndex);
	}

	return _resultRoomIndexArray;
}

void URoomSubsystem::Server_OnCompletedRoom_Implementation(int32 CompletedRoomIndex)
{
	OnCompletedRoom(CompletedRoomIndex);
}

void URoomSubsystem::OnCompletedRoom(int32 CompletedRoomIndex)
{
	UE_LOG(LogTemp, Warning, TEXT("Room Complete"));

	// 서버 권한 체크 - 월드에서 직접 확인
	UWorld* _world = GetWorld();
	if (!_world)
	{
		UE_LOG(LogTemp, Error, TEXT("World is nullptr in OnCompletedRoom"));
		return;
	}
	
	// 언리얼 C++ 문법으로 서버 체크 수정
	if (_world->GetNetMode() == NM_Client) // 클라이언트인 경우
	{
		Server_OnCompletedRoom(CompletedRoomIndex);
		return;
	}

	TObjectPtr<UMyGameInstance> _gameInstance = nullptr;
	if (!UGameControlFunctionLibrary::TryGetGameInstance(_gameInstance))
		return;

	_gameInstance->ChangeRoom(CompletedRoomIndex);
}

void URoomSubsystem::Server_LoadLevel_Implementation(FRoomData RoomData)
{
	LoadLevel(RoomData);
}

void URoomSubsystem::LoadLevel(FRoomData RoomData)
{
	// 서버 권한 체크 - 월드에서 직접 확인
	UWorld* _world = GetWorld();
	if (!_world)
	{
		UE_LOG(LogTemp, Error, TEXT("World is nullptr in LoadLevel"));
		return;
	}
	
	// 서버 체크
	if (_world->GetNetMode() == NM_Client) // 클라이언트인 경우
	{
		Server_LoadLevel(RoomData);
		return;
	}

	FName _roomName = RoomData.RoomTitle;
	UE_LOG(LogTemp, Warning, TEXT("Loading room: %s"), *_roomName.ToString());
	_world->ServerTravel(*_roomName.ToString());
}