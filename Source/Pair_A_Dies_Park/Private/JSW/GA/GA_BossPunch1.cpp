// Fill out your copyright notice in the Description page of Project Settings.

#include "JSW/GA/GA_BossPunch1.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "JSW/BossCharacter.h"
#include "JSW/AI/AIC_Boss.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"


UGA_BossPunch1::UGA_BossPunch1()
{

}

void UGA_BossPunch1::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (ReplicatedDecalBP)
	{
		ABossCharacter* OwnerCharacter = Cast<ABossCharacter>(GetAvatarActorFromActorInfo());
		if (OwnerCharacter)
		{
			FVector AttackLocation = OwnerCharacter->GetActorLocation() + OwnerCharacter->GetActorForwardVector() * PunchLength;
			FRotator DecalRotation = FRotator(-90.f, OwnerCharacter->GetActorRotation().Yaw, 0.f);

			SpawnedDecal = GetWorld()->SpawnActor<ADecalActor>(ReplicatedDecalBP, AttackLocation, DecalRotation);
		}
	}

	HitActors.Empty();

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, AttackMontage);
	if (MontageTask)
	{
		MontageTask->OnCompleted.AddDynamic(this, &UGA_BossPunch1::OnMontageCompleted);
		MontageTask->OnInterrupted.AddDynamic(this, &UGA_BossPunch1::OnMontageCompleted);
		MontageTask->ReadyForActivation();

		if (ABossCharacter* OwnerCharacter = Cast<ABossCharacter>(GetAvatarActorFromActorInfo()))
		{
			PreciousSocketLocation = OwnerCharacter->GetMesh()->GetSocketLocation(SocketName);
		}

		GetWorld()->GetTimerManager().SetTimer(
			OverlapTimerHandle,
			this,
			&UGA_BossPunch1::PerformOverlapCheck,
			0.1f,
			true
		);
	}
	else
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}

}

void UGA_BossPunch1::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	GetWorld()->GetTimerManager().ClearTimer(OverlapTimerHandle);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_BossPunch1::OnMontageCompleted()
{
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UGA_BossPunch1::PerformOverlapCheck()
{	
	if (!GetAbilitySystemComponentFromActorInfo()->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.DamageWindow.Active"))))  return;

	ABossCharacter* OwnerCharacter = Cast<ABossCharacter>(GetAvatarActorFromActorInfo());
	if (!OwnerCharacter) return;

	const FVector CurrnetSocketLocation = OwnerCharacter->GetMesh()->GetSocketLocation(SocketName);
	const FRotator SocketRotation = OwnerCharacter->GetMesh()->GetSocketRotation(SocketName);

	TArray<FHitResult> HitResults;
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(OwnerCharacter);

	UKismetSystemLibrary::BoxTraceMulti(
		GetWorld(),
		PreciousSocketLocation,
		CurrnetSocketLocation,
		HalfSize,
		SocketRotation,
		UEngineTypes::ConvertToTraceType(ECC_Pawn),
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		HitResults,
		true
	);

	for (const FHitResult& Hit : HitResults)
	{
		AActor* TargetActor = Hit.GetActor();
		if (!TargetActor || HitActors.Contains(TargetActor)) continue;

		ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor);
		if (TargetCharacter)
		{
			FVector KnockbackDiraction = TargetCharacter->GetActorLocation() - OwnerCharacter->GetActorLocation();
			KnockbackDiraction.Z = 0;
			KnockbackDiraction.Normalize();

			TargetCharacter->LaunchCharacter(
				(KnockbackDiraction * HorizontalKnockback) + FVector(0.f, 0.f, UpwardKnockback),
				true, true
			);
		}
		HitActors.Add(TargetActor);
	}
	PreciousSocketLocation = CurrnetSocketLocation;
}
