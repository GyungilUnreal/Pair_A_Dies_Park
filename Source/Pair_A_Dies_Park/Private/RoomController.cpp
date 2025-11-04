// Fill out your copyright notice in the Description page of Project Settings.


#include "RoomController.h"
#include "MyGameInstance.h"
#include "RoomManager.h"
#include "Kismet/GameplayStatics.h"
#include "PuzzleBase.h"
#include "MyGameMode.h"

// Sets default values
ARoomController::ARoomController()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ARoomController::BeginPlay()
{
	Super::BeginPlay();
}

// Called every frame
void ARoomController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ARoomController::InitializeRoomController(int32 RoomSequence)
{
	_roomSequence = RoomSequence;
	UE_LOG(LogTemp, Warning, TEXT("Initialize Room %d"), _roomSequence);

	_puzzleArray = SearchPuzzle();
	for (int32 i = 0; i < _puzzleArray.Num(); i++)
	{
		_puzzleArray[i]->InitializePuzzle(this, i);
	}
}

void ARoomController::OnCompletePuzzle(int32 completedPuzzleIndex)
{
	for (int32 i = 0; i < _puzzleArray.Num(); i++)
	{
		if (!_puzzleArray[i]->IsCompletedPuzzle())
			return;
	}

	GetRoomManager()->OnCompletedRoom(_roomSequence);
}

TArray<TObjectPtr<class APuzzleBase>> ARoomController::SearchPuzzle()
{
	TArray<TObjectPtr<class APuzzleBase>> _resultPuzzleArray = TArray<TObjectPtr<class APuzzleBase>>();

	// 월드에서 모든 APuzzleBase 액터 가져오기
	TArray<AActor*> _foundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APuzzleBase::StaticClass(), _foundActors);
	
	// 범위 내에 있는 퍼즐만 필터링
	for (AActor* _actor : _foundActors)
	{
		TObjectPtr<APuzzleBase> _puzzle = Cast<APuzzleBase>(_actor);
		if (_puzzle)
		{
			_resultPuzzleArray.Add(_puzzle);
		}
	}

	return _resultPuzzleArray;
}

TObjectPtr<URoomManager> ARoomController::GetRoomManager() const
{
	TObjectPtr<AMyGameMode> _gameMode = Cast<AMyGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
	if (!_gameMode)
	{
		UE_LOG(LogTemp, Error, TEXT("GameMode is nullptr"));
		return nullptr;
	}

	TObjectPtr<URoomManager> _roomManager = _gameMode->GetRoomManager();
	if (_roomManager == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("RoomManager is nullptr"));
		return nullptr;
	}

	return _roomManager;
}

// Debug
void ARoomController::StartSecondRoom()
{
	TObjectPtr<UMyGameInstance> _gameInstance = Cast<UMyGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
	InitializeRoomController(_gameInstance->GetCurrentRoomSequence());
}