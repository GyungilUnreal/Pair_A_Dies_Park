// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/GameControll/MyGameMode.h"
#include "JHS/GameControll/GameControlFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "JHS/GameControll/MyGameInstance.h"
#include "JHS/Room/RoomManager.h"
#include "CharacterFunctionLibrary.h"

AMyGameMode::AMyGameMode()
{
    _roomManager = CreateDefaultSubobject<URoomManager>(TEXT("RoomManager"));
    if (_roomManager == nullptr)
    {
        UE_LOG(LogTemp, Error, TEXT("RoomManager is nullptr"));
        return;
    }
}

void AMyGameMode::BeginPlay()
{
    Super::BeginPlay();
}

void AMyGameMode::StartGame(bool IsTutorial)
{
    TObjectPtr<UMyGameInstance> _gameInstance = nullptr;
    if (!UGameControlFunctionLibrary::TryGetGameInstance(_gameInstance))
        return;

    // 튜토리얼 여부
    TArray<int32> _roomSequenceArray;
    if (!IsTutorial)
    {
        _roomSequenceArray = _roomManager->CreateRandomRoom();
    }

    _gameInstance->StartRoom(IsTutorial, _roomSequenceArray);
}

void AMyGameMode::OnGameEnd()
{
    UE_LOG(LogTemp, Warning, TEXT("Game end"));
}