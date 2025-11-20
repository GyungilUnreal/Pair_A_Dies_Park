// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "TimerManager.h"
#include "AIC_Boss.generated.h"

struct FBossBlackboardKeys
{
	// C++에서 관리하기 위한 블랙보드 키 이름.
	static const FName TargetPlayer1Key;
	static const FName TargetPlayer2Key;
	static const FName FocusTargetKey;
	static const FName CanSeeTargetKey;
	static const FName IsInAttackRangeKey;
};

UCLASS()
class AAIC_Boss : public AAIController
{
	GENERATED_BODY()
	
public:
	AAIC_Boss();

protected:
	virtual void OnPossess(APawn* InPawn) override;

private:
	FTimerHandle FindPlayersTimerHandle;

	void FindPlayerAndRunBT();

	// 행동트리 에셋
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;
	// 블랙보드 에셋
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	TObjectPtr<UBlackboardData> BlackboardAsset;
};
