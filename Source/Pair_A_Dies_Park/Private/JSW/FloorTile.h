// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FloorTile.generated.h"

UCLASS()
class AFloorTile : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFloorTile();

protected:
	// 시각 메시 컴포넌트.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Visual")
	TObjectPtr<UStaticMeshComponent> MeshComp;

public:	
	// 보스 구역인지 설정.
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Floor Settings")
	bool bIsBossPlatform = false;

};
