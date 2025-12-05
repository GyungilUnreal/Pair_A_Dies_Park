// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/Puzzle/Trigger/ActionTrigger.h"
#include "JHS/Puzzle/Trigger/PuzzleTriggerBase.h"

void AActionTrigger::TriggerEnterEffect()
{
}

void AActionTrigger::TriggerExitEffect()
{
}

void AActionTrigger::ResetTriggerOverride()
{
	if (_originTrigger == nullptr || _originTrigger == this)
		return;

	_originTrigger->ResetTrigger();
}

void AActionTrigger::ActionTriggerEnter()
{
	OnTriggerEnter();
}

void AActionTrigger::ActionTriggerExit()
{
	OnTriggerExit();
}