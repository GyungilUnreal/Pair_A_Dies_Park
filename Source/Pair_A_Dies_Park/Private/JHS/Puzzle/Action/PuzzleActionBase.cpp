// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/Puzzle/Action/PuzzleActionBase.h"

// Sets default values
APuzzleActionBase::APuzzleActionBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APuzzleActionBase::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void APuzzleActionBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APuzzleActionBase::ActivatePuzzleAction()
{
	if (_isActive)
		return;

	_isActive = true;
	OnActivatePuzzleAction();

	if (_isDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("Activate Puzzle Action %d"), _actionIndex);
	}
}

void APuzzleActionBase::DeactivatePuzzleAction()
{
	if (!_isActive)
		return;

	_isActive = false;
	OnDeactivatePuzzleAction();

	if (_isDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("Deactivate Puzzle Action %d"), _actionIndex);
	}
}