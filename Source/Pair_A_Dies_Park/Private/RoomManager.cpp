// Fill out your copyright notice in the Description page of Project Settings.


#include "RoomManager.h"
#include "Kismet/GameplayStatics.h"
#include "MyGameInstance.h"
#include "RoomDataTable.h"

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

	InitializeRoomManager();
}

// Called every frame
void URoomManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void URoomManager::InitializeRoomManager()
{
	// 게임 인스턴스 가져오기
	_gameInstance = Cast<UMyGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (!_gameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get game instance"));
		return;
	}
}

TArray<int32> URoomManager::CreateRandomRoom()
{
	// 모든 방 타입 배열 생성
	TArray<int32> _originRoomTypeArray;
	for (int32 i = 0; i < static_cast<int32>(E_ROOM_TYPE::SIZE); i++)
	{
		_originRoomTypeArray.Add(i);
	}

	// 결과 배열 초기화
	TArray<int32> _resultRoomIndexArray;

	// 랜덤 방 선택
	for (int32 i = 0; i < _maxRoomCount; i++)
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
	UE_LOG(LogTemp, Warning, TEXT("Completed Romm Sequence : %d"), CompletedRommSequence);
	if (CompletedRommSequence + 1 >= _maxRoomCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("Completed All"));
		return;
	}

	LoadLevel(CompletedRommSequence + 1);
}

void URoomManager::LoadLevel(int32 NextRoomSequence)
{
	// 게임 인스턴스가 없으면 로드 실패
	if (!_gameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("Game instance is null"));
		return;
	}
	
	_gameInstance->OnStartRoom(NextRoomSequence);
	const FRoomData _nextRoomData = _gameInstance->GetNextRoomData(NextRoomSequence);

	// 레벨 로드
	FName RoomName = _nextRoomData.RoomTitle;
	UE_LOG(LogTemp, Warning, TEXT("Loading room: %s"), *RoomName.ToString());
	UGameplayStatics::OpenLevel(GetWorld(), RoomName);
}