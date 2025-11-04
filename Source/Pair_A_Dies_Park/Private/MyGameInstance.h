// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "MyGameInstance.generated.h"

struct FRoomData;

UCLASS()
class UMyGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	UMyGameInstance();

private:
	UPROPERTY()
	TArray<FRoomData> _roomDataArray;

	UPROPERTY()
	TArray<int32> _roomSequenceArray;

	UPROPERTY()
	int32 _currentRoomSequence = -1;

public:
	const TArray<FRoomData>& GetRoomDataArray() const { return _roomDataArray; }

	void RegistRoomData(const TArray<FRoomData>& RoomDataArray);

	const TArray<int32>& GetRoomSequence() const { return _roomSequenceArray; }

	void RegistRoomSequence(const TArray<int32>& RoomSequenceArray);

	int32 GetCurrentRoomSequence() const { return _currentRoomSequence; }

	void OnStartRoom(int32 CurrentRoomSequence) { _currentRoomSequence = CurrentRoomSequence; }

	FRoomData GetNextRoomData(int32 CompletedRoomSequence);
};
