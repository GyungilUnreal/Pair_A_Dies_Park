// Fill out your copyright notice in the Description page of Project Settings.


#include "JSW/ANS_DamageWindow.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

void UANS_DamageWindow::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
		{
			UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
			if (ASC)
			{
				ASC->AddLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.DamageWindow.Active")));
			}
		}
	}
}

void UANS_DamageWindow::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (AActor* Owner = MeshComp->GetOwner())
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
		{
			UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
			if (ASC)
			{
				ASC->RemoveLooseGameplayTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.DamageWindow.Active")));
			}
		}
	}
}
