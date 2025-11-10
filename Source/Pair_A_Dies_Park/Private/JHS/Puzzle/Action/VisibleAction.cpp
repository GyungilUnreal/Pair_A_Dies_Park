// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/Puzzle/Action/VisibleAction.h"

void AVisibleAction::OnActivatePuzzleAction()
{
	Super::OnActivatePuzzleAction();

	SetVisible(true);
}

void AVisibleAction::OnDeactivatePuzzleAction()
{
	Super::OnDeactivatePuzzleAction();

	SetVisible(false);
}

void AVisibleAction::SetVisible(bool IsVisible)
{
	for (TObjectPtr<AActor> _actor : _visibleObjectArray)
	{
		if (_actor == nullptr)
			continue;

		_actor->SetActorHiddenInGame(!IsVisible);
		_actor->SetActorEnableCollision(IsVisible);
		_actor->SetActorTickEnabled(IsVisible);
	}
}