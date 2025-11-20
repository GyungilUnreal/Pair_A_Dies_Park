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

		FindPlayerAndRunBT();
	}
}

void AAIC_Boss::FindPlayerAndRunBT()
{
	// 2명의 플레이어를 찾아 블랙보드 키에 저장.
	TArray<AActor*> FoundPlayerControllers;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerController::StaticClass(), FoundPlayerControllers);

	TArray<AActor*> FoundPlayerPawns;

	for (AActor* PC_Actor : FoundPlayerControllers)
	{
		APlayerController* PC = Cast<APlayerController>(PC_Actor);
		if (PC)
		{
			APawn* PlayerPawn = PC->GetPawn();
			if (PlayerPawn && Cast<ASplit_Character>(PlayerPawn))
			{
				FoundPlayerPawns.Add(PlayerPawn);
			}
		}
	}

	// 플레이어를 못찾으면
	if (FoundPlayerPawns.Num() == 0)
	{
		GetWorld()->GetTimerManager().SetTimer(
			FindPlayersTimerHandle,
			this,
			&AAIC_Boss::FindPlayerAndRunBT,
			1.0f,
			false
		);
	}
	// 플레이어를 찾으면
	else
	{
		// 타이머 중지
		GetWorld()->GetTimerManager().ClearTimer(FindPlayersTimerHandle);
		// 블랙보드 값 세팅
		UBlackboardComponent* BlackboardComp = GetBlackboardComponent();

		if (FoundPlayerPawns.Num() > 0)
		{
			BlackboardComp->SetValueAsObject(FBossBlackboardKeys::TargetPlayer1Key, FoundPlayerPawns[0]);
			BlackboardComp->SetValueAsObject(FBossBlackboardKeys::FocusTargetKey, FoundPlayerPawns[0]);
		}

		if (FoundPlayerPawns.Num() > 1)
		{
			BlackboardComp->SetValueAsObject(FBossBlackboardKeys::TargetPlayer2Key, FoundPlayerPawns[1]);
		}
	}
	// 행동트리 실행.
	RunBehaviorTree(BehaviorTreeAsset);
}
