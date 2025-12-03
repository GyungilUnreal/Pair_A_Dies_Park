// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JHS/Puzzle/Trigger/PuzzleTriggerBase.h"
#include "ActionTrigger.generated.h"

/**
 * 
 */
UCLASS()
class AActionTrigger : public APuzzleTriggerBase
{
	GENERATED_BODY()
	
protected:
	virtual void TriggerEnterEffect() override;

	virtual void TriggerExitEffect() override;

public:
	void ActionTriggerEnter();

	void ActionTriggerExit();
};
