// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"

#include "RoomManager.generated.h"

UENUM(BlueprintType)
enum class E_ROOM_TYPE : uint8
{
	Tutorial = 0 UMETA(DisplayName = "Tutorial"),
	Metro UMETA(DisplayName = "Metro"),
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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Room Manager|Room")
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
	TArray<int32> CreateRandomRoom();

	void OnCompletedRoom(int32 CompletedRommSequence);

	void LoadLevel(struct FRoomData RoomData);
};
