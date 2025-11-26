// Fill out your copyright notice in the Description page of Project Settings.


#include "JSW/GA/GA_BossFloorAttack.h"
#include "JSW/FloorManager.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Kismet/GameplayStatics.h"


UGA_BossFloorAttack::UGA_BossFloorAttack()
{
	FGameplayTag AbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Boss.FloorSlam"));
	AbilityTags.AddTag(AbilityTag);
}

void UGA_BossFloorAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AttackMontage);
	if (MontageTask)
	{
		MontageTask->OnCompleted.AddDynamic(this, &UGA_BossFloorAttack::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_BossFloorAttack::OnMontageCompleted);
		MontageTask->ReadyForActivation();
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (ActorInfo && ActorInfo->IsNetAuthority())
	{
		AFloorManager* FloorManager = Cast<AFloorManager>(
			UGameplayStatics::GetActorOfClass(GetWorld(), AFloorManager::StaticClass())
		);

		if (FloorManager)
		{
			// 하나씩 꺼내서 호출합니다.
			for (const FIntPoint& Coord : CoordsToBreak_Test)
			{
				// 1층(Layer 0)의 해당 좌표 타일에 2 데미지(파괴)를 입힘
				FloorManager->Server_DamageTile(0, Coord, 2);
			}
		}

	}
}

void UGA_BossFloorAttack::OnMontageCompleted()
{
	// 몽타주가 끝나면 어빌리티를 종료.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}