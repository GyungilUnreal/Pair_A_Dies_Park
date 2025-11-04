// Fill out your copyright notice in the Description page of Project Settings.


#include "RoomManager.h"
#include "MyGameInstance.h"
#include "RoomController.h"
#include "RoomDataTable.h"
#include "Engine/AssetManager.h"
#include "UObject/ConstructorHelpers.h"
#include "Kismet/GameplayStatics.h"

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

void URoomManager::InitializeRoomManager()
{
	// 게임 인스턴스 가져오기
	_gameInstance = Cast<UMyGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	if (!_gameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to get game instance"));
		return;
	}

	// 방 데이터가 없으면 로드
	if (_gameInstance->GetRoomDataArray().Num() == 0)
	{
		TArray<FRoomData> RoomDataArray;
		if (!LoadRoomDataTable(RoomDataArray))
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load Room Data Table"));
			return;
		}

		_gameInstance->RegistRoomData(RoomDataArray);
	}

	// 방 순서가 없으면 생성
	if (_gameInstance->GetRoomSequence().Num() == 0)
	{
		_gameInstance->RegistRoomSequence(CreateRandomRoom());
	}
}


// Called every frame
void URoomManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


bool URoomManager::LoadRoomDataTable(TArray<FRoomData>& OutRoomDataArray)
{
	// 결과 배열 초기화
	OutRoomDataArray.Empty();

	// 데이터 테이블 로드
	FSoftObjectPath DataTablePath(_roomDataTablePath);
	TObjectPtr<UDataTable> RoomDataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *_roomDataTablePath));
	
	// 로드 실패 처리
	if (!RoomDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load Room Data Table from path: %s"), *_roomDataTablePath);
		return false;
	}

	// 데이터 테이블에서 모든 행 가져오기
	TArray<FName> RowNames = RoomDataTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		FRoomData* RoomData = RoomDataTable->FindRow<FRoomData>(RowName, TEXT(""));
		if (RoomData)
		{
			OutRoomDataArray.Add(*RoomData);
		}
	}

	return OutRoomDataArray.Num() > 0;
}

TArray<int32> URoomManager::CreateRandomRoom()
{
	// 모든 방 타입 배열 생성
	TArray<int32> OriginRoomTypeArray;
	for (int32 i = 0; i < static_cast<int32>(E_ROOM_TYPE::SIZE); i++)
	{
		OriginRoomTypeArray.Add(i);
	}

	// 결과 배열 초기화
	TArray<int32> ResultRoomIndexArray;
	
	// 랜덤 방 선택
	for (int32 i = 0; i < _maxRoomCount; i++)
	{
		// 남은 방 중에서 랜덤 선택
		if (OriginRoomTypeArray.Num() <= 0)
		{
			break;
		}
		
		int32 RandomIndex = FMath::RandRange(0, OriginRoomTypeArray.Num() - 1);
		int32 RandomRoomIndex = OriginRoomTypeArray[RandomIndex];
		
		// 결과 배열에 추가하고 원본 배열에서 제거
		ResultRoomIndexArray.Add(RandomRoomIndex);
		OriginRoomTypeArray.RemoveAt(RandomIndex);
	}

	return ResultRoomIndexArray;
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