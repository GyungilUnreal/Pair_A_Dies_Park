// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "Net/UnrealNetwork.h"

#include "RoomManager.generated.h"



UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class URoomManager : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	URoomManager();



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
	
	//TArray<int32> CreateRandomRoom();

	UFUNCTION(Server, Reliable)
	void Server_OnCompletedRoom(int32 CompletedRoomIndex);
	
	void OnCompletedRoom(int32 CompletedRoomIndex);

	UFUNCTION(Server, Reliable)
	void Server_LoadLevel(FRoomData RoomData);
	
	void LoadLevel(struct FRoomData RoomData);
};
