// Fill out your copyright notice in the Description page of Project Settings.


#include "JSW/AI/BTTask_PickTarget.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIC_Boss.h"


UBTTask_PickTarget::UBTTask_PickTarget()
{
	NodeName = "Pick Target";
}

EBTNodeResult::Type UBTTask_PickTarget::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (!BlackboardComp) return EBTNodeResult::Failed;

	// 현재 타겟과 두 플레이어 정보를 가져옴.
	UObject* CurrentTarget = BlackboardComp->GetValueAsObject(FBossBlackboardKeys::FocusTargetKey);
	UObject* Player1 = BlackboardComp->GetValueAsObject(FBossBlackboardKeys::TargetPlayer1Key);
	UObject* Player2 = BlackboardComp->GetValueAsObject(FBossBlackboardKeys::TargetPlayer2Key);

	// 플레이어가 없으면
	if (!Player1 && !Player2) return EBTNodeResult::Failed;

	// 어그로 교체 로직. 
	UObject* NewTarget = nullptr;
	if (CurrentTarget == Player1 && Player2)
	{
		NewTarget = Player2;
	}
	else
	{
		NewTarget = Player1;
	}
	// 블랙보드의 FocusTargetKey를 새 타겟으로 덮어씌움.
	BlackboardComp->SetValueAsObject(FBossBlackboardKeys::FocusTargetKey, NewTarget);

	return EBTNodeResult::Succeeded;
}
