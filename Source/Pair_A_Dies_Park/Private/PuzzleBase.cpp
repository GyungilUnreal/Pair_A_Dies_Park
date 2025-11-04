// Fill out your copyright notice in the Description page of Project Settings.


#include "PuzzleBase.h"
#include "RoomController.h"

// Sets default values
APuzzleBase::APuzzleBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void APuzzleBase::BeginPlay()
{
	Super::BeginPlay();
	
	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &APuzzleBase::CompletePuzzle, _completeDelay);
}

// Called every frame
void APuzzleBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void APuzzleBase::InitializePuzzle(TObjectPtr<ARoomController> RoomController, int32 PuzzleIndex)
{
	_roomController = RoomController;
	_puzzleIndex = PuzzleIndex;
	_isCompleted = false;
}

void APuzzleBase::CompletePuzzle()
{
	_isCompleted = true;
	UE_LOG(LogTemp, Warning, TEXT("%f"), _completeDelay);
	_roomController->OnCompletePuzzle(_puzzleIndex);
}