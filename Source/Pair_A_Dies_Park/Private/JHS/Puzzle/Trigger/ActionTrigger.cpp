// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/Puzzle/Trigger/ActionTrigger.h"

void AActionTrigger::TriggerEnterEffect()
{
}

void AActionTrigger::TriggerExitEffect()
{
}

void AActionTrigger::ActionTriggerEnter()
{
	OnTriggerEnter();
}

void AActionTrigger::ActionTriggerExit()
{
	OnTriggerExit();
}