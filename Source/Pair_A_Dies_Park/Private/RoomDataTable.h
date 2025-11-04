// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "RoomDataTable.generated.h"

USTRUCT(BlueprintType)
struct FRoomData : public FTableRowBase
{
	GENERATED_BODY()

public:
	FRoomData() : Index(-1), RoomTitle(""), RoomDescription("") {}

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RoomData")
	int32 Index;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RoomData")
	FName RoomTitle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RoomData")
	FString RoomDescription;
};

UCLASS()
class ARoomDataTable : public AActor
{
	GENERATED_BODY()
};