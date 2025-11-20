// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "BTTask_RunBossGAS.generated.h"


UCLASS()
class UBTTask_RunBossGAS : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_RunBossGAS();

	// BT에서 이 태스크가 실행 될 때 호출.
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	// BT 에셋에서 실행할 어빌리티 태그를 지정.
	UPROPERTY(EditAnywhere, Category = "GAS")
	FGameplayTag AbilityToRun;
};
