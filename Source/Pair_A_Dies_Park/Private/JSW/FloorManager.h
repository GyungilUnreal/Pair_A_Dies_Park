// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelSequenceActor.h"
#include "FloorManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTeamLifeChanged, int32, NewLife);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnFallCountChanged, int32, NewCount);

USTRUCT(BlueprintType)
struct FTileData
{
	GENERATED_BODY()

	UPROPERTY()
	uint8 HP = 2;

	UPROPERTY()
	TObjectPtr<AActor> VisualActor = nullptr;
};

UCLASS()
class AFloorManager : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AFloorManager();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Grid")
	float Floor1_Height = 43111.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Grid")
	float Floor2_Height = 49550.0f;

	UFUNCTION(BlueprintCallable, Category = "Boss")
	void ActivateClearItem();

protected:
	// 9X9의 그리드 상태를 저장하는 1차원 배열 (true=파괴됨, false=존재)
	UPROPERTY(ReplicatedUsing = OnRep_GridData)
	TArray<FTileData> GridData;

	// 설정값
	const int32 GridWidth = 9;
	const int32 TilesPerLayer = 81; // 9*9

	UPROPERTY()
	TArray<FVector> InitialSpawnLocations;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Boss")
	TObjectPtr<AActor> ClearRewardItem;

	// 타일 사이즈
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor Grid")
	float TileSize = 1320.0f;

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Cinematic")
	TObjectPtr<ALevelSequenceActor> IntroSequenceActor;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayLevelIntro();

	UPROPERTY(ReplicatedUsing = OnRep_TeamLife)
	int32 TeamLife = 5;

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ActivateClearVisuals();

	UFUNCTION()
	void OnRep_GridData();

	UFUNCTION()
	void OnRep_TeamLife();

	// 데이터 상태(HP)를 보고 액터를 숨기거나 머티리얼을 바꿈
	void UpdateVisualsFromState();

public:
	// 월드 좌표를 그리드 인덱스로 변환 (성공 시 true 반환)
	bool WorldToGridIndex(FVector WorldPos, int32& OutLayer, FIntPoint& OutCoord);

	// 해당 좌표의 타일이 밟을 수 있는 상태인지 확인 (HP > 0)
	bool IsTileWalkable(int32 Layer, FIntPoint Coord);

	// 특정 타일의 월드 위치 반환 (큐브 스폰, 텔레포트용)
	FVector GetTileWorldLocation(int32 Layer, FIntPoint Coord);

	// 데미지 적용 (서버 전용)
	UFUNCTION(BlueprintCallable, Category = "Floor Manager")
	void Server_DamageTile(int32 Layer, FIntPoint Coord, int32 DamageAmount);

	// 타일 복구 (서버 전용)
	UFUNCTION(BlueprintCallable, Category = "Floor Manager")
	void Server_RestoreRandomTiles(int32 Layer, int32 Count);
	
	// 리스폰 할 타일 랜덤으로 찾기.
	UFUNCTION(BlueprintCallable, Category = "Floor Manager")
	FVector GetRandomSafeFloorLocation();

	// 팀 목숨 관리 함수
	UFUNCTION(BlueprintCallable, Category = "Floor Manager")
	void ModifyTeamLife(int32 Amount);

	// 점프맵용 낙하 증가 함수
	UFUNCTION(BlueprintCallable, Category = "Floor Manager")
	void IncrementFallCount();

	// 점프맵 용 낙하 카운트.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Stats")
	int32 FallCount = 0;

	// UI 바인딩용 함수.
	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnTeamLifeChanged OnTeamLifeChanged;

	UPROPERTY(BlueprintAssignable, Category = "UI")
	FOnFallCountChanged OnFallCountChanged;
};