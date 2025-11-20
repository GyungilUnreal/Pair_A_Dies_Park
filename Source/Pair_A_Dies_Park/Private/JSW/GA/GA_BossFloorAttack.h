// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JSW/GA/GA_BossAttack.h"
#include "GA_BossFloorAttack.generated.h"


UCLASS()
class UGA_BossFloorAttack : public UGA_BossAttack
{
	GENERATED_BODY()
	
public:
	UGA_BossFloorAttack();

protected:
	// 파괴할 큐브의 2차원 좌표 목록.
	UPROPERTY(EditDefaultsOnly, Category = "Floor Attack")
	TArray<FIntPoint> CoordsToBreak_Test;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION()
	void OnMontageCompleted();

};
