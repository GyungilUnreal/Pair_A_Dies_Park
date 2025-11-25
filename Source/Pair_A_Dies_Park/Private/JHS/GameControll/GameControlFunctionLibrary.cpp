// Fill out your copyright notice in the Description page of Project Settings.


#include "JHS/GameControll/GameControlFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "JHS/GameControll/MyGameInstance.h"
#include "JHS/GameControll/RoomSubsystem.h"
#include "JHS/GameControll/MyGameMode.h"

bool UGameControlFunctionLibrary::TryGetGameInstance(TObjectPtr<UMyGameInstance>& OutGameInstance)
{
    TObjectPtr<UWorld> World = nullptr;
    if (!TryGetWorld(World))
        return false;

    OutGameInstance = Cast<UMyGameInstance>(UGameplayStatics::GetGameInstance(World));
    if (!OutGameInstance)   
    {
        UE_LOG(LogTemp, Error, TEXT("GameInstance is nullptr"));
        return false;
    }

    return true;
}

bool UGameControlFunctionLibrary::TryGetRoomSubSystem(TObjectPtr<UMyGameInstance> GameInstnace, TObjectPtr<URoomSubsystem>& OutRoomSubSystem)
{
    TObjectPtr<UMyGameInstance> OutGameInstance = GameInstnace;
    if (!OutGameInstance)
    {
        if (!TryGetGameInstance(OutGameInstance))
            return false;
    }

    // 게임 인스턴스에서 직접 서브시스템 가져오기
    OutRoomSubSystem = OutGameInstance->GetSubsystem<URoomSubsystem>();
    if (!OutRoomSubSystem)
    {
        UE_LOG(LogTemp, Error, TEXT("RoomSubsystem is nullptr"));
        return false;
    }

    return true;
}

bool UGameControlFunctionLibrary::TryGetGameMode(TObjectPtr<AMyGameMode>& OutGameMode)
{
    TObjectPtr<UWorld> World = nullptr;
    if (!TryGetWorld(World))
        return false;

    OutGameMode = Cast<AMyGameMode>(UGameplayStatics::GetGameMode(World));
    if (!OutGameMode)
    {
        UE_LOG(LogTemp, Error, TEXT("GameMode is nullptr"));
        return false;
    }

    return true;
}

bool UGameControlFunctionLibrary::TryGetWorld(TObjectPtr<UWorld>& OutWorld)
{
    OutWorld = GEngine ? GEngine->GetCurrentPlayWorld() : nullptr;
    if (!OutWorld)
    {
        UE_LOG(LogTemp, Error, TEXT("World is nullptr"));
        return false;
    }

    return true;
}