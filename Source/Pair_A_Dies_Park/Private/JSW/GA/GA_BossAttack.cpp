// Fill out your copyright notice in the Description page of Project Settings.


#include "JSW/GA/GA_BossAttack.h"
#include "Engine/DecalActor.h"

void UGA_BossAttack::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if (SpawnedDecal)
	{
		SpawnedDecal->Destroy();
	}
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
