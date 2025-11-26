// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RoomSubsystem.generated.h"

UENUM(BlueprintType)
enum class E_ROOM_TYPE : uint8
{
	Boss = 0 UMETA(DisplayName = "Boss"),
	Tutorial UMETA(DisplayName = "Tutorial"),
	Subway UMETA(DisplayName = "Subway"),
	Spaceship UMETA(DisplayName = "Spaceship"),
	Dungeon UMETA(DisplayName = "Dungeon"),

	SIZE UMETA(DisplayName = "SIZE")
};

UCLASS(BlueprintType, Blueprintable)
class URoomSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "RoomSubsystem|Debug")
	bool _isDebugRoom = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "RoomSubsystem|Debug")
	E_ROOM_TYPE _debugRoomType = E_ROOM_TYPE::Subway;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "RoomSubsystem|Room")
	int32 _maxRoomCount = 3;

public:
	bool GetIsDebug() { return _isDebugRoom; }

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(Server, Reliable)
	void Server_CreateRoomSequence();

	TArray<int32> CreateRoomSequence();

	UFUNCTION(Server, Reliable)
	void Server_OnCompletedRoom(int32 CompletedRoomIndex);

	void OnCompletedRoom(int32 CompletedRoomIndex);

	UFUNCTION(Server, Reliable)
	void Server_LoadLevel(FRoomData RoomData);

	void LoadLevel(struct FRoomData RoomData);
};
