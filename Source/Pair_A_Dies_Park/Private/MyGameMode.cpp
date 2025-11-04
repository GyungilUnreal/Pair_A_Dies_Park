// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameMode.h"
#include "RoomManager.h"

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
    // 튜토리얼 여부에 따라 시작 방 인덱스 결정
    int32 NextRoomIndex = IsTutorial ? -1 : 0;

    // 3초 후에 레벨 로드
    FTimerHandle TimerHandle;
    GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateLambda([this, NextRoomIndex]()
    {
        if (_roomManager)
        {
            _roomManager->LoadLevel(NextRoomIndex);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("RoomManager is null in StartGame timer callback"));
        }
    }), 3.0f, false);
}