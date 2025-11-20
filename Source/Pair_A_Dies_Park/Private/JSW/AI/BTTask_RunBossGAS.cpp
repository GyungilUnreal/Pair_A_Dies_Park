// Fill out your copyright notice in the Description page of Project Settings.


#include "JSW/AI/BTTask_RunBossGAS.h"
#include "AIController.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"


UBTTask_RunBossGAS::UBTTask_RunBossGAS()
{
	NodeName = "Run GAS Ability";
}

EBTNodeResult::Type UBTTask_RunBossGAS::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	// 컨트롤러 가져오기
	AAIController* AIC = OwnerComp.GetAIOwner();
	if (!AIC) return EBTNodeResult::Failed;

	// 컨트롤러가 조종하는 폰 가져오기
	APawn* AIPawn = AIC->GetPawn();
	if (!AIPawn) return EBTNodeResult::Failed;

	// 폰에서 AbilitySystemInterface를 구현했는지 확인
	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(AIPawn);
	if (!ASI) return EBTNodeResult::Failed;

	// ASI에서 ASC를 가져옴
	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return EBTNodeResult::Failed;

	// AbilityToRun 태그로 어빌리티 실행 시도
	if (ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(AbilityToRun)))
	{
		return EBTNodeResult::Succeeded;
	}
	return EBTNodeResult::Failed;
}
