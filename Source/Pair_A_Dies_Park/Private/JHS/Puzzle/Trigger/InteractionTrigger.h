// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JHS/Puzzle/Trigger/PuzzleTriggerBase.h"
#include "InteractionTrigger.generated.h"

class USphereComponent;

UCLASS()
class AInteractionTrigger : public APuzzleTriggerBase
{
	GENERATED_BODY()

public:
	AInteractionTrigger();

public:
	void InteractActicate();

	void InteractDeacticate();
};
