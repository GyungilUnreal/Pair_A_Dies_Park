// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/GameControll/MyGameMode.h"
#include "Kismet/GameplayStatics.h"
#include "MyGameInstance.h"
#include "JHS/Room/RoomManager.h"

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
    TObjectPtr<UMyGameInstance> _gameInstance = Cast<UMyGameInstance>(UGameplayStatics::GetGameInstance(GetWorld()));
    _gameInstance->RegistRoomSequence(_roomManager->CreateRandomRoom());

    // 튜토리얼 여부에 따라 시작 방 인덱스 결정
    _gameInstance->ChangeRoomSequence(IsTutorial ? -1 : 0);
}

void AMyGameMode::OnGameEnd()
{
    UE_LOG(LogTemp, Warning, TEXT("Game end"));
}