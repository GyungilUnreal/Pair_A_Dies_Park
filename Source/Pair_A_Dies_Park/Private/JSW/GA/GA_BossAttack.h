// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/DecalActor.h"
#include "GA_BossAttack.generated.h"

class UAnimMontage;

UCLASS()
class UGA_BossAttack : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	// 어빌리티 종료함수
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:

	// 공격 애니메이션 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack Base")
	UAnimMontage* AttackMontage;

	// 데칼 블루프린트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Attack Base")
	TSubclassOf<ADecalActor> ReplicatedDecalBP;

	// 중복 피격방지용 공격에 맞은 액터 저장용.
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> HitActors;

	// 스폰된 데칼 액터. (삭제용)
	UPROPERTY()
	ADecalActor* SpawnedDecal;

};
