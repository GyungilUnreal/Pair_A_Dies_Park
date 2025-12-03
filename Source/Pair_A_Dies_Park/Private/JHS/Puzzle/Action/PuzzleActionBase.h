// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PuzzleActionBase.generated.h"

UCLASS()
class APuzzleActionBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APuzzleActionBase();

protected:
	// 액션 활성화 상태 (복제됨)
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_IsActivate)
	bool _isActivate = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Puzzle|Debug")
	bool _isDebug = false;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Puzzle|Debug")
	int32 _actionIndex = -1;

public:
	bool IsActivate() { return _isActivate; }

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	// 상속 구현
	virtual void OnActivatePuzzleAction() { }

	virtual void OnDeactivatePuzzleAction() { }
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// 복제 상태가 변경되었을 때 호출되는 함수
	UFUNCTION()
	void OnRep_IsActivate();
	
	// 네트워크 복제 설정
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	void InitializePuzzleAction();

	void ActivatePuzzleAction();

	void DeactivatePuzzleAction();
};
