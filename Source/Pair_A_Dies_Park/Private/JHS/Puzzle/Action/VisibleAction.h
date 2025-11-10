// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JHS/Puzzle/Action/PuzzleActionBase.h"
#include "VisibleAction.generated.h"

/**
 * 
 */
UCLASS()
class AVisibleAction : public APuzzleActionBase
{
	GENERATED_BODY()
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Puzzle|Visible Action")
	TArray<TObjectPtr<AActor>> _visibleObjectArray = TArray<TObjectPtr<AActor>>();

protected:
	virtual void OnActivatePuzzleAction() override;

	virtual void OnDeactivatePuzzleAction() override;

private:
	void SetVisible(bool IsVisible);
};
