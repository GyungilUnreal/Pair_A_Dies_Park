// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"

#include "RoomManager.generated.h"

UENUM(BlueprintType)
enum class E_ROOM_TYPE : uint8
{
	Boss = 0 UMETA(DisplayName = "Boss"),
	Tutorial UMETA(DisplayName = "Tutorial"),
	Subway UMETA(DisplayName = "Subway"),
	Volcanic UMETA(DisplayName = "Volcanic"),
	Dungeon UMETA(DisplayName = "Dungeon"),

	SIZE UMETA(DisplayName = "SIZE")
};

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class URoomManager : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	URoomManager();

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Room Manager|Debug")
	bool _isDebugRoom = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Room Manager|Debug")
	E_ROOM_TYPE _debugRoomType;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Room Manager|Room")
	int32 _maxRoomCount = 3;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void InitializeRoomManager();

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(Server, Reliable)
	void Server_CreateRandomRoom();
	
	TArray<int32> CreateRandomRoom();

	UFUNCTION(Server, Reliable)
	void Server_OnCompletedRoom(int32 CompletedRoomIndex);
	
	void OnCompletedRoom(int32 CompletedRoomIndex);

	UFUNCTION(Server, Reliable)
	void Server_LoadLevel(FRoomData RoomData);
	
	void LoadLevel(struct FRoomData RoomData);
};
