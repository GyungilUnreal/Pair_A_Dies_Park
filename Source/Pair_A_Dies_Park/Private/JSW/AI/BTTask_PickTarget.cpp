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

	AAIController* AIC = OwnerComp.GetAIOwner();
	APawn* BossPawn = AIC ? AIC->GetPawn() : nullptr;
	if (!BossPawn) return EBTNodeResult::Failed;

	AActor* CurrentTarget = Cast<AActor>(BlackboardComp->GetValueAsObject(FBossBlackboardKeys::FocusTargetKey));
	AActor* Player1 = Cast<AActor>(BlackboardComp->GetValueAsObject(FBossBlackboardKeys::TargetPlayer1Key));
	AActor* Player2 = Cast<AActor>(BlackboardComp->GetValueAsObject(FBossBlackboardKeys::TargetPlayer2Key));

	TArray<AActor*> Candidates;
	if (Player1) Candidates.Add(Player1);
	if (Player2) Candidates.Add(Player2);

	if (Candidates.Num() == 0) return EBTNodeResult::Failed;

	AActor* NewTarget = nullptr;

	// 현재 타겟이 없거나 죽었으면 -> 가장 가까운 놈 찾기
	if (!CurrentTarget)
	{
		float MinDistSq = FLT_MAX;
		for (AActor* Candidate : Candidates)
		{
			float DistSq = FVector::DistSquared(BossPawn->GetActorLocation(), Candidate->GetActorLocation());
			if (DistSq < MinDistSq)
			{
				MinDistSq = DistSq;
				NewTarget = Candidate;
			}
		}
	}
	// 현재 타겟이 잘 살아있다면 -> 다른 플레이어로 어그로
	else
	{
		if (Candidates.Num() > 1)
		{
			if (CurrentTarget == Player1) NewTarget = Player2;
			else NewTarget = Player1;
		}
		else
		{
			NewTarget = CurrentTarget;
		}
	}

	if (NewTarget)
	{
		BlackboardComp->SetValueAsObject(FBossBlackboardKeys::FocusTargetKey, NewTarget);
		return EBTNodeResult::Succeeded;
	}

	return EBTNodeResult::Failed;
}