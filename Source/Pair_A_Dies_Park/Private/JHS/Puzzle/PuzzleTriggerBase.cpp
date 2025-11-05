// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/Puzzle/PuzzleTriggerBase.h"
#include "JHS/Puzzle/PuzzleBase.h"

// Sets default values
APuzzleTriggerBase::APuzzleTriggerBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APuzzleTriggerBase::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void APuzzleTriggerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APuzzleTriggerBase::InitializePuzzleTrigger(TObjectPtr<class APuzzleBase> Puzzle)
{
	_puzzle = Puzzle;

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &APuzzleTriggerBase::OnTrigger, _triggerDelay);
}

void APuzzleTriggerBase::OnTrigger()
{
	UE_LOG(LogTemp, Warning, TEXT("Triggered %s"), *GetName());
	_puzzle->OnChangeTriggerState(this, true);
}