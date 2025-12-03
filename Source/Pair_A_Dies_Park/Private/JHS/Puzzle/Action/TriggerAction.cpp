// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/Puzzle/Action/TriggerAction.h"
#include "JHS/Puzzle/Trigger/ActionTrigger.h"

void ATriggerAction::OnActivatePuzzleAction()
{
	if (_actionTrigger != nullptr)
	{
		_actionTrigger->ActionTriggerEnter();
	}
}

void ATriggerAction::OnDeactivatePuzzleAction()
{
	if (_actionTrigger != nullptr)
	{
		_actionTrigger->ActionTriggerExit();
	}
}