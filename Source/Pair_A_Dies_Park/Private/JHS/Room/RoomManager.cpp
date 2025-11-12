// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/Room/RoomManager.h"
#include "JHS/GameControll/GameControlFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "JHS/GameControll/MyGameInstance.h"
#include "JHS/Room/RoomDataTable.h"
#include "Net/UnrealNetwork.h"

// Sets default values for this component's properties
URoomManager::URoomManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
	
	// Enable network replication
	SetIsReplicatedByDefault(true);
}


// Called when the game starts
void URoomManager::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void URoomManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void URoomManager::InitializeRoomManager()
{
}

TArray<int32> URoomManager::CreateRandomRoom()
{
	// Create a single test room if in debug mode
	if (_isDebugRoom)
	{
		TArray<int32> _debugRoomArray;
		_debugRoomArray.Add((int32)_debugRoomType);
		return _debugRoomArray;
	}

	// Create array of all room types
	TArray<int32> _originRoomTypeArray;
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

void URoomManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// Register properties for replication
	DOREPLIFETIME(URoomManager, _isDebugRoom);
	DOREPLIFETIME(URoomManager, _debugRoomType);
	DOREPLIFETIME(URoomManager, _maxRoomCount);
}

void URoomManager::Server_CreateRandomRoom_Implementation()
{
	CreateRandomRoom();
}

void URoomManager::Server_OnCompletedRoom_Implementation(int32 CompletedRoomIndex)
{
	OnCompletedRoom(CompletedRoomIndex);
}

void URoomManager::Server_LoadLevel_Implementation(FRoomData RoomData)
{
	LoadLevel(RoomData);
}

void URoomManager::OnCompletedRoom(int32 CompletedRoomIndex)
{
	// Check if running on server
	if (!GetOwner()->HasAuthority())
	{
		Server_OnCompletedRoom(CompletedRoomIndex);
		return;
	}

	TObjectPtr<UMyGameInstance> _gameInstance = nullptr;
	if (!UGameControlFunctionLibrary::TryGetGameInstance(_gameInstance))
		return;

	_gameInstance->ChangeRoom(CompletedRoomIndex);
}

void URoomManager::LoadLevel(FRoomData RoomData)
{
	// Check if running on server
	if (!GetOwner()->HasAuthority())
	{
		Server_LoadLevel(RoomData);
		return;
	}

	FName _roomName = RoomData.RoomTitle;
	UE_LOG(LogTemp, Warning, TEXT("Loading room: %s"), *_roomName.ToString());
	GetWorld()->ServerTravel(*_roomName.ToString());
}