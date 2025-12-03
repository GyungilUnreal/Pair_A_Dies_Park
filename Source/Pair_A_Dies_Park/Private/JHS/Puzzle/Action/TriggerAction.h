// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JHS/Puzzle/Action/PuzzleActionBase.h"
#include "TriggerAction.generated.h"

class AActionTrigger;

UCLASS()
class ATriggerAction : public APuzzleActionBase
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Action Trigger")
	TObjectPtr<AActionTrigger> _actionTrigger = nullptr;

protected:
	virtual void OnActivatePuzzleAction() override;

	virtual void OnDeactivatePuzzleAction() override;
};
