// Fill out your copyright notice in the Description page of Project Settings.


#include "JSW/AI/AIC_Boss.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Kismet/GameplayStatics.h"
#include "Split_Character.h"
#include "GameFramework/PlayerController.h"

// 블랙보드 키 정의
const FName FBossBlackboardKeys::TargetPlayer1Key = TEXT("TargetPlayer1");
const FName FBossBlackboardKeys::TargetPlayer2Key = TEXT("TargetPlayer2");
const FName FBossBlackboardKeys::FocusTargetKey = TEXT("FocusTarget");
const FName FBossBlackboardKeys::CanSeeTargetKey = TEXT("CanSeeTarget");
const FName FBossBlackboardKeys::IsInAttackRangeKey = TEXT("IsInAttackRangeKey");

AAIC_Boss::AAIC_Boss()
{
	//SetPerceptionComponent(*CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("PerceptionComp")));
}

void AAIC_Boss::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (BehaviorTreeAsset)
	{
		// 블랙보드 초기화, 컨트롤러에 할당.
		UBlackboardComponent* BlackboardComp;
		UseBlackboard(BlackboardAsset, BlackboardComp);

		RunBehaviorTree(BehaviorTreeAsset);
	}
}

void AAIC_Boss::InitBossFight()
{
	// 플레이어 컨트롤러 찾기
	TArray<AActor*> FoundPlayerControllers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerController::StaticClass(), FoundPlayerControllers);

	TArray<AActor*> FoundPlayerPawns;
	for (AActor* PC_Actor : FoundPlayerControllers)
	{
		if (APlayerController* PC = Cast<APlayerController>(PC_Actor))
		{
			if (APawn* PlayerPawn = PC->GetPawn())
			{
				FoundPlayerPawns.Add(PlayerPawn);
			}
		}
	}

	// 블랙보드 세팅
	UBlackboardComponent* BB = GetBlackboardComponent();
	if (BB && FoundPlayerPawns.Num() > 0)
	{
		BB->SetValueAsObject(FBossBlackboardKeys::TargetPlayer1Key, FoundPlayerPawns[0]);

		BB->SetValueAsObject(FBossBlackboardKeys::FocusTargetKey, FoundPlayerPawns[0]);

		if (FoundPlayerPawns.Num() > 1)
		{
			BB->SetValueAsObject(FBossBlackboardKeys::TargetPlayer2Key, FoundPlayerPawns[1]);
		}

		BB->SetValueAsBool(TEXT("IsAwake"), true);
	}
}