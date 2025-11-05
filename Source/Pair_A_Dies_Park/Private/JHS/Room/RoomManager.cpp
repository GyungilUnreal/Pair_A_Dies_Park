// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/Room/RoomManager.h"
#include "Kismet/GameplayStatics.h"
#include "JHS/GameControll/MyGameInstance.h"
#include "JHS/Room/RoomDataTable.h"

// Sets default values for this component's properties
URoomManager::URoomManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
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
	// 테스트 룸 1개
	if (_isDebugRoom)
	{
		TArray<int32> _debugRoomArray;
		_debugRoomArray.Add(0);
		return _debugRoomArray;
	}

	// 모든 방 타입 배열 생성
	TArray<int32> _originRoomTypeArray;
	for (int32 i = 0; i < static_cast<int32>(E_ROOM_TYPE::SIZE); i++)
	{
		_originRoomTypeArray.Add(i);
	}

	// 랜덤 방 선택
	TArray<int32> _resultRoomIndexArray;
	int32 _originRoomNum = _originRoomTypeArray.Num();
	int32 _roomCount = _maxRoomCount <= _originRoomNum ? _maxRoomCount : _originRoomNum;
	for (int32 i = 0; i < _roomCount; i++)
	{
		// 남은 방 중에서 랜덤 선택
		if (_originRoomTypeArray.Num() <= 0)
		{
			break;
		}

		int32 _randomIndex = FMath::RandRange(0, _originRoomTypeArray.Num() - 1);
		int32 _randomRoomIndex = _originRoomTypeArray[_randomIndex];

		// 결과 배열에 추가하고 원본 배열에서 제거
		_resultRoomIndexArray.Add(_randomRoomIndex);
		_originRoomTypeArray.RemoveAt(_randomIndex);
	}

	return _resultRoomIndexArray;
}

void URoomManager::OnCompletedRoom(int32 CompletedRommSequence)
{
	TObjectPtr<UMyGameInstance> _gameInstance = Cast<UMyGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (!_gameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("GameInstance is nullptr"));
		return;
	}

	_gameInstance->ChangeRoomSequence(CompletedRommSequence);
}

void URoomManager::LoadLevel(FRoomData RoomData)
{
	FName RoomName = RoomData.RoomTitle;
	UE_LOG(LogTemp, Warning, TEXT("Loading room: %s"), *RoomName.ToString());
	UGameplayStatics::OpenLevel(GetWorld(), RoomName);
}