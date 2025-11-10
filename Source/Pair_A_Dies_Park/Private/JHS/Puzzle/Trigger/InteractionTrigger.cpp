// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/Puzzle/Trigger/InteractionTrigger.h"
#include "Components/SphereComponent.h"
#include "Split_Character.h"

AInteractionTrigger::AInteractionTrigger()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AInteractionTrigger::InteractActicate()
{
    OnTriggerEnter();
}

void AInteractionTrigger::InteractDeacticate()
{
    OnTriggerExit();
}