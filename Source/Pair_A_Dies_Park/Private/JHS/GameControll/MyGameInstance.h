// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "JHS/Room/RoomDataTable.h"

#include "MyGameInstance.generated.h"

UCLASS()
class UMyGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	UMyGameInstance();

private:
	bool _isGameClear = false;

#pragma region Room Data Table
	const FString _roomDataTablePath = TEXT("/Game/Collaborators/JHS/Resource/Data/RoomTable.RoomTable");

	const FString _roomLevelFolderPath = TEXT("Game/Collaborators/JHS/Map/");

	UPROPERTY()
	TArray<FRoomData> _roomDataArray;
#pragma endregion Room Data Table

	UPROPERTY()
	TArray<int32> _roomSequenceArray;

	UPROPERTY()
	int32 _currentRoomSequence = -1;

public:
	virtual void Init() override;

public:
	UFUNCTION(BlueprintCallable, Category = "Game Mode|Start Game")
	void GameStart(bool IsTutorial);

	void GameEnd();

	FRoomData GetCurrentRoomData();

	void ChangeRoom(int32 CompletedRoomIndex);

private:
	void LoadLevel(int32 RoomIndex);

	bool LoadRoomDataTable(TArray<FRoomData>& OutRoomDataArray);
};
