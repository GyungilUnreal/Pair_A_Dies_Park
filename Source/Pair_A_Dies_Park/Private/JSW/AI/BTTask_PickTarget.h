// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_PickTarget.generated.h"


UCLASS()
class UBTTask_PickTarget : public UBTTaskNode
{
	GENERATED_BODY()
	
public:
	UBTTask_PickTarget();

	// BT에서 이 태스크가 실행 될 때 호출.
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

};
