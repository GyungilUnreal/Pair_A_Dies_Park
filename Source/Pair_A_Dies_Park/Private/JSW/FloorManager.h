// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FloorManager.generated.h"

UCLASS()
class AFloorManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AFloorManager();

protected:
	// 9X9의 그리드 상태를 저장하는 1차원 배열 (true=파괴됨, false=존재)
	UPROPERTY(ReplicatedUsing = OnRep_GridState)
	TArray<bool> GridState;

	// 레벨에 배치된 AFloorTile 액터를 정렬, 저장하는 로컬 배열 (복제 안 함)
	UPROPERTY()
	TArray<TObjectPtr<AActor>> LocalCubes;

	// 파괴 가능 여부 마스크 (true=파괴가능, false=보스발판)
	UPROPERTY()
	TArray<bool> bIsBreakable;

	// 그리드의 가로 크기
	UPROPERTY(EditDefaultsOnly, Category = "Floor Grid")
	int32 GridWidth = 9;

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_GridState();

	// 상태를 로컬 큐브 비주얼에 반영
	void UpdateVisualsFromState();

public:
	UFUNCTION(BlueprintCallable, Category = "Floor Manager")
	void Server_BreakCubes(const TArray<FIntPoint>& CoordsToBreak);

	UFUNCTION(BlueprintCallable, Category = "Floor Manager")
	void Server_RegenerateCubes(const TArray<FIntPoint>& CoordsToRegen);
};
