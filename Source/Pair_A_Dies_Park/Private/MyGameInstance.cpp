// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameInstance.h"
#include "RoomDataTable.h"

UMyGameInstance::UMyGameInstance()
{
	
}

void UMyGameInstance::RegistRoomData(const TArray<FRoomData>& RoomDataArray)
{
    _roomDataArray = RoomDataArray;
    UE_LOG(LogTemp, Warning, TEXT("Room Data Table loaded with %d entries"), _roomDataArray.Num());
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

FRoomData UMyGameInstance::GetNextRoomData(int32 CompletedRoomSequence)
{
	int32 _nextRoomSequence = _roomSequenceArray[CompletedRoomSequence];
	return _roomDataArray[_nextRoomSequence];
}